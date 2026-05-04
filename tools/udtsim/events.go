package main

import "fmt"

type EventKind string

const (
	EventTickStart          EventKind = "TickStart"
	EventTickEnd            EventKind = "TickEnd"
	EventPacketBuilt        EventKind = "PacketBuilt"
	EventPacketQueued       EventKind = "PacketQueued"
	EventPacketReordered    EventKind = "PacketReordered"
	EventPacketDropped      EventKind = "PacketDropped"
	EventPacketDelivered    EventKind = "PacketDelivered"
	EventTxTimeout          EventKind = "TxTimeout"
	EventTxSendData         EventKind = "TxSendData"
	EventTxSendRetransmit   EventKind = "TxSendRetransmit"
	EventTxAckAccepted      EventKind = "TxAckAccepted"
	EventTxAckIgnored       EventKind = "TxAckIgnored"
	EventTxNakReceived      EventKind = "TxNakReceived"
	EventRxDataReceived     EventKind = "RxDataReceived"
	EventRxGapDetected      EventKind = "RxGapDetected"
	EventRxImmediateNakSent EventKind = "RxImmediateNakSent"
	EventRxAckSent          EventKind = "RxAckSent"
)

type Event struct {
	Kind EventKind
	TSUs uint64
	Data map[string]any
}

type EventSink interface {
	OnEvent(e Event)
}

type Dispatcher struct {
	sinks []EventSink
}

func (d *Dispatcher) Register(s EventSink) {
	d.sinks = append(d.sinks, s)
}

func (d *Dispatcher) emit(kind EventKind, tsUs uint64, data map[string]any) {
	e := Event{Kind: kind, TSUs: tsUs, Data: data}
	for _, s := range d.sinks {
		s.OnEvent(e)
	}
}

func (d *Dispatcher) OnTickStart(tsUs uint64, tick uint64) {
	d.emit(EventTickStart, tsUs, map[string]any{"tick": tick})
}

func (d *Dispatcher) OnTickEnd(tsUs uint64, tick uint64) {
	d.emit(EventTickEnd, tsUs, map[string]any{"tick": tick})
}

func (d *Dispatcher) OnPacketBuilt(tsUs uint64, from, to, pktType string, seq uint32) {
	d.emit(EventPacketBuilt, tsUs, map[string]any{"from": from, "to": to, "type": pktType, "seq": seq})
}

func (d *Dispatcher) OnPacketQueued(tsUs uint64, from, to, pktType string, seq uint32, deliverAtUs uint64) {
	d.emit(EventPacketQueued, tsUs, map[string]any{"from": from, "to": to, "type": pktType, "seq": seq, "deliver_at": deliverAtUs})
}

func (d *Dispatcher) OnPacketReordered(tsUs uint64, direction string, firstType string, firstSeq uint32, secondType string, secondSeq uint32) {
	d.emit(EventPacketReordered, tsUs, map[string]any{
		"direction": direction,
		"first_type": firstType,
		"first_seq": firstSeq,
		"second_type": secondType,
		"second_seq": secondSeq,
	})
}

func (d *Dispatcher) OnPacketDropped(tsUs uint64, from, to, pktType string, seq uint32, reason string) {
	d.emit(EventPacketDropped, tsUs, map[string]any{"from": from, "to": to, "type": pktType, "seq": seq, "reason": reason})
}

func (d *Dispatcher) OnPacketDelivered(tsUs uint64, from, to, pktType string, seq uint32) {
	d.emit(EventPacketDelivered, tsUs, map[string]any{"from": from, "to": to, "type": pktType, "seq": seq})
}

func (d *Dispatcher) OnTxTimeout(tsUs uint64) {
	d.emit(EventTxTimeout, tsUs, nil)
}

func (d *Dispatcher) OnTxSendData(tsUs uint64, seq uint32) {
	d.emit(EventTxSendData, tsUs, map[string]any{"seq": seq})
}

func (d *Dispatcher) OnTxSendRetransmit(tsUs uint64, seq uint32) {
	d.emit(EventTxSendRetransmit, tsUs, map[string]any{"seq": seq})
}

func (d *Dispatcher) OnTxAckAccepted(tsUs uint64, ackSeq uint32) {
	d.emit(EventTxAckAccepted, tsUs, map[string]any{"ack": ackSeq})
}

func (d *Dispatcher) OnTxAckIgnored(tsUs uint64, ackSeq uint32, reason string) {
	d.emit(EventTxAckIgnored, tsUs, map[string]any{"ack": ackSeq, "reason": reason})
}

func (d *Dispatcher) OnTxNakReceived(tsUs uint64, lossCount uint32) {
	d.emit(EventTxNakReceived, tsUs, map[string]any{"loss_count": lossCount})
}

func (d *Dispatcher) OnRxDataReceived(tsUs uint64, seq uint32) {
	d.emit(EventRxDataReceived, tsUs, map[string]any{"seq": seq})
}

func (d *Dispatcher) OnRxGapDetected(tsUs uint64, firstMissing, until uint32) {
	d.emit(EventRxGapDetected, tsUs, map[string]any{"first_missing": firstMissing, "until": until})
}

func (d *Dispatcher) OnRxImmediateNakSent(tsUs uint64, missingCount uint32) {
	d.emit(EventRxImmediateNakSent, tsUs, map[string]any{"missing_count": missingCount})
}

func (d *Dispatcher) OnRxAckSent(tsUs uint64, ackSeq uint32) {
	d.emit(EventRxAckSent, tsUs, map[string]any{"ack": ackSeq})
}

type DebugSink struct {
	Enabled bool
}

func (s *DebugSink) OnEvent(e Event) {
	if !s.Enabled {
		return
	}
	if len(e.Data) == 0 {
		fmt.Printf("[%10dus] %s\n", e.TSUs, e.Kind)
		return
	}
	fmt.Printf("[%10dus] %s %v\n", e.TSUs, e.Kind, e.Data)
}
