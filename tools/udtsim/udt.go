package main

type UDTConfig struct {
	TxMaxSeq                   uint32
	TxInitialExpUs             uint64
	TxInitialFlowWindow        uint32
	RxInitialFlowWindow        uint32
	RxAckIntervalUs            uint64
	RxNakIntervalUs            uint64
	RxExpIntervalUs            uint64
	RxImmediateNakMinUs        uint64
	RxNakUrgencyScalePct       uint32
	RxNakUrgencySmoothPct      uint32
	RxNakIntervalMaxMultiplier uint32
}

func DefaultUDTConfig() UDTConfig {
	return UDTConfig{
		TxMaxSeq:                   0x000FFFFF,
		TxInitialExpUs:             300000,
		TxInitialFlowWindow:        45600,
		RxInitialFlowWindow:        45600,
		RxAckIntervalUs:            10000,
		RxNakIntervalUs:            300000,
		RxExpIntervalUs:            300000,
		RxImmediateNakMinUs:        4000,
		RxNakUrgencyScalePct:       100,
		RxNakUrgencySmoothPct:      60,
		RxNakIntervalMaxMultiplier: 2,
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
	rxNakWindowBeginUs      uint64
	rxNakWindowEndUs        uint64
	rxNakWindowRecvCount    uint32
	rxNakUrgency            uint32
	rxNakDue                bool
	rxNakUrgencyScalePct    uint32
	rxNakUrgencySmoothPct   uint32
	rxNakMaxMultiplier      uint32

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
		rxNakUrgencyScalePct:    cfg.RxNakUrgencyScalePct,
		rxNakUrgencySmoothPct:   cfg.RxNakUrgencySmoothPct,
		rxNakMaxMultiplier:      cfg.RxNakIntervalMaxMultiplier,
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

// Tick simulates the passage of time for the sender side, returning the next time it
// needs to be ticked again.
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

	ccs := u.cc.Snapshot()
	inFlightBytes := ccmulU32(u.txInFlight.Size(), u.avgPacketSizeBytes)
	flowBudgetBytes := uint32(0)
	if inFlightBytes < u.txFlowWindowSizeInBytes {
		flowBudgetBytes = u.txFlowWindowSizeInBytes - inFlightBytes
	}
	u.disp.OnTxCCSample(nowUs, ccs.Cwnd, ccs.InFlight, ccs.Budget, ccs.PacingUs, ccs.SlowStart)
	u.disp.OnTxFlowSample(nowUs, u.txFlowWindowSizeInBytes, inFlightBytes, flowBudgetBytes)

	for {
		if u.cc.BudgetBeforeCongestion() == 0 {
			ccs = u.cc.Snapshot()
			u.disp.OnTxCCBlocked(nowUs, ccs.Cwnd, ccs.InFlight)
			u.txState = TXWait
			u.txExpTimeoutUs = nowUs + u.txExpUs
			return
		}

		// Virtual send-time cursor: advance txLastSendUs by one pacing interval per packet.
		// PacingTimeoutUs also captures explicit delays from OnLoss/OnTimeout (via cc.nextSendUs).
		nextSendUs := u.cc.PacingTimeoutUs(nowUs, u.txLastSendUs)
		if nextSendUs > nowUs {
			return
		}

		if u.txMissing.Size() > 0 {
			seq := u.txMissing.Pop()
			if seq >= 0 && u.buildData(uint32(seq)) {
				// type Packet struct {
				// 	Type       PacketType
				// 	Seq        uint32
				// 	AckSeq     uint32
				// 	FlowWindow uint32
				// 	Missing    []uint32
				// 	SizeBytes  int
				// 	From       string
				// 	To         string
				// }
				pkt := Packet{Type: PktData, Seq: uint32(seq), From: "sender", To: "receiver"}
				u.cc.OnMissingPacketSent(uint32(seq), nowUs)
				u.send(pkt)
				u.txLastSendUs = nextSendUs
				u.disp.OnTxSendRetransmit(nowUs, uint32(seq))
				continue
			}
		}

		if u.txNextSeq < u.txMaxSeq {
			if u.txInFlight.Size()*u.avgPacketSizeBytes < u.txFlowWindowSizeInBytes {
				seq := u.txNextSeq
				if u.buildData(seq) {
					u.txInFlight.Push(seq)
					u.cc.OnPacketSent(seq, nowUs)
					u.send(Packet{Type: PktData, Seq: seq, From: "sender", To: "receiver"})
					u.txLastSendUs = nextSendUs
					u.txNextSeq++
					u.disp.OnTxSendData(nowUs, seq)
					continue
				}
			}
			u.disp.OnTxFlowBlocked(nowUs, u.txFlowWindowSizeInBytes, u.txInFlight.Size()*u.avgPacketSizeBytes)
		}

		// Nothing to send.
		u.txState = TXWait
		u.txExpTimeoutUs = nowUs + u.txExpUs
		return
	}
}

func (u *UDT) computeNakWindowIntervalUs() uint64 {
	base := u.rxNakIntUs
	if base == 0 {
		base = 1
	}
	minInt := base
	maxMul := uint64(u.rxNakMaxMultiplier)
	if maxMul == 0 {
		maxMul = 1
	}
	maxInt := base * maxMul
	urgency := clampUrgencyU32(u.rxNakUrgency, 0, 100)
	x := uint64(100 - urgency)
	return minInt + ((maxInt-minInt)*x)/100
}

func (u *UDT) updateRxNakWindow(nowUs uint64) {
	if u.rxNakWindowEndUs == 0 {
		intUs := u.computeNakWindowIntervalUs()
		u.rxNakWindowBeginUs = nowUs
		u.rxNakWindowEndUs = nowUs + intUs
		u.rxNakWindowRecvCount = 0
		return
	}

	for nowUs >= u.rxNakWindowEndUs {
		recv := u.rxNakWindowRecvCount
		missing := u.rxMissing.Size()
		denom := recv + missing
		if denom == 0 {
			denom = 1
		}

		instantUrgency := (missing*100 + (denom >> 1)) / denom
		if missing > 0 && instantUrgency == 0 {
			instantUrgency = 1
		}

		scale := clampUrgencyU32(u.rxNakUrgencyScalePct, 1, 200)
		instantUrgency = clampUrgencyU32((instantUrgency*scale)/100, 0, 100)

		smooth := clampUrgencyU32(u.rxNakUrgencySmoothPct, 0, 100)
		u.rxNakUrgency = clampUrgencyU32((u.rxNakUrgency*smooth+instantUrgency*(100-smooth))/100, 0, 100)
		if instantUrgency > 0 && u.rxNakUrgency == 0 {
			u.rxNakUrgency = 1
		}

		if missing > 0 {
			u.rxNakDue = true
		}

		intervalUs := u.computeNakWindowIntervalUs()
		u.disp.OnRxNakWindowClosed(nowUs, recv, missing, u.rxNakUrgency, intervalUs)

		u.rxNakWindowRecvCount = 0
		u.rxNakWindowBeginUs = u.rxNakWindowEndUs
		u.rxNakWindowEndUs = u.rxNakWindowBeginUs + intervalUs
	}
}

func (u *UDT) rxNextTickUs(nowUs uint64) uint64 {
	u.updateRxNakWindow(nowUs)

	nextAckUs := u.rxLastAckSentUs + u.rxAckIntUs
	nextNakUs := u.rxNakWindowEndUs
	if u.rxNakDue {
		nextNakUs = nowUs
	}
	nextExpUs := u.rxLastProgressUs + u.rxExpIntUs

	nextTickUs := nextAckUs
	if nextNakUs < nextTickUs {
		nextTickUs = nextNakUs
	}
	if nextExpUs < nextTickUs {
		nextTickUs = nextExpUs
	}
	return nextTickUs
}

func (u *UDT) OnTxAckReceived(ackSeq uint32, flowWindow uint32, nowUs uint64) {
	if ackSeq > u.txNextSeq {
		u.txState = TXActive
		u.disp.OnTxAckIgnored(nowUs, ackSeq, "ack-beyond-sent")
		return
	}
	if ackSeq <= u.txLastAcked {
		u.txState = TXActive
		u.cc.OnAck(ackSeq, nowUs) // safe: cc.OnAck guards lastAckSeq; releases pacing
		u.disp.OnTxAckIgnored(nowUs, ackSeq, "stale-or-duplicate")
		return
	}
	u.txInFlight.RemoveRange(u.txLastAcked, ackSeq)
	u.txMissing.RemoveRange(u.txLastAcked, ackSeq)
	u.txLastAcked = ackSeq
	u.txLastAckUs = nowUs
	prevFlowWindow := u.txFlowWindowSizeInBytes
	u.txFlowWindowSizeInBytes = flowWindow
	if prevFlowWindow != flowWindow {
		u.disp.OnTxFlowWindowUpdate(nowUs, prevFlowWindow, flowWindow, "ack")
	}
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
	prevFlowWindow := u.txFlowWindowSizeInBytes
	u.txFlowWindowSizeInBytes = flowWindow
	if prevFlowWindow != flowWindow {
		u.disp.OnTxFlowWindowUpdate(nowUs, prevFlowWindow, flowWindow, "nak")
	}
	u.txState = TXActive
	u.txExpTimeoutUs = nowUs + u.txExpUs
	u.disp.OnTxNakReceived(nowUs, lossCount)
}

func (u *UDT) RxTick(nowUs uint64) {
	_ = u.rxNextTickUs(nowUs)

	if (nowUs-u.rxLastAckSentUs) >= u.rxAckIntUs && u.rxHighestContig < 0xFFFFFFFF {
		ackSeq := u.rxHighestContig + 1
		u.send(Packet{Type: PktAck, AckSeq: ackSeq, FlowWindow: u.rxFlowWindowSizeInBytes, From: "receiver", To: "sender"})
		u.rxLastAckSentUs = nowUs
		u.disp.OnRxAckSent(nowUs, ackSeq)
	}
	if u.rxNakDue && u.rxMissing.Size() > 0 {
		missing := u.rxMissing.ToSlice()
		nakInt := u.computeNakWindowIntervalUs()
		u.send(Packet{Type: PktNak, Missing: missing, FlowWindow: u.rxFlowWindowSizeInBytes, From: "receiver", To: "sender"})
		u.rxLastNakSentUs = nowUs
		u.rxNakDue = false
		u.disp.OnRxNakSent(nowUs, uint32(len(missing)), u.rxNakUrgency, nakInt)
	}
	if (nowUs - u.rxLastProgressUs) >= u.rxExpIntUs {
		u.rxLastProgressUs = nowUs
	}
}

func (u *UDT) OnRxDataReceived(seq uint32, nowUs uint64) {
	u.disp.OnRxDataReceived(nowUs, seq)
	if !u.rxReceived.Push(seq) {
		u.rxMissing.Remove(seq)
		return
	}
	u.rxNakWindowRecvCount++
	if seq > (u.rxHighestContig + 1) {
		firstMissing := u.rxHighestContig + 1
		for s := firstMissing; s < seq; s++ {
			if !u.rxReceived.Has(s) {
				u.rxMissing.Push(s)
			}
		}
		u.disp.OnRxGapDetected(nowUs, firstMissing, seq)
	}
	u.rxMissing.Remove(seq)
	if u.rxMissing.Size() == 0 {
		u.rxNakDue = false
		u.rxNakUrgency = 0
		u.rxNakWindowBeginUs = nowUs
		u.rxNakWindowEndUs = nowUs + u.computeNakWindowIntervalUs()
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
