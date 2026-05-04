package main

import "fmt"

type UDTConfig struct {
	TxMaxSeq            uint32
	TxInitialExpUs      uint64
	TxInitialFlowWindow uint32
	RxInitialFlowWindow uint32
	RxAckIntervalUs     uint64
	RxNakIntervalUs     uint64
	RxExpIntervalUs     uint64
	RxImmediateNakMinUs uint64
}

func DefaultUDTConfig() UDTConfig {
	return UDTConfig{
		TxMaxSeq:            0x000FFFFF,
		TxInitialExpUs:      300000,
		TxInitialFlowWindow: 45600,
		RxInitialFlowWindow: 45600,
		RxAckIntervalUs:     10000,
		RxNakIntervalUs:     300000,
		RxExpIntervalUs:     300000,
		RxImmediateNakMinUs: 4000,
	}
}

type TXState int

const (
	TXActive TXState = iota
	TXWait
)

type UDT struct {
	name string
	disp *Dispatcher
	cfg  UDTConfig
	cc   CC

	isSender bool

	txNextSeq               uint32
	txLastAcked             uint32
	txState                 TXState
	txMaxSeq                uint32
	txFlowWindowSizeInBytes uint32
	avgPacketSizeBytes      uint32
	txLastSendUs            uint64
	txLastAckUs             uint64
	txExpUs                 uint64
	txExpTimeoutUs          uint64

	rxHighestContig         uint32
	rxFlowWindowSizeInBytes uint32
	rxLastProgressUs        uint64
	rxLastAckSentUs         uint64
	rxLastNakSentUs         uint64
	rxAckIntUs              uint64
	rxNakIntUs              uint64
	rxExpIntUs              uint64
	rxLastGapFirstMissing   uint32
	rxLastImmediateNakUs    uint64

	txInFlight *SequenceMap
	txMissing  *SequenceMap
	rxReceived *SequenceMap
	rxMissing  *SequenceMap

	buildData func(seq uint32) bool
	send      func(pkt Packet)
}

func NewUDT(name string, isSender bool, cfg UDTConfig, cc CC, disp *Dispatcher, maxSeq uint32, avgPacketSize uint32, sendFn func(pkt Packet), buildData func(seq uint32) bool) *UDT {
	u := &UDT{
		name:                    name,
		disp:                    disp,
		cfg:                     cfg,
		cc:                      cc,
		isSender:                isSender,
		txState:                 TXActive,
		txMaxSeq:                cfg.TxMaxSeq,
		txFlowWindowSizeInBytes: cfg.TxInitialFlowWindow,
		avgPacketSizeBytes:      avgPacketSize,
		txExpUs:                 cfg.TxInitialExpUs,
		rxHighestContig:         0xFFFFFFFF,
		rxFlowWindowSizeInBytes: cfg.RxInitialFlowWindow,
		rxAckIntUs:              cfg.RxAckIntervalUs,
		rxNakIntUs:              cfg.RxNakIntervalUs,
		rxExpIntUs:              cfg.RxExpIntervalUs,
		rxLastGapFirstMissing:   0xFFFFFFFF,
		txInFlight:              NewSequenceMap(maxSeq, SeqMapLowest, 11),
		txMissing:               NewSequenceMap(maxSeq, SeqMapRoundRobin, 17),
		rxReceived:              NewSequenceMap(maxSeq, SeqMapLowest, 23),
		rxMissing:               NewSequenceMap(maxSeq, SeqMapLowest, 29),
		send:                    sendFn,
		buildData:               buildData,
	}
	return u
}

func (u *UDT) txNextTickUs(nowUs uint64) uint64 {
	if u.txState == TXWait {
		return u.txExpTimeoutUs
	}
	pt := u.cc.PacingTimeoutUs(nowUs, u.txLastSendUs)
	if pt > 0 {
		return pt
	}
	return 0
}

func (u *UDT) TxTick(nowUs uint64) {
	if u.txState == TXWait {
		if nowUs < u.txExpTimeoutUs {
			return
		}
		if u.txMissing.Size() == 0 {
			u.txMissing.Merge(u.txInFlight)
		}
		u.txState = TXActive
		u.cc.OnTimeout(nowUs)
		u.disp.OnTxTimeout(nowUs)
	}

	if u.cc.BudgetBeforeCongestion() == 0 {
		u.txState = TXWait
		u.txExpTimeoutUs = nowUs + u.txExpUs
		return
	}

	if nowUs < u.cc.PacingTimeoutUs(nowUs, u.txLastSendUs) {
		return
	}

	if u.txMissing.Size() > 0 {
		seq := u.txMissing.Pop()
		if seq >= 0 && u.buildData(uint32(seq)) {
			pkt := Packet{Type: PktData, Seq: uint32(seq), From: "sender", To: "receiver"}
			u.send(pkt)
			u.txLastSendUs = nowUs
			u.disp.OnTxSendRetransmit(nowUs, uint32(seq))
			return
		}
	}

	if u.txNextSeq < u.txMaxSeq {
		if u.txInFlight.Size()*u.avgPacketSizeBytes < u.txFlowWindowSizeInBytes {
			seq := u.txNextSeq
			if u.buildData(seq) {
				u.txInFlight.Push(seq)
				u.cc.OnPacketSent(seq, nowUs)
				u.send(Packet{Type: PktData, Seq: seq, From: "sender", To: "receiver"})
				u.txLastSendUs = nowUs
				u.txNextSeq++
				u.disp.OnTxSendData(nowUs, seq)
				return
			}
		}
	}

	u.txState = TXWait
	u.txExpTimeoutUs = nowUs + u.txExpUs
}

func (u *UDT) OnTxAckReceived(ackSeq uint32, flowWindow uint32, nowUs uint64) {
	if ackSeq > u.txNextSeq {
		u.txState = TXActive
		u.disp.OnTxAckIgnored(nowUs, ackSeq, "ack-beyond-sent")
		return
	}
	if ackSeq <= u.txLastAcked {
		u.txState = TXActive
		u.disp.OnTxAckIgnored(nowUs, ackSeq, "stale-or-duplicate")
		return
	}
	u.txInFlight.RemoveRange(u.txLastAcked, ackSeq)
	u.txMissing.RemoveRange(u.txLastAcked, ackSeq)
	u.txLastAcked = ackSeq
	u.txLastAckUs = nowUs
	u.txFlowWindowSizeInBytes = flowWindow
	u.txState = TXActive
	u.cc.OnAck(ackSeq, nowUs)
	u.disp.OnTxAckAccepted(nowUs, ackSeq)
}

func (u *UDT) OnTxNakReceived(missing []uint32, flowWindow uint32, nowUs uint64) {
	nakMap := NewSequenceMap(u.txMaxSeq, SeqMapLowest, 31)
	for _, s := range missing {
		nakMap.Push(s)
	}
	u.txMissing.Merge(nakMap)
	lossCount := uint32(len(missing))
	u.cc.OnLoss(u.txLastAcked+1, lossCount, nowUs)
	u.txFlowWindowSizeInBytes = flowWindow
	u.txState = TXActive
	u.txExpTimeoutUs = nowUs + u.txExpUs
	u.disp.OnTxNakReceived(nowUs, lossCount)
}

func (u *UDT) RxTick(nowUs uint64) {
	if (nowUs-u.rxLastAckSentUs) >= u.rxAckIntUs && u.rxHighestContig < 0xFFFFFFFF {
		ackSeq := u.rxHighestContig + 1
		u.send(Packet{Type: PktAck, AckSeq: ackSeq, FlowWindow: u.rxFlowWindowSizeInBytes, From: "receiver", To: "sender"})
		u.rxLastAckSentUs = nowUs
		u.disp.OnRxAckSent(nowUs, ackSeq)
	}
	if u.rxMissing.Size() > 0 && (nowUs-u.rxLastNakSentUs) >= u.rxNakIntUs {
		// Limit the amount of missing sequence numbers included in a NAK to avoid creating excessively large NAK packets.
		missing := u.rxMissing.ToSlice()
		fmt.Printf("[%10dus] %s: preparing NAK for missing packets: %v\n", nowUs, u.name, missing)
		u.send(Packet{Type: PktNak, Missing: missing, FlowWindow: u.rxFlowWindowSizeInBytes, From: "receiver", To: "sender"})
		u.rxLastNakSentUs = nowUs
	}
	if (nowUs - u.rxLastProgressUs) >= u.rxExpIntUs {
		u.rxLastProgressUs = nowUs
	}
}

func (u *UDT) OnRxDataReceived(seq uint32, nowUs uint64) {
	u.cc.OnPacketReceived(seq, nowUs)
	u.disp.OnRxDataReceived(nowUs, seq)
	if !u.rxReceived.Push(seq) {
		// Duplicate - could be a spurious rxMissing entry; clean it up.
		if u.rxMissing.Has(seq) {
			u.rxMissing.Remove(seq)
			fmt.Printf("[%10dus] %s: removed spurious missing entry for duplicate packet %d\n", nowUs, u.name, seq)
		}
		return
	}
	if seq > (u.rxHighestContig + 1) {
		firstMissing := u.rxHighestContig + 1
		for s := firstMissing; s < seq; s++ {
			if !u.rxReceived.Has(s) {
				u.rxMissing.Push(s)
			}
		}
		u.disp.OnRxGapDetected(nowUs, firstMissing, seq)

		// On gap detection, pull in the periodic NAK schedule instead of
		// sending an immediate NAK. This is friendlier on low-loss LAN/WiFi
		// where short-lived gaps are often caused by reordering.
		pullInUs := u.cfg.RxImmediateNakMinUs

		targetNextNakUs := nowUs + pullInUs
		currentNextNakUs := u.rxLastNakSentUs + u.rxNakIntUs
		if targetNextNakUs < currentNextNakUs {
			if targetNextNakUs > u.rxNakIntUs {
				u.rxLastNakSentUs = targetNextNakUs - u.rxNakIntUs
			} else {
				u.rxLastNakSentUs = 0
			}
		}

		u.rxLastImmediateNakUs = nowUs
		u.rxLastGapFirstMissing = firstMissing
	}
	u.rxMissing.Remove(seq)
	if u.rxMissing.Size() == 0 {
		// No active gap remains; restore normal NAK cadence baseline.
		u.rxLastNakSentUs = nowUs
		u.rxLastGapFirstMissing = 0xFFFFFFFF
		u.rxLastImmediateNakUs = 0
	}
	for u.rxReceived.Has(u.rxHighestContig + 1) {
		u.rxHighestContig++
	}
	u.rxLastProgressUs = nowUs
}

func (u *UDT) AckedPacketCount() uint32 {
	return u.txLastAcked
}

func (u *UDT) SentPacketCount() uint32 {
	return u.txNextSeq
}

func (u *UDT) ReceivedPacketCount() uint32 {
	if u.rxHighestContig == 0xFFFFFFFF {
		return 0
	}
	return u.rxHighestContig + 1
}
