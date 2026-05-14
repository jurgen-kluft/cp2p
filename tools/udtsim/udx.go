package main

// --------------------------------------------------------------------------------
// --- CongestionWindow -----------------------------------------------------------
// --------------------------------------------------------------------------------

type CongestionWindow struct {
	Window  [2]uint32 // [min, max] clamp for Current
	Initial uint32
	Current uint32
}

func NewCongestionWindow(min uint32, max uint32, initial uint32) *CongestionWindow {
	min = clampU32(min, 1, max)
	max = clampU32(max, min, 0xFFFFFFFF)
	initial = clampU32(initial, min, max)
	return &CongestionWindow{
		Window:  [2]uint32{min, max},
		Initial: initial,
		Current: initial,
	}
}

// OnReceived for ACK applies AIMD additive increase: +1/cwnd per ACK (congestion avoidance), and
// for NAK applies AIMD multiplicative decrease: cwnd *= 7/8 on loss.
func (c *CongestionWindow) OnReceived(pktType PacketType, recorder SenderProtocolRecorder, nowUs uint64) {
	switch pktType {
	case PacketTypeAck:
		if c.Current > 0 {
			c.Current++
		} else {
			c.Current = c.Initial
		}
		if c.Current > c.Window[1] {
			c.Current = c.Window[1]
		}
		if recorder != nil {
			recorder.OnCongestionWindowUpdated(c.Current, nowUs)
		}
	case PacketTypeNak:
		c.Current = c.Current - c.Current/8
		if c.Current < c.Window[0] {
			c.Current = c.Window[0]
		}
		if recorder != nil {
			recorder.OnCongestionWindowUpdated(c.Current, nowUs)
		}
	}
}

// Reset returns the congestion window to its initial value.
func (c *CongestionWindow) Reset() {
	c.Current = c.Initial
}

// InFlight returns true if adding one more packet would exceed the congestion window.
func (c *CongestionWindow) AllowsPacket(inFlightCount uint32) bool {
	return inFlightCount < c.Current
}

// --------------------------------------------------------------------------------
// --- FlowWindow -----------------------------------------------------------------
// --------------------------------------------------------------------------------

type FlowWindow struct {
	Window  [2]uint32 // [min, max] clamp for Current
	Initial uint32
	Current uint32
}

func NewFlowWindow(minBytes uint32, maxBytes uint32, initialBytes uint32) *FlowWindow {
	minBytes = clampU32(minBytes, 1, maxBytes)
	maxBytes = clampU32(maxBytes, minBytes, 0xFFFFFFFF)
	initialBytes = clampU32(initialBytes, minBytes, maxBytes)
	return &FlowWindow{
		Window:  [2]uint32{minBytes, maxBytes},
		Initial: initialBytes,
		Current: initialBytes,
	}
}

// Update applies the receiver-advertised window size, clamped to [Window[0], Window[1]].
func (f *FlowWindow) Update(advertisedBytes uint32, recorder SenderProtocolRecorder, nowUs uint64) {
	advertisedBytes = clampU32(advertisedBytes, f.Window[0], f.Window[1])
	if advertisedBytes == f.Current {
		return
	}
	f.Current = advertisedBytes
	if recorder != nil {
		recorder.OnFlowWindowUpdated(f.Current, nowUs)
	}
}

// Reset returns the flow window to its initial value.
func (f *FlowWindow) Reset() {
	f.Current = f.Initial
}

// AllowsPacket returns true if the given in-flight byte count leaves room for one more packet of packetBytes.
func (f *FlowWindow) AllowsPacket(inFlightBytes uint32, packetBytes uint32) bool {
	return inFlightBytes+packetBytes <= f.Current
}

// --------------------------------------------------------------------------------
// --- PacketPacing ---------------------------------------------------------------
// --------------------------------------------------------------------------------

type PacketPacing struct {
	MinMax     [2]uint64 // Min and Max pacing in microseconds
	PacingUs   uint64    // Current pacing in microseconds
	NextSendUs uint64    // Earliest time the protocol allows to send the next packet
}

func NewPacketPacing(minUs uint64, maxUs uint64, nowUs uint64) *PacketPacing {
	return &PacketPacing{
		MinMax:     [2]uint64{minUs, maxUs},
		PacingUs:   minUs,
		NextSendUs: nowUs,
	}
}

// Update recalculates the pacing interval as rttUs/cwnd, clamped to [MinMax[0], MinMax[1]].
// This matches UDT's inter-packet gap = estimated RTT / congestion window.
func (p *PacketPacing) Update(rttUs uint32, cwnd uint32, recorder SenderProtocolRecorder, nowUs uint64) {
	if cwnd == 0 {
		cwnd = 1
	}

	interval := uint64(rttUs) / uint64(cwnd)
	interval = clampU64(interval, p.MinMax[0], p.MinMax[1])

	if interval == p.PacingUs {
		return
	}
	p.PacingUs = interval
	if recorder != nil {
		recorder.OnPacketPacingUpdated(p.PacingUs, nowUs)
	}
}

// OnPacketSent advances the next-allowed send time by the current pacing interval.
func (p *PacketPacing) OnPacketSent(nowUs uint64) {
	if nowUs > p.NextSendUs {
		p.NextSendUs = nowUs + p.PacingUs
	} else {
		p.NextSendUs += p.PacingUs
	}
}

// AllowsSend returns true when the current time has reached or passed the next allowed send time.
func (p *PacketPacing) AllowsSend(nowUs uint64) bool {
	return nowUs >= p.NextSendUs
}

// --------------------------------------------------------------------------------
// --- SenderProtocolRecorder -----------------------------------------------------
// --------------------------------------------------------------------------------

type SenderProtocolRecorder interface {
	OnPacketSent(seq uint32, resent bool, nowUs uint64)
	OnAckReceived(ackSeq uint32, nowUs uint64)
	OnNakReceived(nowUs uint64)
	OnSenderTimeout(nowUs uint64)
	OnCongestionWindowUpdated(cwnd uint32, nowUs uint64)
	OnRttEstimateReceived(rtt uint32, nowUs uint64)
	OnFlowWindowUpdated(fwnd uint32, nowUs uint64)
	OnPacketPacingUpdated(ppUs uint64, nowUs uint64)
}

// --------------------------------------------------------------------------------
// --- Packet Related Interfaces --------------------------------------------------
// --------------------------------------------------------------------------------

type PacketReader interface {
	ReadUInt8() uint8
	ReadUInt16() uint16
	ReadUInt32() uint32
	ReadUInt64() uint64
	ReadSequenceMap(seqMap *SequenceMap)
}

func ReadPacketHeader(r PacketReader) (packetType uint8, flags uint8, seq uint32) {
	header := r.ReadUInt32()
	packetType = uint8(header >> 28)
	flags = uint8((header >> 20) & 0xFF)
	seq = header & 0xFFFFF
	return
}

func ReadPacketRtts(r PacketReader) (rtts uint32) {
	rtts = r.ReadUInt32()
	return
}

type PacketWriter interface {
	WriteUInt8(value uint8)
	WriteUInt16(value uint16)
	WriteUInt32(value uint32)
	WriteUInt64(value uint64)
	WriteSequenceMap(seqMap *SequenceMap)
}

func WritePacketHeader(w PacketWriter, packetType uint8, flags uint8, seq uint32) {
	header := (uint32(packetType) << 28) | (uint32(flags) << 20) | (seq & 0xFFFFF)
	w.WriteUInt32(header)
}

type PacketBuilder interface {
	NewPacketWriter() PacketWriter
}

type PacketIO interface {
	SendPacket(writer PacketWriter)
}

// --------------------------------------------------------------------------------
// --- SenderProtocol -------------------------------------------------------------
// --------------------------------------------------------------------------------

type SenderProtocol interface {
	Begin(nowUs uint64)                                                        // Called once at the start of the sending process, allowing the protocol to initialize any state.
	Process(nowUs uint64, packetBuild PacketBuilder, packetIO PacketIO) uint64 // Called periodically to allow the protocol to decide when to send packets or ACKs/NAKs. Returns the next time (in microseconds) the protocol wants to be called.

	OnPacketReceived(packet PacketReader, nowUs uint64) // Called when an ACK or NAK is received
}

// --------------------------------------------------------------------------------
// --- UDX Sender Implementation --------------------------------------------------
// --------------------------------------------------------------------------------

// A protocol implementation for the sender
type SenderUDX struct {
	recorder            SenderProtocolRecorder
	inFlight            *SequenceMap
	missing             *SequenceMap
	nakMissing          *SequenceMap
	cwnd                *CongestionWindow // Congestion control state (MUST be present for the protocol to work)
	fwnd                *FlowWindow       // Flow control state (MUST be present for the protocol to work)
	pp                  *PacketPacing     // Packet pacing state (MUST be present for the protocol to work)
	averagePktSizeBytes uint32            // Configured average packet size in bytes, used for flow control decisions
	currentSeq          uint32
	maxSeq              uint32
	beginTimeUs         uint64
	timeOutIntervalUs   uint64
	timeOutUs           uint64
}

type SenderUDXConfig struct {
	CWndMin             uint32
	CWndMax             uint32
	CWndInitial         uint32
	FWndMin             uint32
	FWndMax             uint32
	FWndInitial         uint32
	PacingMinUs         uint64
	PacingMaxUs         uint64
	AveragePktSizeBytes uint32
	MaxSeq              uint32
	TimeOutIntervalUs   uint64
}

func NewSenderUDX(recorder SenderProtocolRecorder, config SenderUDXConfig) *SenderUDX {
	return &SenderUDX{
		recorder:            recorder,
		inFlight:            NewSequenceMap(config.MaxSeq, SeqMapLowest, 123),
		missing:             NewSequenceMap(config.MaxSeq, SeqMapRoundRobin, 123),
		nakMissing:          NewSequenceMap(config.MaxSeq, SeqMapLowest, 123),
		cwnd:                &CongestionWindow{Window: [2]uint32{config.CWndMin, config.CWndMax}, Initial: config.CWndInitial, Current: config.CWndInitial},
		fwnd:                &FlowWindow{Window: [2]uint32{config.FWndMin, config.FWndMax}, Initial: config.FWndInitial, Current: config.FWndInitial},
		pp:                  NewPacketPacing(config.PacingMinUs, config.PacingMaxUs, 0),
		averagePktSizeBytes: config.AveragePktSizeBytes,
		currentSeq:          0,
		maxSeq:              config.MaxSeq,
		beginTimeUs:         0,
		timeOutIntervalUs:   config.TimeOutIntervalUs,
		timeOutUs:           0,
	}
}

func (s *SenderUDX) Begin(nowUs uint64) {
	s.beginTimeUs = nowUs
	s.timeOutUs = nowUs + s.timeOutIntervalUs
	s.fwnd.Update(s.cwnd.Current*s.averagePktSizeBytes, s.recorder, nowUs)
}

func (s *SenderUDX) resetTimeout(nowUs uint64) {
	s.timeOutUs = nowUs + s.timeOutIntervalUs
}

// Process is called periodically by the sender to allow the protocol to decide when to
// send new packets or retransmissions.
// It returns the minimum time (in microseconds) until the next call to Process, which allows
// the protocol to implement packet pacing and timeouts.
func (s *SenderUDX) Process(nowUs uint64, packetBuild PacketBuilder, packetIO PacketIO) uint64 {
	if s.pp == nil || s.cwnd == nil || s.fwnd == nil || s.inFlight == nil || s.missing == nil {
		return nowUs + 1
	}

	// Timeout handling: move all currently in-flight packets into the retransmit queue.
	if s.timeOutUs > 0 && nowUs >= s.timeOutUs {
		if s.missing.Size() == 0 {
			s.missing.Merge(s.inFlight)
		}
		s.cwnd.OnReceived(PacketTypeNak, s.recorder, nowUs)

		// Flow Window is following the Congestion Window, so we update it based on the
		// new congestion window size after a timeout event.
		s.fwnd.Update(s.cwnd.Current*s.averagePktSizeBytes, s.recorder, nowUs)

		if s.recorder != nil {
			s.recorder.OnSenderTimeout(nowUs)
			s.recorder.OnCongestionWindowUpdated(s.cwnd.Current, nowUs)
		}
		if s.pp.PacingUs == 0 {
			s.pp.PacingUs = s.pp.MinMax[0]
		}
		s.timeOutUs = nowUs + (s.pp.PacingUs * uint64(s.cwnd.Current))
	}

	if !s.pp.AllowsSend(nowUs) {
		nextWakeUs := s.pp.NextSendUs
		if s.timeOutUs > 0 && s.timeOutUs < nextWakeUs {
			nextWakeUs = s.timeOutUs
		}
		return nextWakeUs
	}

	inFlightCount := s.inFlight.Size()
	inFlightBytes := ccmulU32(inFlightCount, s.averagePktSizeBytes)
	canSendByCongestion := s.cwnd.AllowsPacket(inFlightCount)
	canSendByFlow := s.fwnd.AllowsPacket(inFlightBytes, s.averagePktSizeBytes)

	// UDT sender priority: retransmissions first, then new data.
	if s.missing.Size() > 0 && canSendByCongestion && canSendByFlow {
		seq := s.missing.Pop()
		if seq >= 0 {
			s.inFlight.Push(uint32(seq))

			w := packetBuild.NewPacketWriter()
			WritePacketHeader(w, PacketTypeData, 0, uint32(seq))

			// 1 * 60 * 60 * 1000 * 1000 = 3_600_000_000, so 32-bit can hold one
			// hour of microsecond timestamps, which should be enough for a single
			// session, and since UDX is only active for one object session, this
			// should be more than enough for the entire session duration.
			w.WriteUInt32(uint32(nowUs - s.beginTimeUs))
			// w.WriteBytes, the actual data (but this is a demo)
			packetIO.SendPacket(w)

			s.pp.OnPacketSent(nowUs)
			if s.recorder != nil {
				s.recorder.OnPacketSent(uint32(seq), true, nowUs)
				s.recorder.OnPacketPacingUpdated(s.pp.PacingUs, nowUs)
			}
			if s.timeOutUs == 0 || s.timeOutUs < nowUs {
				s.timeOutUs = nowUs + (s.pp.PacingUs * uint64(s.cwnd.Current))
			}
			return s.pp.NextSendUs
		}
	}

	if s.currentSeq < s.maxSeq && canSendByCongestion && canSendByFlow {
		seq := s.currentSeq
		s.currentSeq++
		s.inFlight.Push(seq)

		w := packetBuild.NewPacketWriter()
		WritePacketHeader(w, PacketTypeData, 0, uint32(seq))
		w.WriteUInt32(uint32(nowUs - s.beginTimeUs))
		// w.WriteBytes, the actual data (but this is a demo)
		packetIO.SendPacket(w)

		s.pp.OnPacketSent(nowUs)
		if s.recorder != nil {
			s.recorder.OnPacketSent(seq, false, nowUs)
			s.recorder.OnPacketPacingUpdated(s.pp.PacingUs, nowUs)
		}
		if s.timeOutUs == 0 || s.timeOutUs < nowUs {
			s.timeOutUs = nowUs + (s.pp.PacingUs * uint64(s.cwnd.Current))
		}
		return s.pp.NextSendUs
	}

	nextWakeUs := s.pp.NextSendUs
	if s.timeOutUs > 0 && s.timeOutUs < nextWakeUs {
		nextWakeUs = s.timeOutUs
	}
	if nextWakeUs == 0 {
		nextWakeUs = nowUs + 1
	}
	return nextWakeUs
}

const (
	PacketTypeAck  = 1
	PacketTypeNak  = 2
	PacketTypeData = 3
)

func (s *SenderUDX) onAckReceived(ackSeq uint32, rtts uint32, nowUs uint64) {
	// Receiving an ACK means we acknowledge all packets up to and including ackSeq.
	// So we can remove all those packets from the in-flight map.
	s.inFlight.RemoveRange(0, ackSeq+1)

	// Also clear anything in the missing map that is up to and including ackSeq,
	// since those are now acknowledged as well.
	s.missing.RemoveRange(0, ackSeq+1)

	s.cwnd.OnReceived(PacketTypeAck, s.recorder, nowUs)

	s.fwnd.Update(s.cwnd.Current*s.averagePktSizeBytes, s.recorder, nowUs)

	// The ACK includes a compensated RTT value (send_time - receiver_delay).
	// We compute the RTT by subtracting this from our current relative time.
	if rtts < 0xFFFFFFFF {
		nowRelative := uint32(nowUs - s.beginTimeUs)
		// If compensation underflowed, rtts might be in a wrapped state; skip update
		if nowRelative >= rtts {
			rttEstimateUs := nowRelative - rtts
			s.pp.Update(rttEstimateUs, s.cwnd.Current, s.recorder, nowUs)
			if s.recorder != nil {
				s.recorder.OnRttEstimateReceived(rttEstimateUs, nowUs)
			}
		}
	}

	// Reset the timeout since we received an ACK; we know at least some packets got through.
	s.resetTimeout(nowUs)

	if s.recorder != nil {
		s.recorder.OnAckReceived(ackSeq, nowUs)
	}
}

func (s *SenderUDX) onNakReceived(seqMap *SequenceMap, nowUs uint64) {
	// Receiving a NAK means that the receiver is missing certain packets.
	// The seqMap contains the sequence numbers of the missing packets.
	s.missing.Merge(seqMap)

	// Reset the timeout since we received a NAK, and we know that at least some
	// packets got through.
	s.resetTimeout(nowUs)

	s.cwnd.OnReceived(PacketTypeNak, s.recorder, nowUs)
	s.fwnd.Update(s.cwnd.Current*s.averagePktSizeBytes, s.recorder, nowUs)

	s.timeOutUs = nowUs + (s.pp.PacingUs * uint64(s.cwnd.Current))

	if s.recorder != nil {
		s.recorder.OnNakReceived(nowUs)
	}
}

func (s *SenderUDX) OnPacketReceived(reader PacketReader, nowUs uint64) {

	packetType, _, seq := ReadPacketHeader(reader)

	switch packetType {

	case PacketTypeAck:
		// Read the compensated RTT value echoed by the receiver:
		//   compensatedRtts = sendTimeRelative + receiverProcessingDelay
		// The sender subtracts this from its own current relative time to obtain
		// RTT_full - receiverProcessingDelay, i.e. pure network round-trip time.
		// 0xFFFFFFFF signals no valid RTT sample.
		rtts := ReadPacketRtts(reader)
		s.onAckReceived(seq, rtts, nowUs)

	case PacketTypeNak:
		// Decode the NAK packet, this depends on the protocol, some might have included
		// the flow window and RTT information, some might not.
		// Also a NAK includes information regarding missing packets, again this can be
		// protocol specific. In this demo, we assume it includes a serialized SequenceMap.
		s.nakMissing.RemoveAll()
		reader.ReadSequenceMap(s.nakMissing)
		s.onNakReceived(s.nakMissing, nowUs)
	}
}

// --------------------------------------------------------------------------------
// --- Receiver Protocol Recorder -------------------------------------------------
// --------------------------------------------------------------------------------

type ReceiverProtocolRecorder interface {
	OnDataPacketReceived(seq uint32, nowUs uint64)
	OnAckSent(seq uint32, nowUs uint64)
	OnNakSent(nowUs uint64)
	OnReceiverTimeout(nowUs uint64)

	OnInvalidPacketReceived(nowUs uint64)
	OnDuplicateSequenceReceived(fromSeq uint32, toSeq uint32, nowUs uint64)
	OnMissingSequencesDetected(fromSeq uint32, toSeq uint32, nowUs uint64)
	OnReceiverFlowWindowUpdated(fwnd uint32, nowUs uint64)
}

// --------------------------------------------------------------------------------
// --- ReceiverProtocol -----------------------------------------------------------
// --------------------------------------------------------------------------------

type ReceiverProtocol interface {
	Begin(nowUs uint64)
	Process(nowUs uint64, packetBuild PacketBuilder, packetIO PacketIO) uint64

	OnPacketReceived(reader PacketReader, nowUs uint64)
	CurrentFlowWindow() *FlowWindow
}

// --------------------------------------------------------------------------------
// --- UDX Receiver Implementation ------------------------------------------------
// --------------------------------------------------------------------------------

type ReceiverUDX struct {
	recorder             ReceiverProtocolRecorder
	received             *SequenceMap
	missing              *SequenceMap
	highestContigSeq     uint32
	highestReceivedSeq   uint32
	highestReceivedSeqUs uint64
	maxSeq               uint32
	ackIntervalUs        uint64
	nakIntervalUs        uint64
	ackTimeOutUs         uint64
	nakTimeOutUs         uint64
	receivedTimeUs       uint64
	receivedRtts         uint32
}

type ReceiverUDXConfig struct {
	MaxSeq        uint32
	AckIntervalUs uint64
	NakIntervalUs uint64
}

// TODO:
// - Implement EXP timer for receiver timeout, and trigger appropriate callback on timeout.
// - Sender back-pressure handling, e.g. Flow Window Management:
//   For UDX + UOBP we don't need this, memory is preallocated, but for a more general protocol w
//   e might want to have some callback to the application layer to apply back-pressure when the
//   receiver's missing map grows too large, indicating that the sender is sending more than the
//   receiver can handle.

func NewReceiverUDX(recorder ReceiverProtocolRecorder, config ReceiverUDXConfig) *ReceiverUDX {
	return &ReceiverUDX{
		recorder:             recorder,
		received:             NewSequenceMap(config.MaxSeq, SeqMapLowest, 123),
		missing:              NewSequenceMap(config.MaxSeq, SeqMapRoundRobin, 123),
		highestContigSeq:     0xFFFFFFFF,
		highestReceivedSeq:   0,
		highestReceivedSeqUs: 0,
		maxSeq:               config.MaxSeq,
		ackIntervalUs:        config.AckIntervalUs,
		nakIntervalUs:        config.NakIntervalUs,
		ackTimeOutUs:         0,
		nakTimeOutUs:         0,
		receivedTimeUs:       0,
		receivedRtts:         0,
	}
}

func (r *ReceiverUDX) Begin(nowUs uint64) {
	r.ackTimeOutUs = nowUs + r.ackIntervalUs
	r.nakTimeOutUs = nowUs + r.nakIntervalUs
}

func (r *ReceiverUDX) Process(nowUs uint64, packetBuild PacketBuilder, packetIO PacketIO) uint64 {

	if nowUs >= r.ackTimeOutUs {
		w := packetBuild.NewPacketWriter()

		WritePacketHeader(w, PacketTypeAck, 0, r.highestContigSeq)

		// Write compensated RTT value: original send time plus receiver processing delay.
		// This allows the sender to calculate RTT without receiver processing overhead:
		//   sender computes nowRelative - compensatedRtts
		//     = (T_ack_recv - t_begin) - ((T_send - t_begin) + receiverDelay)
		//     = (T_ack_recv - T_send) - receiverDelay
		//     = RTT_full - receiverDelay
		if r.receivedTimeUs > 0 && (nowUs-r.receivedTimeUs) < 0xFFFFFFFF {
			// receivedRtts = send_time_relative (from DATA packet)
			// receiver_delay = nowUs - r.receivedTimeUs
			// send back => (send_time_relative + receiver_delay)
			receiverDelay := uint32(nowUs - r.receivedTimeUs)
			compensatedRtts := r.receivedRtts + receiverDelay
			w.WriteUInt32(compensatedRtts)
		} else {
			w.WriteUInt32(0xFFFFFFFF)
		}

		packetIO.SendPacket(w)
		r.recorder.OnAckSent(r.highestContigSeq, nowUs)
		r.ackTimeOutUs = nowUs + r.ackIntervalUs

		// Reset the received time and RTTs after sending an ACK, since we've now acknowledged the receipt
		// of those packets, and any future ACKs will be for new packets with new RTT estimates.
		r.receivedTimeUs = 0
		r.receivedRtts = 0
	}

	if nowUs >= r.nakTimeOutUs && r.missing.Size() > 0 {
		w := packetBuild.NewPacketWriter()
		WritePacketHeader(w, PacketTypeNak, 0, 0)
		w.WriteSequenceMap(r.missing)
		packetIO.SendPacket(w)
		r.recorder.OnNakSent(nowUs)
		r.nakTimeOutUs = nowUs + r.nakIntervalUs
	}

	nextWakeUs := r.ackTimeOutUs
	if r.missing.Size() > 0 && r.nakTimeOutUs < nextWakeUs {
		nextWakeUs = r.nakTimeOutUs
	}

	return nextWakeUs
}

func (r *ReceiverUDX) OnDataPacketReceived(reader PacketReader, nowUs uint64) {
	packetType, _, seq := ReadPacketHeader(reader)

	// RTT send time from sender, we need to reply this in an ACK packet so that
	// the sender can calculate the RTT based on the time difference between now
	// and this send time.
	rtts := reader.ReadUInt32()

	if packetType != PacketTypeData {
		r.recorder.OnInvalidPacketReceived(nowUs)
		return
	}

	// Store the RTT estimate and receive time for this sequence number so that
	// it can be used when sending the next ACK. Only update after type validation
	// to avoid corrupting RTT state with garbage from malformed packets.
	r.receivedTimeUs = nowUs
	r.receivedRtts = rtts

	r.recorder.OnDataPacketReceived(seq, nowUs)

	if !r.received.Push(seq) {
		r.recorder.OnDuplicateSequenceReceived(seq, seq, nowUs)
		return
	}

	if seq > (r.highestContigSeq + 1) {
		r.recorder.OnMissingSequencesDetected(r.highestContigSeq+1, seq-1, nowUs)
		firstMissing := r.highestContigSeq + 1
		for s := firstMissing; s < seq; s++ {
			if !r.received.Has(s) {
				r.missing.Push(s)
			}
		}
	}

	if seq > r.highestReceivedSeq {
		r.highestReceivedSeq = seq
		r.highestReceivedSeqUs = nowUs
	}

	r.missing.Remove(seq)

	if hseq, ok := r.received.SearchContiguesSet(r.highestContigSeq+1, r.maxSeq); ok {
		r.highestContigSeq = hseq
	}
}
