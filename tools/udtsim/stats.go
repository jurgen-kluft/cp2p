package main

import (
	"fmt"
	"image"
	"image/color"
	"image/png"
	"os"
	"sort"
)

type SeqTimestamp struct {
	TSUs uint64
	Seq  uint32
}

type CountTimestamp struct {
	TSUs  uint64
	Count uint32
}

type TimelineData struct {
	SenderDataSent        []SeqTimestamp
	SenderMissingDataSent []SeqTimestamp
	SenderAckReceived     []SeqTimestamp
	SenderNakReceived     []CountTimestamp
	ReceiverDataReceived  []SeqTimestamp
	ReceiverAckSent       []SeqTimestamp
	ReceiverNakSent       []CountTimestamp
}

type StatsSink struct {
	counts                         map[EventKind]uint64
	txAckIgnoredByReason           map[string]uint64
	txNakLossCountSum              uint64
	txNakLossCountMax              uint32
	rxGapWidthSum                  uint64
	rxGapWidthMax                  uint32
	ccSampleCount                  uint64
	ccCwndSum                      uint64
	ccCwndMin                      uint32
	ccCwndMax                      uint32
	ccInFlightSum                  uint64
	ccInFlightMax                  uint32
	ccBudgetSum                    uint64
	ccBudgetMin                    uint32
	ccBudgetMax                    uint32
	ccPacingUsSum                  uint64
	ccPacingUsMin                  uint64
	ccPacingUsMax                  uint64
	ccSlowStartTicks               uint64
	ccBlockedCount                 uint64
	flowSampleCount                uint64
	flowWindowBytesSum             uint64
	flowWindowBytesMin             uint32
	flowWindowBytesMax             uint32
	flowInFlightBytesSum           uint64
	flowInFlightBytesMax           uint32
	flowBudgetBytesSum             uint64
	flowBudgetBytesMin             uint32
	flowBudgetBytesMax             uint32
	flowBlockedCount               uint64
	flowWindowUpdateCount          uint64
	packetDropsByType              map[string]uint64
	packetDropsByReason            map[string]uint64
	packetDropsByTypeReason        map[string]uint64
	packetDeliveredByType          map[string]uint64
	packetDeliveredByDirection     map[string]uint64
	packetDeliveredByTypeDirection map[string]uint64
	reordersByDirection            map[string]uint64
	reordersByTypePair             map[string]uint64
	nakUrgencySum                  uint64
	nakUrgencyMax                  uint32
	nakUrgencyMin                  uint32
	nakIntervalSumUs               uint64
	nakIntervalMaxUs               uint64
	nakIntervalMinUs               uint64
	nakMissingSum                  uint64
	nakMissingMax                  uint32
	nakWindowCount                 uint64
	nakWindowUrgencySum            uint64
	nakWindowUrgencyMax            uint32
	nakWindowUrgencyMin            uint32
	nakWindowRecvSum               uint64
	nakWindowMissingSum            uint64
	senderDataSent                 []SeqTimestamp
	senderMissingDataSent          []SeqTimestamp
	senderAckReceived              []SeqTimestamp
	senderNakReceived              []CountTimestamp
	receiverDataReceived           []SeqTimestamp
	receiverAckSent                []SeqTimestamp
	receiverNakSent                []CountTimestamp
	ccBlockedTimestamps            []uint64
	flowBlockedTimestamps          []uint64
	flowWindowLeftSnapshots        []TimelineValueSnapshot
}

type TimelineValueSnapshot struct {
	TSUs  uint64
	Value uint32
}

func NewStatsSink() *StatsSink {
	return &StatsSink{
		counts:                         make(map[EventKind]uint64),
		txAckIgnoredByReason:           make(map[string]uint64),
		ccCwndMin:                      ^uint32(0),
		ccBudgetMin:                    ^uint32(0),
		ccPacingUsMin:                  ^uint64(0),
		flowWindowBytesMin:             ^uint32(0),
		flowBudgetBytesMin:             ^uint32(0),
		packetDropsByType:              make(map[string]uint64),
		packetDropsByReason:            make(map[string]uint64),
		packetDropsByTypeReason:        make(map[string]uint64),
		packetDeliveredByType:          make(map[string]uint64),
		packetDeliveredByDirection:     make(map[string]uint64),
		packetDeliveredByTypeDirection: make(map[string]uint64),
		reordersByDirection:            make(map[string]uint64),
		reordersByTypePair:             make(map[string]uint64),
		nakUrgencyMin:                  ^uint32(0),
		nakIntervalMinUs:               ^uint64(0),
		nakWindowUrgencyMin:            ^uint32(0),
	}
}

func (s *StatsSink) OnEvent(e Event) {
	s.counts[e.Kind]++
	switch e.Kind {
	case EventTxSendData:
		seq, _ := e.Data["seq"].(uint32)
		s.senderDataSent = append(s.senderDataSent, SeqTimestamp{TSUs: e.TSUs, Seq: seq})
	case EventTxSendRetransmit:
		seq, _ := e.Data["seq"].(uint32)
		s.senderMissingDataSent = append(s.senderMissingDataSent, SeqTimestamp{TSUs: e.TSUs, Seq: seq})
	case EventTxAckAccepted:
		ack, _ := e.Data["ack"].(uint32)
		s.senderAckReceived = append(s.senderAckReceived, SeqTimestamp{TSUs: e.TSUs, Seq: ack})
	case EventTxAckIgnored:
		ack, _ := e.Data["ack"].(uint32)
		s.senderAckReceived = append(s.senderAckReceived, SeqTimestamp{TSUs: e.TSUs, Seq: ack})
		reason, _ := e.Data["reason"].(string)
		if reason == "" {
			reason = "unknown"
		}
		s.txAckIgnoredByReason[reason]++
	case EventTxNakReceived:
		lossCount, _ := e.Data["loss_count"].(uint32)
		s.senderNakReceived = append(s.senderNakReceived, CountTimestamp{TSUs: e.TSUs, Count: lossCount})
		s.txNakLossCountSum += uint64(lossCount)
		if lossCount > s.txNakLossCountMax {
			s.txNakLossCountMax = lossCount
		}
	case EventRxDataReceived:
		seq, _ := e.Data["seq"].(uint32)
		s.receiverDataReceived = append(s.receiverDataReceived, SeqTimestamp{TSUs: e.TSUs, Seq: seq})
	case EventRxAckSent:
		ack, _ := e.Data["ack"].(uint32)
		s.receiverAckSent = append(s.receiverAckSent, SeqTimestamp{TSUs: e.TSUs, Seq: ack})
	case EventRxNakSent:
		missingCount, _ := e.Data["missing_count"].(uint32)
		s.receiverNakSent = append(s.receiverNakSent, CountTimestamp{TSUs: e.TSUs, Count: missingCount})
		urgency, _ := e.Data["urgency"].(uint32)
		intervalUs, _ := e.Data["interval_us"].(uint64)
		s.nakUrgencySum += uint64(urgency)
		if urgency > s.nakUrgencyMax {
			s.nakUrgencyMax = urgency
		}
		if urgency < s.nakUrgencyMin {
			s.nakUrgencyMin = urgency
		}
		s.nakIntervalSumUs += intervalUs
		if intervalUs > s.nakIntervalMaxUs {
			s.nakIntervalMaxUs = intervalUs
		}
		if intervalUs < s.nakIntervalMinUs {
			s.nakIntervalMinUs = intervalUs
		}
		s.nakMissingSum += uint64(missingCount)
		if missingCount > s.nakMissingMax {
			s.nakMissingMax = missingCount
		}
	case EventTxCCSample:
		cwnd, _ := e.Data["cwnd"].(uint32)
		inFlight, _ := e.Data["in_flight"].(uint32)
		budget, _ := e.Data["budget"].(uint32)
		pacingUs, _ := e.Data["pacing_us"].(uint64)
		slowStart, _ := e.Data["slow_start"].(bool)
		s.ccSampleCount++
		s.ccCwndSum += uint64(cwnd)
		if cwnd < s.ccCwndMin {
			s.ccCwndMin = cwnd
		}
		if cwnd > s.ccCwndMax {
			s.ccCwndMax = cwnd
		}
		s.ccInFlightSum += uint64(inFlight)
		if inFlight > s.ccInFlightMax {
			s.ccInFlightMax = inFlight
		}
		s.ccBudgetSum += uint64(budget)
		if budget < s.ccBudgetMin {
			s.ccBudgetMin = budget
		}
		if budget > s.ccBudgetMax {
			s.ccBudgetMax = budget
		}
		s.ccPacingUsSum += pacingUs
		if pacingUs < s.ccPacingUsMin {
			s.ccPacingUsMin = pacingUs
		}
		if pacingUs > s.ccPacingUsMax {
			s.ccPacingUsMax = pacingUs
		}
		if slowStart {
			s.ccSlowStartTicks++
		}
	case EventTxCCBlocked:
		s.ccBlockedCount++
		s.ccBlockedTimestamps = append(s.ccBlockedTimestamps, e.TSUs)
	case EventTxFlowSample:
		flowWindowBytes, _ := e.Data["flow_window_bytes"].(uint32)
		inFlightBytes, _ := e.Data["in_flight_bytes"].(uint32)
		flowBudgetBytes, _ := e.Data["flow_budget_bytes"].(uint32)
		s.flowSampleCount++
		s.flowWindowBytesSum += uint64(flowWindowBytes)
		if flowWindowBytes < s.flowWindowBytesMin {
			s.flowWindowBytesMin = flowWindowBytes
		}
		if flowWindowBytes > s.flowWindowBytesMax {
			s.flowWindowBytesMax = flowWindowBytes
		}
		s.flowInFlightBytesSum += uint64(inFlightBytes)
		if inFlightBytes > s.flowInFlightBytesMax {
			s.flowInFlightBytesMax = inFlightBytes
		}
		s.flowBudgetBytesSum += uint64(flowBudgetBytes)
		if flowBudgetBytes < s.flowBudgetBytesMin {
			s.flowBudgetBytesMin = flowBudgetBytes
		}
		if flowBudgetBytes > s.flowBudgetBytesMax {
			s.flowBudgetBytesMax = flowBudgetBytes
		}
		// Track flow window left over time
		s.flowWindowLeftSnapshots = append(s.flowWindowLeftSnapshots, TimelineValueSnapshot{TSUs: e.TSUs, Value: flowBudgetBytes})
	case EventTxFlowBlocked:
		s.flowBlockedCount++
		s.flowBlockedTimestamps = append(s.flowBlockedTimestamps, e.TSUs)
	case EventTxFlowWindowUpdate:
		s.flowWindowUpdateCount++

	case EventRxGapDetected:
		firstMissing, _ := e.Data["first_missing"].(uint32)
		until, _ := e.Data["until"].(uint32)
		if until > firstMissing {
			gapWidth := until - firstMissing
			s.rxGapWidthSum += uint64(gapWidth)
			if gapWidth > s.rxGapWidthMax {
				s.rxGapWidthMax = gapWidth
			}
		}
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

	case EventRxNakWindowClosed:
		urgency, _ := e.Data["urgency"].(uint32)
		recvCount, _ := e.Data["recv_count"].(uint32)
		missingCount, _ := e.Data["missing_count"].(uint32)
		s.nakWindowCount++
		s.nakWindowUrgencySum += uint64(urgency)
		if urgency > s.nakWindowUrgencyMax {
			s.nakWindowUrgencyMax = urgency
		}
		if urgency < s.nakWindowUrgencyMin {
			s.nakWindowUrgencyMin = urgency
		}
		s.nakWindowRecvSum += uint64(recvCount)
		s.nakWindowMissingSum += uint64(missingCount)
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
		EventTxCCSample,
		EventTxCCBlocked,
		EventTxFlowSample,
		EventTxFlowBlocked,
		EventTxFlowWindowUpdate,
		EventTxTimeout,
		EventTxAckAccepted,
		EventTxAckIgnored,
		EventTxNakReceived,
		EventRxDataReceived,
		EventRxGapDetected,
		EventRxImmediateNakSent,
		EventRxNakSent,
		EventRxNakWindowClosed,
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

	if s.counts[EventTxAckIgnored] > 0 || s.counts[EventTxNakReceived] > 0 || s.counts[EventRxGapDetected] > 0 {
		fmt.Println("\n=== Protocol Detail ===")
		if s.counts[EventTxAckIgnored] > 0 {
			printStringCounts("Tx Ack Ignored By Reason", s.txAckIgnoredByReason)
		}
		if s.counts[EventTxNakReceived] > 0 {
			n := s.counts[EventTxNakReceived]
			fmt.Printf("  %-24s %d\n", "Tx NAK LossCount Avg:", s.txNakLossCountSum/uint64(n))
			fmt.Printf("  %-24s %d\n", "Tx NAK LossCount Max:", s.txNakLossCountMax)
		}
		if s.counts[EventRxGapDetected] > 0 {
			n := s.counts[EventRxGapDetected]
			fmt.Printf("  %-24s %d\n", "Rx Gap Width Avg:", s.rxGapWidthSum/uint64(n))
			fmt.Printf("  %-24s %d\n", "Rx Gap Width Max:", s.rxGapWidthMax)
		}
	}

	if s.ccSampleCount > 0 {
		fmt.Println("\n=== Congestion Control Detail ===")
		n := s.ccSampleCount
		fmt.Printf("  %-24s %d\n", "Samples:", n)
		fmt.Printf("  %-24s %d\n", "cwnd Avg:", s.ccCwndSum/n)
		fmt.Printf("  %-24s %d\n", "cwnd Min:", s.ccCwndMin)
		fmt.Printf("  %-24s %d\n", "cwnd Max:", s.ccCwndMax)
		fmt.Printf("  %-24s %d\n", "InFlight Avg:", s.ccInFlightSum/n)
		fmt.Printf("  %-24s %d\n", "InFlight Max:", s.ccInFlightMax)
		fmt.Printf("  %-24s %d\n", "Budget Avg:", s.ccBudgetSum/n)
		fmt.Printf("  %-24s %d\n", "Budget Min:", s.ccBudgetMin)
		fmt.Printf("  %-24s %d\n", "Budget Max:", s.ccBudgetMax)
		fmt.Printf("  %-24s %d\n", "Pacing Avg(us):", s.ccPacingUsSum/n)
		fmt.Printf("  %-24s %d\n", "Pacing Min(us):", s.ccPacingUsMin)
		fmt.Printf("  %-24s %d\n", "Pacing Max(us):", s.ccPacingUsMax)
		fmt.Printf("  %-24s %d\n", "SlowStart Ticks:", s.ccSlowStartTicks)
		fmt.Printf("  %-24s %.2f%%\n", "SlowStart Ratio:", float64(s.ccSlowStartTicks)*100/float64(n))
		fmt.Printf("  %-24s %d\n", "CC Blocked Ticks:", s.ccBlockedCount)
		fmt.Printf("  %-24s %.2f%%\n", "CC Blocked Ratio:", float64(s.ccBlockedCount)*100/float64(n))
	}

	if s.flowSampleCount > 0 {
		fmt.Println("\n=== Flow Control Detail ===")
		n := s.flowSampleCount
		fmt.Printf("  %-24s %d\n", "Samples:", n)
		fmt.Printf("  %-24s %d\n", "FlowWin Avg(bytes):", s.flowWindowBytesSum/n)
		fmt.Printf("  %-24s %d\n", "FlowWin Min(bytes):", s.flowWindowBytesMin)
		fmt.Printf("  %-24s %d\n", "FlowWin Max(bytes):", s.flowWindowBytesMax)
		fmt.Printf("  %-24s %d\n", "InFlight Avg(bytes):", s.flowInFlightBytesSum/n)
		fmt.Printf("  %-24s %d\n", "InFlight Max(bytes):", s.flowInFlightBytesMax)
		fmt.Printf("  %-24s %d\n", "FlowBudget Avg(bytes):", s.flowBudgetBytesSum/n)
		fmt.Printf("  %-24s %d\n", "FlowBudget Min(bytes):", s.flowBudgetBytesMin)
		fmt.Printf("  %-24s %d\n", "FlowBudget Max(bytes):", s.flowBudgetBytesMax)
		fmt.Printf("  %-24s %d\n", "Flow Blocked Ticks:", s.flowBlockedCount)
		fmt.Printf("  %-24s %.2f%%\n", "Flow Blocked Ratio:", float64(s.flowBlockedCount)*100/float64(n))
		fmt.Printf("  %-24s %d\n", "FlowWin Updates:", s.flowWindowUpdateCount)
	}

	if s.counts[EventRxNakSent] > 0 || s.nakWindowCount > 0 {
		fmt.Println("\n=== RX NAK Window Detail ===")
		if s.counts[EventRxNakSent] > 0 {
			n := s.counts[EventRxNakSent]
			fmt.Printf("  %-24s %d\n", "NAKs Sent:", n)
			fmt.Printf("  %-24s %d\n", "NAK Missing Avg:", s.nakMissingSum/uint64(n))
			fmt.Printf("  %-24s %d\n", "NAK Missing Max:", s.nakMissingMax)
			fmt.Printf("  %-24s %d\n", "NAK Urgency Avg:", s.nakUrgencySum/uint64(n))
			fmt.Printf("  %-24s %d\n", "NAK Urgency Min:", s.nakUrgencyMin)
			fmt.Printf("  %-24s %d\n", "NAK Urgency Max:", s.nakUrgencyMax)
			fmt.Printf("  %-24s %d\n", "NAK Interval Avg(us):", s.nakIntervalSumUs/uint64(n))
			fmt.Printf("  %-24s %d\n", "NAK Interval Min(us):", s.nakIntervalMinUs)
			fmt.Printf("  %-24s %d\n", "NAK Interval Max(us):", s.nakIntervalMaxUs)
		}
		if s.nakWindowCount > 0 {
			w := s.nakWindowCount
			fmt.Printf("  %-24s %d\n", "Windows Closed:", w)
			fmt.Printf("  %-24s %d\n", "Window Recv Avg:", s.nakWindowRecvSum/w)
			fmt.Printf("  %-24s %d\n", "Window Missing Avg:", s.nakWindowMissingSum/w)
			fmt.Printf("  %-24s %d\n", "Window Urgency Avg:", s.nakWindowUrgencySum/w)
			fmt.Printf("  %-24s %d\n", "Window Urgency Min:", s.nakWindowUrgencyMin)
			fmt.Printf("  %-24s %d\n", "Window Urgency Max:", s.nakWindowUrgencyMax)
		}
	}
}

func (s *StatsSink) TimelineData() TimelineData {
	return TimelineData{
		SenderDataSent:        append([]SeqTimestamp(nil), s.senderDataSent...),
		SenderMissingDataSent: append([]SeqTimestamp(nil), s.senderMissingDataSent...),
		SenderAckReceived:     append([]SeqTimestamp(nil), s.senderAckReceived...),
		SenderNakReceived:     append([]CountTimestamp(nil), s.senderNakReceived...),
		ReceiverDataReceived:  append([]SeqTimestamp(nil), s.receiverDataReceived...),
		ReceiverAckSent:       append([]SeqTimestamp(nil), s.receiverAckSent...),
		ReceiverNakSent:       append([]CountTimestamp(nil), s.receiverNakSent...),
	}
}

func (s *StatsSink) ExportTimelinePNG(filePath string, simDurationUs uint64) error {
	const (
		width     = 3840
		height    = 2160
		leftPad   = 260
		rightPad  = 820
		topPad    = 90
		bottomPad = 170
		panelGap  = 130
		fontScale = 3
		lineW     = 4
		circR     = 8
	)

	// Top panel: Data / Receiver. Bottom panel: ACK / NAK / Flow.
	colorSenderData := color.RGBA{20, 108, 148, 255}
	colorSenderRetrans := color.RGBA{220, 20, 60, 255}
	colorReceiverData := color.RGBA{255, 140, 0, 255}
	colorAckProgress := color.RGBA{0, 128, 0, 255}
	colorSenderAckEvents := color.RGBA{46, 139, 87, 255}
	colorReceiverAckEvents := color.RGBA{30, 144, 255, 255}
	colorSenderNak := color.RGBA{138, 43, 226, 255}
	colorReceiverNak := color.RGBA{199, 21, 133, 255}
	colorFlowNorm := color.RGBA{100, 100, 100, 255}
	ccColor := color.RGBA{220, 20, 60, 255}
	flowColor := color.RGBA{30, 144, 255, 255}
	black := color.RGBA{0, 0, 0, 255}
	gridColor := color.RGBA{200, 206, 214, 255}

	img := image.NewRGBA(image.Rect(0, 0, width, height))
	bg := color.RGBA{245, 247, 250, 255}
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			img.Set(x, y, bg)
		}
	}

	plotW := width - leftPad - rightPad
	panelH := (height - topPad - bottomPad - panelGap) / 2
	topPanelY := topPad
	bottomPanelY := topPad + panelH + panelGap
	plotLeft := leftPad
	plotRight := leftPad + plotW
	charH := 7 * fontScale

	type TV struct {
		ts    uint64
		count uint32
	}
	makeCounter := func(events []SeqTimestamp) []TV {
		out := make([]TV, len(events))
		for i, ev := range events {
			out[i] = TV{ts: ev.TSUs, count: uint32(i + 1)}
		}
		return out
	}
	makeCounterFromCount := func(events []CountTimestamp) []TV {
		out := make([]TV, len(events))
		sum := uint32(0)
		for i, ev := range events {
			sum += ev.Count
			out[i] = TV{ts: ev.TSUs, count: sum}
		}
		return out
	}
	makeAckProgress := func(events []SeqTimestamp) []TV {
		out := make([]TV, len(events))
		highest := uint32(0)
		for i, ev := range events {
			if ev.Seq > highest {
				highest = ev.Seq
			}
			out[i] = TV{ts: ev.TSUs, count: highest}
		}
		return out
	}

	senderData := makeCounter(s.senderDataSent)
	senderRetrans := makeCounter(s.senderMissingDataSent)
	receiverData := makeCounter(s.receiverDataReceived)
	senderAckEvents := makeCounter(s.senderAckReceived)
	receiverAckEvents := makeCounter(s.receiverAckSent)
	ackProgress := makeAckProgress(s.senderAckReceived)
	senderNak := makeCounterFromCount(s.senderNakReceived)
	receiverNak := makeCounterFromCount(s.receiverNakSent)
	flowAbs := make([]TV, len(s.flowWindowLeftSnapshots))
	for i, snap := range s.flowWindowLeftSnapshots {
		flowAbs[i] = TV{ts: snap.TSUs, count: snap.Value}
	}

	maxSeries := func(seriesList ...[]TV) uint32 {
		maxV := uint32(1)
		for _, series := range seriesList {
			if len(series) > 0 {
				if v := series[len(series)-1].count; v > maxV {
					maxV = v
				}
			}
		}
		return maxV
	}

	maxFlowAbs := maxSeries(flowAbs)

	maxTop := maxSeries(senderData, senderRetrans, receiverData)
	maxBottomLeft := maxSeries(ackProgress, senderNak, receiverNak)
	maxBottomRight := maxSeries(senderAckEvents, receiverAckEvents)

	mapX := func(ts uint64) int {
		if simDurationUs == 0 {
			return plotLeft
		}
		x := plotLeft + int(float64(ts)/float64(simDurationUs)*float64(plotW))
		if x < plotLeft {
			return plotLeft
		}
		if x > plotRight {
			return plotRight
		}
		return x
	}
	mapY := func(panelY int, maxV uint32, val uint32) int {
		y := panelY + panelH - int(float64(val)/float64(maxV)*float64(panelH))
		if y < panelY {
			return panelY
		}
		if y > panelY+panelH {
			return panelY + panelH
		}
		return y
	}
	mapYRect := func(y0, y1 int, maxV uint32, val uint32) int {
		h := y1 - y0
		y := y1 - int(float64(val)/float64(maxV)*float64(h))
		if y < y0 {
			return y0
		}
		if y > y1 {
			return y1
		}
		return y
	}

	drawPanelGrid := func(panelY int) {
		for i := 0; i <= 10; i++ {
			y := panelY + panelH*i/10
			drawHLine(img, plotLeft, plotRight, y, gridColor)
		}
		for i := 0; i <= 5; i++ {
			x := plotLeft + plotW*i/5
			drawVLine_Thin(img, x, panelY, panelY+panelH, gridColor)
		}
	}

	drawSeries := func(data []TV, panelY int, maxV uint32, c color.RGBA) {
		if len(data) == 0 {
			return
		}
		px, py := mapX(data[0].ts), mapY(panelY, maxV, data[0].count)
		for _, v := range data[1:] {
			cx, cy := mapX(v.ts), mapY(panelY, maxV, v.count)
			drawLineSegment(img, px, py, cx, cy, c, lineW)
			px, py = cx, cy
		}
		if px < plotRight {
			drawLineSegment(img, px, py, plotRight, py, c, lineW)
		}
	}

	drawLeftTicks := func(panelY int, maxV uint32, axisLabel string) {
		for i := 0; i <= 10; i++ {
			y := panelY + panelH*i/10
			val := maxV * uint32(10-i) / 10
			drawHLine(img, plotLeft-12, plotLeft, y, black)
			label := fmt.Sprintf("%d", val)
			lx := plotLeft - textWidth(label, fontScale) - 16
			ly := y - charH/2
			drawTextScaled(img, lx, ly, label, fontScale, black)
		}
		for i, ch := range axisLabel {
			lx := 6
			ly := panelY + panelH/2 - len(axisLabel)*charH/2 + i*charH
			drawTextScaled(img, lx, ly, string(ch), fontScale, black)
		}
	}

	drawRightTicks := func(panelY int, maxV uint32, axisLabel string) {
		drawVLine(img, plotRight, panelY, panelY+panelH, black)
		for i := 0; i <= 10; i++ {
			y := panelY + panelH*i/10
			val := maxV * uint32(10-i) / 10
			drawHLine(img, plotRight, plotRight+12, y, black)
			label := fmt.Sprintf("%d", val)
			lx := plotRight + 18
			ly := y - charH/2
			drawTextScaled(img, lx, ly, label, fontScale, black)
		}
		axX := plotRight + 180
		for i, ch := range axisLabel {
			ly := panelY + panelH/2 - len(axisLabel)*charH/2 + i*charH
			drawTextScaled(img, axX, ly, string(ch), fontScale, black)
		}
	}

	// Draw panel grids.
	drawPanelGrid(topPanelY)
	drawPanelGrid(bottomPanelY)

	// Top panel lines: data transfer perspective.
	drawSeries(senderData, topPanelY, maxTop, colorSenderData)
	drawSeries(senderRetrans, topPanelY, maxTop, colorSenderRetrans)
	drawSeries(receiverData, topPanelY, maxTop, colorReceiverData)

	// Bottom panel left-axis lines: progress + NAK.
	drawSeries(ackProgress, bottomPanelY, maxBottomLeft, colorAckProgress)
	drawSeries(senderNak, bottomPanelY, maxBottomLeft, colorSenderNak)
	drawSeries(receiverNak, bottomPanelY, maxBottomLeft, colorReceiverNak)

	// Bottom panel right-axis lines: ACK event counts.
	drawSeries(senderAckEvents, bottomPanelY, maxBottomRight, colorSenderAckEvents)
	drawSeries(receiverAckEvents, bottomPanelY, maxBottomRight, colorReceiverAckEvents)

	// Dedicated third-axis style for flow: inset with absolute-byte Y scale.
	insetX0 := plotLeft + (plotW*67)/100
	insetX1 := plotRight - 24
	insetY0 := bottomPanelY + 24
	insetY1 := bottomPanelY + (panelH*58)/100
	drawFilledRect(img, insetX0, insetY0, insetX1, insetY1, color.RGBA{238, 241, 245, 255})
	drawHLine(img, insetX0, insetX1, insetY0, black)
	drawHLine(img, insetX0, insetX1, insetY1, black)
	drawVLine(img, insetX0, insetY0, insetY1, black)
	drawVLine(img, insetX1, insetY0, insetY1, black)
	for i := 1; i < 4; i++ {
		y := insetY0 + (insetY1-insetY0)*i/4
		drawHLine(img, insetX0, insetX1, y, color.RGBA{210, 216, 224, 255})
	}
	if len(flowAbs) > 0 {
		insetMapX := func(ts uint64) int {
			if simDurationUs == 0 {
				return insetX0
			}
			x := insetX0 + int(float64(ts)/float64(simDurationUs)*float64(insetX1-insetX0))
			if x < insetX0 {
				return insetX0
			}
			if x > insetX1 {
				return insetX1
			}
			return x
		}
		px, py := insetMapX(flowAbs[0].ts), mapYRect(insetY0, insetY1, maxFlowAbs, flowAbs[0].count)
		for _, v := range flowAbs[1:] {
			cx, cy := insetMapX(v.ts), mapYRect(insetY0, insetY1, maxFlowAbs, v.count)
			drawLineSegment(img, px, py, cx, cy, colorFlowNorm, 3)
			px, py = cx, cy
		}
		if px < insetX1 {
			drawLineSegment(img, px, py, insetX1, py, colorFlowNorm, 3)
		}
	}
	drawTextScaled(img, insetX0+10, insetY0+8, "Flow Window Left (bytes)", fontScale, black)
	midFlow := maxFlowAbs / 2
	drawTextScaled(img, insetX0+10, insetY1-charH-8, "0", fontScale, black)
	drawTextScaled(img, insetX0+10, insetY0+(insetY1-insetY0)/2-charH/2, fmt.Sprintf("%d", midFlow), fontScale, black)
	drawTextScaled(img, insetX0+10, insetY0+10+charH, fmt.Sprintf("%d", maxFlowAbs), fontScale, black)

	// Blocked markers remain on the top panel.
	ccRow := topPanelY + circR + 6
	flowRow := topPanelY + 2*circR + 16
	for _, ts := range s.ccBlockedTimestamps {
		drawCircle(img, mapX(ts), ccRow, circR, ccColor)
	}
	for _, ts := range s.flowBlockedTimestamps {
		drawCircle(img, mapX(ts), flowRow, circR, flowColor)
	}

	// Axes for both panels.
	drawHLine(img, plotLeft, plotRight, topPanelY+panelH, black)
	drawVLine(img, plotLeft, topPanelY, topPanelY+panelH, black)
	drawHLine(img, plotLeft, plotRight, bottomPanelY+panelH, black)
	drawVLine(img, plotLeft, bottomPanelY, bottomPanelY+panelH, black)

	drawLeftTicks(topPanelY, maxTop, "Packets")
	drawLeftTicks(bottomPanelY, maxBottomLeft, "Progress")
	drawRightTicks(bottomPanelY, maxBottomRight, "ACK Events")

	// X-axis tick marks and labels on bottom panel.
	const xTicks = 10
	for i := 0; i <= xTicks; i++ {
		x := plotLeft + plotW*i/xTicks
		ts := simDurationUs * uint64(i) / xTicks
		drawVLine(img, x, bottomPanelY+panelH, bottomPanelY+panelH+14, black)
		label := fmt.Sprintf("%d", ts)
		lx := x - textWidth(label, fontScale)/2
		drawTextScaled(img, lx, bottomPanelY+panelH+18, label, fontScale, black)
	}
	xLabel := "Time (us)"
	drawTextScaled(img, plotLeft+plotW/2-textWidth(xLabel, fontScale)/2,
		bottomPanelY+panelH+18+charH+8, xLabel, fontScale, black)

	// Panel labels.
	drawTextScaled(img, plotLeft, topPanelY-charH-8, "Data / Receiver", fontScale, black)
	drawTextScaled(img, plotLeft, bottomPanelY-charH-8, "ACK / NAK / Flow", fontScale, black)

	type legendEntry struct {
		label string
		c     color.RGBA
	}
	entries := []legendEntry{
		{"Top: Sender Data", colorSenderData},
		{"Top: Sender Retrans", colorSenderRetrans},
		{"Top: Receiver Data Arrivals (all)", colorReceiverData},
		{"Bottom-L: ACK Progress (highest ack seq)", colorAckProgress},
		{"Bottom-R: Sender ACK events", colorSenderAckEvents},
		{"Bottom-R: Receiver ACK events", colorReceiverAckEvents},
		{"Bottom-L: Sender NAK", colorSenderNak},
		{"Bottom-L: Receiver NAK", colorReceiverNak},
		{"Inset: Flow Window Left (bytes)", colorFlowNorm},
		{"CC Blocked (red circles)", ccColor},
		{"Flow Blocked (blue circles)", flowColor},
	}
	legendX := plotRight + 40
	legendY := topPanelY + 10
	swW := 10 * fontScale
	swH := charH
	lineSpacing := charH + fontScale*2 + 4
	for i, e := range entries {
		y := legendY + i*lineSpacing
		drawFilledRect(img, legendX, y, legendX+swW, y+swH, e.c)
		drawTextScaled(img, legendX+swW+10, y, e.label, fontScale, black)
	}

	out, err := os.Create(filePath)
	if err != nil {
		return err
	}
	defer out.Close()
	return png.Encode(out, img)
}

func drawHLine(img *image.RGBA, x0, x1, y int, c color.RGBA) {
	if y < 0 || y >= img.Bounds().Dy() {
		return
	}
	if x0 > x1 {
		x0, x1 = x1, x0
	}
	if x0 < 0 {
		x0 = 0
	}
	if x1 >= img.Bounds().Dx() {
		x1 = img.Bounds().Dx() - 1
	}
	for x := x0; x <= x1; x++ {
		img.SetRGBA(x, y, c)
	}
}

func drawVLine(img *image.RGBA, x, y0, y1 int, c color.RGBA) {
	if x < 0 || x >= img.Bounds().Dx() {
		return
	}
	if y0 > y1 {
		y0, y1 = y1, y0
	}
	if y0 < 0 {
		y0 = 0
	}
	if y1 >= img.Bounds().Dy() {
		y1 = img.Bounds().Dy() - 1
	}
	for y := y0; y <= y1; y++ {
		img.SetRGBA(x, y, c)
	}
}

func drawVLine_Thin(img *image.RGBA, x, y0, y1 int, c color.RGBA) {
	drawVLine(img, x, y0, y1, color.RGBA{c.R, c.G, c.B, 80})
}

func drawLineSegment(img *image.RGBA, x0, y0, x1, y1 int, c color.RGBA, width int) {
	// Simple Bresenham line with thickness
	dx := abs(x1 - x0)
	dy := abs(y1 - y0)
	sx := 1
	if x0 > x1 {
		sx = -1
	}
	sy := 1
	if y0 > y1 {
		sy = -1
	}
	err := dx - dy

	x, y := x0, y0
	for {
		// Draw point with thickness
		for dx := -width / 2; dx <= width/2; dx++ {
			for dy := -width / 2; dy <= width/2; dy++ {
				px, py := x+dx, y+dy
				if px >= 0 && px < img.Bounds().Dx() && py >= 0 && py < img.Bounds().Dy() {
					img.SetRGBA(px, py, c)
				}
			}
		}

		if x == x1 && y == y1 {
			break
		}
		e2 := 2 * err
		if e2 > -dy {
			err -= dy
			x += sx
		}
		if e2 < dx {
			err += dx
			y += sy
		}
	}
}

func drawCircle(img *image.RGBA, centerX, centerY, radius int, c color.RGBA) {
	// Midpoint circle algorithm
	x := radius
	y := 0
	decisionParam := 3 - 2*radius

	for x >= y {
		// Octants
		plotPoints := func(cx, cy, px, py int) {
			for dx := -2; dx <= 2; dx++ {
				for dy := -2; dy <= 2; dy++ {
					px2, py2 := cx+px+dx, cy+py+dy
					if px2 >= 0 && px2 < img.Bounds().Dx() && py2 >= 0 && py2 < img.Bounds().Dy() {
						img.SetRGBA(px2, py2, c)
					}
				}
			}
		}

		plotPoints(centerX, centerY, x, y)
		plotPoints(centerX, centerY, -x, y)
		plotPoints(centerX, centerY, x, -y)
		plotPoints(centerX, centerY, -x, -y)
		plotPoints(centerX, centerY, y, x)
		plotPoints(centerX, centerY, -y, x)
		plotPoints(centerX, centerY, y, -x)
		plotPoints(centerX, centerY, -y, -x)

		if decisionParam < 0 {
			decisionParam = decisionParam + 4*y + 6
		} else {
			decisionParam = decisionParam + 4*(y-x) + 10
			x--
		}
		y++
	}
}

func drawFilledRect(img *image.RGBA, x0, y0, x1, y1 int, c color.RGBA) {
	if x0 > x1 {
		x0, x1 = x1, x0
	}
	if y0 > y1 {
		y0, y1 = y1, y0
	}
	if x0 < 0 {
		x0 = 0
	}
	if y0 < 0 {
		y0 = 0
	}
	if x1 >= img.Bounds().Dx() {
		x1 = img.Bounds().Dx() - 1
	}
	if y1 >= img.Bounds().Dy() {
		y1 = img.Bounds().Dy() - 1
	}
	for y := y0; y <= y1; y++ {
		for x := x0; x <= x1; x++ {
			img.SetRGBA(x, y, c)
		}
	}
}

func abs(x int) int {
	if x < 0 {
		return -x
	}
	return x
}
