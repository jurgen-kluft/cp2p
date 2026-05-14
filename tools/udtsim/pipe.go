package main

import (
	"math/rand"
	"sort"
)

const (
	defaultDataPacketBytes      = 1200
	defaultAckPacketBytes       = 64
	defaultNakPacketBaseBytes   = 64
	defaultNakMissingEntryBytes = 4
	defaultReorderWindowPackets = 4
)

type PacketType int

const (
	PktData PacketType = iota
	PktAck
	PktNak
)

func (t PacketType) String() string {
	switch t {
	case PktData:
		return "DATA"
	case PktAck:
		return "ACK"
	case PktNak:
		return "NAK"
	default:
		return "UNKNOWN"
	}
}

type Packet struct {
	Type       PacketType
	Seq        uint32
	AckSeq     uint32
	FlowWindow uint32
	Missing    []uint32
	SizeBytes  int
	From       string
	To         string
}

type queuedPacket struct {
	pkt         Packet
	sizeBytes   int
	direction   string
	deliverAtUs uint64
}

type PipeConfig struct {
	DropProb           float64
	ReorderProb        float64
	BaseLatUs          uint64
	JitterUs           int64
	MaxQueue           int
	MaxQueueBytes      int
	ForwardBitsPerSec  uint64
	ReverseBitsPerSec  uint64
	DataPacketBytes    int
	AckPacketBytes     int
	NakPacketBaseBytes int
	NakMissingBytes    int
	Seed               int64
}

type linkState struct {
	nextTxAvailableUs uint64
	queuedBytes       int
}

type MessagePipe struct {
	cfg     PipeConfig
	disp    *Dispatcher
	rnd     *rand.Rand
	queue   []queuedPacket
	forward linkState
	reverse linkState
}

func NewMessagePipe(cfg PipeConfig, disp *Dispatcher) *MessagePipe {
	m := &MessagePipe{cfg: cfg, disp: disp, rnd: rand.New(rand.NewSource(cfg.Seed))}
	m.queue = make([]queuedPacket, 0, cfg.MaxQueue)
	return m
}

func (p *MessagePipe) packetSizeBytes(pkt Packet) int {
	if pkt.SizeBytes > 0 {
		return pkt.SizeBytes
	}
	switch pkt.Type {
	case PktData:
		if p.cfg.DataPacketBytes > 0 {
			return p.cfg.DataPacketBytes
		}
		return defaultDataPacketBytes
	case PktAck:
		if p.cfg.AckPacketBytes > 0 {
			return p.cfg.AckPacketBytes
		}
		return defaultAckPacketBytes
	case PktNak:
		baseBytes := p.cfg.NakPacketBaseBytes
		if baseBytes <= 0 {
			baseBytes = defaultNakPacketBaseBytes
		}
		missingBytes := p.cfg.NakMissingBytes
		if missingBytes <= 0 {
			missingBytes = defaultNakMissingEntryBytes
		}
		return baseBytes + len(pkt.Missing)*missingBytes
	default:
		return defaultAckPacketBytes
	}
}

func (p *MessagePipe) directionForPacket(pkt Packet) string {
	return pkt.From + "->" + pkt.To
}

func (p *MessagePipe) linkForPacket(pkt Packet) *linkState {
	if pkt.From == "sender" && pkt.To == "receiver" {
		return &p.forward
	}
	return &p.reverse
}

func (p *MessagePipe) bandwidthForPacket(pkt Packet) uint64 {
	if pkt.From == "sender" && pkt.To == "receiver" {
		return p.cfg.ForwardBitsPerSec
	}
	return p.cfg.ReverseBitsPerSec
}

func serializationDelayUs(sizeBytes int, bitsPerSec uint64) uint64 {
	if sizeBytes <= 0 || bitsPerSec == 0 {
		return 0
	}
	bits := uint64(sizeBytes) * 8
	return (bits*1_000_000 + bitsPerSec - 1) / bitsPerSec
}

func (p *MessagePipe) Enqueue(nowUs uint64, pkt Packet) {
	if p.cfg.MaxQueue > 0 && len(p.queue) >= p.cfg.MaxQueue {
		p.disp.OnPacketDropped(nowUs, pkt.From, pkt.To, pkt.Type.String(), pkt.Seq, "queue-full")
		return
	}
	if p.rnd.Float64() < p.cfg.DropProb {
		p.disp.OnPacketDropped(nowUs, pkt.From, pkt.To, pkt.Type.String(), pkt.Seq, "drop-prob")
		return
	}
	sizeBytes := p.packetSizeBytes(pkt)
	link := p.linkForPacket(pkt)
	if p.cfg.MaxQueueBytes > 0 && link.queuedBytes+sizeBytes > p.cfg.MaxQueueBytes {
		p.disp.OnPacketDropped(nowUs, pkt.From, pkt.To, pkt.Type.String(), pkt.Seq, "queue-bytes-full")
		return
	}
	lat := int64(p.cfg.BaseLatUs)
	if p.cfg.JitterUs > 0 {
		j := p.rnd.Int63n(2*p.cfg.JitterUs+1) - p.cfg.JitterUs
		lat += j
		if lat < 0 {
			lat = 0
		}
	}
	bw := p.bandwidthForPacket(pkt)
	txStartUs := nowUs
	if link.nextTxAvailableUs > txStartUs {
		txStartUs = link.nextTxAvailableUs
	}
	serializeUs := serializationDelayUs(sizeBytes, bw)
	txFinishUs := txStartUs + serializeUs
	deliverAt := txFinishUs + uint64(lat)
	link.nextTxAvailableUs = txFinishUs
	link.queuedBytes += sizeBytes
	direction := p.directionForPacket(pkt)
	p.queue = append(p.queue, queuedPacket{pkt: pkt, sizeBytes: sizeBytes, direction: direction, deliverAtUs: deliverAt})
	p.disp.OnPacketQueued(nowUs, pkt.From, pkt.To, pkt.Type.String(), pkt.Seq, deliverAt)

	if len(p.queue) > 1 && p.rnd.Float64() < p.cfg.ReorderProb {
		i := len(p.queue) - 1
		window := defaultReorderWindowPackets
		if window > len(p.queue)-1 {
			window = len(p.queue) - 1
		}
		jMin := i - window
		j := jMin + p.rnd.Intn(window)
		first := p.queue[i]
		second := p.queue[j]
		// Reordering is modeled by swapping scheduled delivery timestamps.
		// This preserves queue membership while changing arrival order.
		p.queue[i].deliverAtUs, p.queue[j].deliverAtUs = p.queue[j].deliverAtUs, p.queue[i].deliverAtUs
		p.disp.OnPacketReordered(nowUs, first.direction, first.pkt.Type.String(), first.pkt.Seq, second.pkt.Type.String(), second.pkt.Seq)
	}
}

func (p *MessagePipe) Tick(nowUs uint64, deliverFn func(nowUs uint64, pkt Packet)) {
	if len(p.queue) == 0 {
		return
	}
	sort.SliceStable(p.queue, func(i, j int) bool {
		return p.queue[i].deliverAtUs < p.queue[j].deliverAtUs
	})
	outIdx := 0
	for i := 0; i < len(p.queue); i++ {
		q := p.queue[i]
		if q.deliverAtUs <= nowUs {
			link := p.linkForPacket(q.pkt)
			link.queuedBytes -= q.sizeBytes
			if link.queuedBytes < 0 {
				link.queuedBytes = 0
			}
			p.disp.OnPacketDelivered(q.deliverAtUs, q.pkt.From, q.pkt.To, q.pkt.Type.String(), q.pkt.Seq)
			deliverFn(q.deliverAtUs, q.pkt)
		} else {
			p.queue[outIdx] = q
			outIdx++
		}
	}
	p.queue = p.queue[:outIdx]
}
