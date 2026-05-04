package main

import (
	"flag"
	"fmt"
	"strings"
	"time"
)

func formatUint64Underscore(value uint64) string {
	text := fmt.Sprintf("%d", value)
	if len(text) <= 3 {
		return text
	}
	var parts []string
	for len(text) > 3 {
		parts = append([]string{text[len(text)-3:]}, parts...)
		text = text[:len(text)-3]
	}
	parts = append([]string{text}, parts...)
	return strings.Join(parts, "_")
}

func main() {
	var (
		reportedSimUs   = flag.Uint64("sim-us", 5_000_000, "deprecated compatibility flag; simulation now stops only when delivery completes")
		maxTicks        = flag.Uint64("max-ticks", 0, "hard upper limit on ticks (0 = unlimited)")
		tickUs          = flag.Uint64("tick-us", 1000, "tick step in microseconds")
		timeScale       = flag.Float64("time-scale", 0, "sim speed scale: 1.0 realtime, 0 disabled sleep, 0.2 slower")
		dataCount       = flag.Uint("data-count", 25, "number of original data packets the sender must deliver")
		dropProb        = flag.Float64("drop", 0.02, "packet drop probability")
		reorderProb     = flag.Float64("reorder", 0.03, "packet reorder probability")
		latUs           = flag.Uint64("lat-us", 30000, "base one-way latency us")
		jitterUs        = flag.Int64("jitter-us", 8000, "uniform jitter +/- us")
		fwdMbps         = flag.Uint64("fwd-mbps", 150, "sender to receiver bandwidth in megabits per second")
		revMbps         = flag.Uint64("rev-mbps", 150, "receiver to sender bandwidth in megabits per second")
		queueBytes      = flag.Int("queue-bytes", 1<<20, "maximum queued bytes per direction before overflow drops")
		dataBytes       = flag.Int("data-bytes", 1200, "wire size of each DATA packet in bytes")
		ackBytes        = flag.Int("ack-bytes", 64, "wire size of each ACK packet in bytes")
		nakBytes        = flag.Int("nak-bytes", 64, "base wire size of each NAK packet in bytes")
		nakMissBytes    = flag.Int("nak-miss-bytes", 4, "additional bytes per missing sequence carried in a NAK")
		ackIntervalUs   = flag.Uint64("ack-interval-us", 10000, "receiver cumulative ACK interval in microseconds")
		flowWindowBytes = flag.Int("flow-window-bytes", 25600, "initial receiver flow window size in bytes")
		initialCwnd     = flag.Int("initial-cwnd", 16, "initial congestion window in packets")
		debug           = flag.Bool("debug", false, "print debug events")
	)
	flag.Parse()

	disp := &Dispatcher{}
	stats := NewStatsSink()
	disp.Register(stats)
	disp.Register(&DebugSink{Enabled: *debug})

	cfg := DefaultUDTConfig()
	cfg.TxInitialFlowWindow = uint32(*flowWindowBytes)
	cfg.RxInitialFlowWindow = uint32(*flowWindowBytes)
	cfg.RxAckIntervalUs = *ackIntervalUs
	pipe := NewMessagePipe(PipeConfig{
		DropProb: *dropProb, ReorderProb: *reorderProb,
		BaseLatUs: *latUs, JitterUs: *jitterUs, MaxQueue: 1_000_000, MaxQueueBytes: *queueBytes,
		ForwardBitsPerSec:  *fwdMbps * 1_000_000,
		ReverseBitsPerSec:  *revMbps * 1_000_000,
		DataPacketBytes:    *dataBytes,
		AckPacketBytes:     *ackBytes,
		NakPacketBaseBytes: *nakBytes,
		NakMissingBytes:    *nakMissBytes,
		Seed:               123,
	}, disp)

	senderCC := NewUDTCC(uint32(*initialCwnd))
	receiverCC := NewUDTCC(uint32(*initialCwnd))

	var sender, receiver *UDT

	senderSend := func(pkt Packet) {
		disp.OnPacketBuilt(simNowUs, pkt.From, pkt.To, pkt.Type.String(), pkt.Seq)
		pipe.Enqueue(simNowUs, pkt)
	}
	receiverSend := senderSend
	targetPackets := uint32(*dataCount)

	sender = NewUDT("sender", true, cfg, senderCC, disp, cfg.TxMaxSeq, uint32(*dataBytes), senderSend, func(seq uint32) bool {
		return seq < targetPackets
	})
	receiver = NewUDT("receiver", false, cfg, receiverCC, disp, cfg.TxMaxSeq, uint32(*dataBytes), receiverSend, func(seq uint32) bool {
		_ = seq
		return false
	})

	deliver := func(nowUs uint64, pkt Packet) {
		switch pkt.Type {
		case PktData:
			receiver.OnRxDataReceived(pkt.Seq, nowUs)
		case PktAck:
			sender.OnTxAckReceived(pkt.AckSeq, pkt.FlowWindow, nowUs)
		case PktNak:
			sender.OnTxNakReceived(pkt.Missing, pkt.FlowWindow, nowUs)
		}
	}

	simNowUs = 0
	var tick uint64
	transferComplete := targetPackets == 0
	for !transferComplete {
		disp.OnTickStart(simNowUs, tick)
		sender.TxTick(simNowUs)
		pipe.Tick(simNowUs, deliver)
		receiver.RxTick(simNowUs)
		disp.OnTickEnd(simNowUs, tick)
		tick++

		transferComplete = sender.SentPacketCount() >= targetPackets && receiver.ReceivedPacketCount() >= targetPackets
		if transferComplete {
			break
		}
		if *maxTicks > 0 && tick >= *maxTicks {
			break
		}

		if *timeScale > 0 {
			step := time.Duration(float64(*tickUs)/(*timeScale)) * time.Microsecond
			time.Sleep(step)
		}

		simNowUs += *tickUs
	}
	_ = reportedSimUs

	txDataPackets := stats.counts[EventTxSendData] + stats.counts[EventTxSendRetransmit]
	txWireBytes := txDataPackets * uint64(*dataBytes)
	senderThroughputBytesPerSec := uint64(0)
	if simNowUs > 0 {
		senderThroughputBytesPerSec = (txWireBytes * 1_000_000) / simNowUs
	}

	fmt.Printf("Total Simulation Time: %s us\n", formatUint64Underscore(simNowUs))
	fmt.Printf(
		"Estimated Sender Throughput: %s bytes/s (%s bytes over wire)\n",
		formatUint64Underscore(senderThroughputBytesPerSec),
		formatUint64Underscore(txWireBytes),
	)

	// Configuration Summary
	fmt.Println()
	fmt.Println("=== Configuration Summary ===")
	fmt.Printf("  Target Packets: %d\n", targetPackets)
	fmt.Printf("  Tick Duration: %d us\n", *tickUs)
	fmt.Printf("  Time Scale: %.2f\n", *timeScale)
	fmt.Printf("  Drop Probability: %.2f%%\n", *dropProb*100)
	fmt.Printf("  Reorder Probability: %.2f%%\n", *reorderProb*100)
	fmt.Printf("  Base Latency: %d us\n", *latUs)
	fmt.Printf("  Jitter: ±%d us\n", *jitterUs)
	fmt.Printf("  Forward Bandwidth: %d Mbps\n", *fwdMbps)
	fmt.Printf("  Reverse Bandwidth: %d Mbps\n", *revMbps)
	fmt.Printf("  Queue Size: %d bytes\n", *queueBytes)
	fmt.Printf("  Data Packet Size: %d bytes\n", *dataBytes)
	fmt.Printf("  ACK Packet Size: %d bytes\n", *ackBytes)
	fmt.Printf("  NAK Packet Base Size: %d bytes\n", *nakBytes)
	fmt.Printf("  NAK Missing Entry Size: %d bytes\n", *nakMissBytes)
	fmt.Printf("  ACK Interval: %d us\n", *ackIntervalUs)
	fmt.Printf("  Flow Window Size: %d bytes\n", *flowWindowBytes)
	fmt.Printf("  Initial Congestion Window: %d packets\n", *initialCwnd)

	fmt.Println()
	fmt.Printf(
		"Simulation finished at %dus (ticks=%d, target=%d, sent=%d, acked=%d, received=%d, complete=%t)\n",
		simNowUs,
		tick,
		targetPackets,
		sender.SentPacketCount(),
		sender.AckedPacketCount(),
		receiver.ReceivedPacketCount(),
		transferComplete,
	)

	stats.PrintSummary()
}

var simNowUs uint64
