package main

import (
	"fmt"
	"sort"
)

type StatsSink struct {
	counts                         map[EventKind]uint64
	packetDropsByType              map[string]uint64
	packetDropsByReason            map[string]uint64
	packetDropsByTypeReason        map[string]uint64
	packetDeliveredByType          map[string]uint64
	packetDeliveredByDirection     map[string]uint64
	packetDeliveredByTypeDirection map[string]uint64
	reordersByDirection            map[string]uint64
	reordersByTypePair             map[string]uint64
}

func NewStatsSink() *StatsSink {
	return &StatsSink{
		counts:                         make(map[EventKind]uint64),
		packetDropsByType:              make(map[string]uint64),
		packetDropsByReason:            make(map[string]uint64),
		packetDropsByTypeReason:        make(map[string]uint64),
		packetDeliveredByType:          make(map[string]uint64),
		packetDeliveredByDirection:     make(map[string]uint64),
		packetDeliveredByTypeDirection: make(map[string]uint64),
		reordersByDirection:            make(map[string]uint64),
		reordersByTypePair:             make(map[string]uint64),
	}
}

func (s *StatsSink) OnEvent(e Event) {
	s.counts[e.Kind]++
	switch e.Kind {
	case EventPacketDropped:
		pktType, _ := e.Data["type"].(string)
		reason, _ := e.Data["reason"].(string)
		if pktType == "" {
			pktType = "UNKNOWN"
		}
		if reason == "" {
			reason = "unknown"
		}
		s.packetDropsByType[pktType]++
		s.packetDropsByReason[reason]++
		s.packetDropsByTypeReason[pktType+"/"+reason]++
	case EventPacketDelivered:
		pktType, _ := e.Data["type"].(string)
		from, _ := e.Data["from"].(string)
		to, _ := e.Data["to"].(string)
		if pktType == "" {
			pktType = "UNKNOWN"
		}
		direction := from + "->" + to
		if direction == "->" {
			direction = "unknown"
		}
		s.packetDeliveredByType[pktType]++
		s.packetDeliveredByDirection[direction]++
		s.packetDeliveredByTypeDirection[pktType+"/"+direction]++
	case EventPacketReordered:
		direction, _ := e.Data["direction"].(string)
		firstType, _ := e.Data["first_type"].(string)
		secondType, _ := e.Data["second_type"].(string)
		if direction == "" {
			direction = "unknown"
		}
		if firstType == "" {
			firstType = "UNKNOWN"
		}
		if secondType == "" {
			secondType = "UNKNOWN"
		}
		s.reordersByDirection[direction]++
		s.reordersByTypePair[firstType+"<->"+secondType]++
	}
}

func printStringCounts(title string, counts map[string]uint64) {
	if len(counts) == 0 {
		fmt.Printf("%s none\n", title)
		return
	}
	fmt.Println(title)
	keys := make([]string, 0, len(counts))
	for key := range counts {
		keys = append(keys, key)
	}
	sort.Strings(keys)
	for _, key := range keys {
		value := counts[key]
		fmt.Printf("  %-24s %d\n", key+":", value)
	}
}

func (s *StatsSink) PrintSummary() {
	fmt.Println()
	fmt.Println("=== Simulation Stats ===")
	keys := []EventKind{
		EventTickStart,
		EventPacketQueued,
		EventPacketReordered,
		EventPacketDropped,
		EventPacketDelivered,
		EventTxSendData,
		EventTxSendRetransmit,
		EventTxTimeout,
		EventTxAckAccepted,
		EventTxAckIgnored,
		EventTxNakReceived,
		EventRxDataReceived,
		EventRxGapDetected,
		EventRxImmediateNakSent,
		EventRxAckSent,
	}
	for _, k := range keys {
		fmt.Printf("%-22s %d\n", string(k)+":", s.counts[k])
	}

	fmt.Println("\n=== Pipe Detail ===")
	printStringCounts("Delivered By Type", s.packetDeliveredByType)
	printStringCounts("Delivered By Direction", s.packetDeliveredByDirection)
	printStringCounts("Delivered By Type/Direction", s.packetDeliveredByTypeDirection)
	printStringCounts("Drops By Type", s.packetDropsByType)
	printStringCounts("Drops By Reason", s.packetDropsByReason)
	printStringCounts("Drops By Type/Reason", s.packetDropsByTypeReason)
	printStringCounts("Reorders By Direction", s.reordersByDirection)
	printStringCounts("Reorders By Type Pair", s.reordersByTypePair)
}
