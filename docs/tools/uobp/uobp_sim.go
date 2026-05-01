package main

import (
	"encoding/binary"
	"encoding/json"
	"flag"
	"fmt"
	"math/rand"
	"sync"
	"time"
)

type u8 = uint8
type u16 = uint16
type u32 = uint32
type u64 = uint64

type i8 = int8
type i16 = int16
type i32 = int32
type i64 = int64

// ============================================================
// UOBP wire constants
// ============================================================

const (
	MSG_OBJECT_INFO        u16 = 1
	MSG_OBJECT_DATA        u16 = 2
	MSG_OBJECT_ACK         u16 = 3
	MSG_OBJECT_ACK_CONFIRM u16 = 4
)

// ============================================================
// Simulation stats (CI / JSON friendly)
// ============================================================

type PipeStats struct {
	PacketsIn  u64 `json:"packets_in"`
	PacketsOut u64 `json:"packets_out"`
	Dropped    u64 `json:"dropped"`
	Reordered  u64 `json:"reordered"`
}

type RxStats struct {
	ObjectInfos      u64 `json:"object_infos"`
	DataPackets      u64 `json:"data_packets"`
	ValidBlocks      u64 `json:"valid_blocks"`
	DuplicateBlocks  u64 `json:"duplicate_blocks"`
	HashFailures     u64 `json:"hash_failures"`
	CompletedObjects u64 `json:"completed_objects"`
	AckPacketsSent   u64 `json:"ack_packets_sent"`
	AckCoalesced     u64 `json:"ack_coalesced"`
}

type TxStats struct {
	ObjectsStarted u64 `json:"objects_started"`
	DataSent       u64 `json:"data_packets_sent"`
	Retransmits    u64 `json:"retransmits"`
	AcksReceived   u64 `json:"acks_received"`
	AcksTimeout    u64 `json:"acks_timeout"`

	MaxInFlight   u16 `json:"max_in_flight"`
	FinalInFlight u16 `json:"final_in_flight"`
}

type BenchmarkStats struct {
	ElapsedMs             u64     `json:"elapsed_ms"`
	ElapsedSeconds        float64 `json:"elapsed_seconds"`
	PacketsPerSecond      float64 `json:"packets_per_second"`
	DataPacketsPerSecond  float64 `json:"data_packets_per_second"`
	RetransmitsPerSecond  float64 `json:"retransmits_per_second"`
	AckPacketsPerSecond   float64 `json:"ack_packets_per_second"`
	AckCoalescedPerSecond float64 `json:"ack_coalesced_per_second"`
}

type Stats struct {
	Pipe      PipeStats      `json:"pipe"`
	Rx        RxStats        `json:"receiver"`
	Tx        TxStats        `json:"sender"`
	Benchmark BenchmarkStats `json:"benchmark"`
}

func ratePerSecond(count u64, secs float64) float64 {
	if secs <= 0 {
		return 0
	}
	return float64(count) / secs
}

// ============================================================
// Wire structs (logical)
// ============================================================

type objectInfo struct {
	ObjectIndex u16
	ObjectGen   u16
	ObjectSize  u32
	BlockSize   u16
}

type objectData struct {
	ObjectIndex u16
	ObjectGen   u16
	BlockIdx    u16
	BlockLen    u16
	Hash32      u32
}

type rxState u8

const (
	RXSTATEIDLE rxState = iota
	RXSTATERECEIVING
	RXSTATECOMPLETEDAWAITCONFIRM
)

type txState u8

const (
	TXSTATEINIT txState = iota
	TXSTATESENDING
	TXSTATEAWAITINGACK
	TXSTATECONFIRMED
)

type rxAckState u8

const (
	rxAckStateClean rxAckState = iota
	rxAckStatePending
)

func encodeObjectInfo(objectIndex, objectGen u16, objectSize u32, blockSize u16) []u8 {
	info := make([]u8, 12)
	binary.LittleEndian.PutUint16(info[0:], MSG_OBJECT_INFO)
	binary.LittleEndian.PutUint16(info[2:], objectIndex)
	binary.LittleEndian.PutUint16(info[4:], objectGen)
	binary.LittleEndian.PutUint32(info[6:], objectSize)
	binary.LittleEndian.PutUint16(info[10:], blockSize)
	return info
}

func decodeObjectInfo(buf []u8) (objectInfo, bool) {
	if len(buf) < 12 {
		return objectInfo{}, false
	}
	if binary.LittleEndian.Uint16(buf[0:2]) != MSG_OBJECT_INFO {
		return objectInfo{}, false
	}
	return objectInfo{
		ObjectIndex: binary.LittleEndian.Uint16(buf[2:4]),
		ObjectGen:   binary.LittleEndian.Uint16(buf[4:6]),
		ObjectSize:  binary.LittleEndian.Uint32(buf[6:10]),
		BlockSize:   binary.LittleEndian.Uint16(buf[10:12]),
	}, true
}

func encodeObjectData(objectIndex, objectGen, blockIdx u16, payload []u8) []u8 {
	buf := make([]u8, 14+len(payload))
	binary.LittleEndian.PutUint16(buf[0:], MSG_OBJECT_DATA)
	binary.LittleEndian.PutUint16(buf[2:], objectIndex)
	binary.LittleEndian.PutUint16(buf[4:], objectGen)
	binary.LittleEndian.PutUint16(buf[6:], blockIdx)
	binary.LittleEndian.PutUint16(buf[8:], u16(len(payload)))
	binary.LittleEndian.PutUint32(buf[10:], hash32(payload))
	copy(buf[14:], payload)
	return buf
}

func decodeObjectData(buf []u8) (objectData, []u8, bool) {
	if len(buf) < 14 {
		return objectData{}, nil, false
	}
	if binary.LittleEndian.Uint16(buf[0:2]) != MSG_OBJECT_DATA {
		return objectData{}, nil, false
	}
	msg := objectData{
		ObjectIndex: binary.LittleEndian.Uint16(buf[2:4]),
		ObjectGen:   binary.LittleEndian.Uint16(buf[4:6]),
		BlockIdx:    binary.LittleEndian.Uint16(buf[6:8]),
		BlockLen:    binary.LittleEndian.Uint16(buf[8:10]),
		Hash32:      binary.LittleEndian.Uint32(buf[10:14]),
	}
	if int(14+msg.BlockLen) > len(buf) {
		return objectData{}, nil, false
	}
	return msg, buf[14 : 14+msg.BlockLen], true
}

func encodeAck(objectIndex, objectGen u16, prefix i32, bitmap []u8) []u8 {
	ack := make([]u8, 8+len(bitmap))
	binary.LittleEndian.PutUint16(ack[0:], MSG_OBJECT_ACK)
	binary.LittleEndian.PutUint16(ack[2:], objectIndex)
	binary.LittleEndian.PutUint16(ack[4:], objectGen)
	binary.LittleEndian.PutUint16(ack[6:], u16(prefix))
	copy(ack[8:], bitmap)
	return ack
}

func decodeAck(buf []u8) (u16, u16, u16, []u8, bool) {
	if len(buf) < 8 {
		return 0, 0, 0, nil, false
	}
	if binary.LittleEndian.Uint16(buf[0:2]) != MSG_OBJECT_ACK {
		return 0, 0, 0, nil, false
	}
	objIdx := binary.LittleEndian.Uint16(buf[2:4])
	objGen := binary.LittleEndian.Uint16(buf[4:6])
	prefix := binary.LittleEndian.Uint16(buf[6:8])
	return objIdx, objGen, prefix, buf[8:], true
}

func encodeAckConfirm(objectIndex, objectGen u16) []u8 {
	confirm := make([]u8, 6)
	binary.LittleEndian.PutUint16(confirm[0:], MSG_OBJECT_ACK_CONFIRM)
	binary.LittleEndian.PutUint16(confirm[2:], objectIndex)
	binary.LittleEndian.PutUint16(confirm[4:], objectGen)
	return confirm
}

func decodeAckConfirm(buf []u8) (u16, u16, bool) {
	if len(buf) < 6 {
		return 0, 0, false
	}
	if binary.LittleEndian.Uint16(buf[0:2]) != MSG_OBJECT_ACK_CONFIRM {
		return 0, 0, false
	}
	objIdx := binary.LittleEndian.Uint16(buf[2:4])
	objGen := binary.LittleEndian.Uint16(buf[4:6])
	return objIdx, objGen, true
}

// ============================================================
// Bitmap helpers
// ============================================================

func bitmapBytes(bits i32) i32 {
	return (bits + 7) >> 3
}

func bitmapClear(b []u8) {
	for i := range b {
		b[i] = 0
	}
}

func bitmapTest(b []u8, bit u16) bool {
	return (b[bit>>3]>>(bit&7))&1 != 0
}

func bitmapSet(b []u8, bit u16) {
	b[bit>>3] |= 1 << (bit & 7)
}

func bitmapPrefixLen(b []u8, numBlocks i32) i32 {
	for i := i32(0); i < numBlocks; i++ {
		if !bitmapTest(b, u16(i)) {
			return i
		}
	}
	return numBlocks
}

// ============================================================
// Hash (deterministic placeholder)
// ============================================================

func hash32(data []u8) u32 {
	var h u32 = 2166136261
	for _, v := range data {
		h ^= u32(v)
		h *= 16777619
	}
	return h
}

// ============================================================
// RX engine (multi‑object)
// ============================================================

type rxCtx struct {
	ObjectGen       u16
	ObjectSize      u32
	BlockSize       u16
	NumBlocks       i32
	Data            []u8
	Bitmap          []u8
	BlocksReceived  u16
	PacketsSinceAck u16
	AckCount        u16
	LastAckMs       u32
	AckState        rxAckState
	State           rxState
}

type rx struct {
	slots [256]rxCtx
	mu    sync.Mutex
	stats *RxStats
}

func newRx(stats *RxStats) *rx {
	return &rx{
		stats: stats,
	}
}

func (r *rx) onObjectInfo(info objectInfo) {
	r.mu.Lock()
	defer r.mu.Unlock()

	r.stats.ObjectInfos++

	slot := &r.slots[info.ObjectIndex&0xff]
	if slot.ObjectGen == info.ObjectGen && slot.State != RXSTATEIDLE {
		// Duplicate OBJECT_INFO for in-flight/completed generation; keep existing state.
		return
	}
	*slot = rxCtx{}

	numBlocks := (i32(info.ObjectSize) + (i32(info.BlockSize) - 1)) / i32(info.BlockSize)

	slot.ObjectGen = info.ObjectGen
	slot.ObjectSize = info.ObjectSize
	slot.BlockSize = info.BlockSize
	slot.NumBlocks = numBlocks
	slot.Data = make([]u8, info.ObjectSize)
	slot.Bitmap = make([]u8, bitmapBytes(numBlocks))
	bitmapClear(slot.Bitmap)
	slot.State = RXSTATERECEIVING
}

func (r *rx) onObjectData(msg objectData, payload []u8) {

	r.mu.Lock()
	defer r.mu.Unlock()

	r.stats.DataPackets++

	slot := &r.slots[msg.ObjectIndex&0xff]
	if slot.State == RXSTATECOMPLETEDAWAITCONFIRM {
		// Completed objects ACK on heartbeat; do not re-arm per duplicate.
		return
	}

	if slot.State != RXSTATERECEIVING || slot.ObjectGen != msg.ObjectGen {
		return
	}
	if msg.BlockIdx >= u16(slot.NumBlocks) {
		return
	}
	if bitmapTest(slot.Bitmap, msg.BlockIdx) {
		r.stats.DuplicateBlocks++
		return
	}
	if hash32(payload) != msg.Hash32 {
		r.stats.HashFailures++
		return
	}

	offset := int(msg.BlockIdx) * int(slot.BlockSize)
	copy(slot.Data[offset:], payload)

	bitmapSet(slot.Bitmap, msg.BlockIdx)
	slot.BlocksReceived++
	slot.AckState = rxAckStatePending
	slot.PacketsSinceAck++

	r.stats.ValidBlocks++

	if slot.BlocksReceived == u16(slot.NumBlocks) {
		r.stats.CompletedObjects++
		fmt.Println("Object completed:", msg.ObjectIndex, "gen", msg.ObjectGen)
		slot.State = RXSTATECOMPLETEDAWAITCONFIRM
	}
}

func (r *rx) onAckConfirm(objectIndex, objectGen u16) {
	r.mu.Lock()
	defer r.mu.Unlock()

	slot := &r.slots[objectIndex&0xff]
	if slot.State == RXSTATECOMPLETEDAWAITCONFIRM && slot.ObjectGen == objectGen {
		*slot = rxCtx{}
	}
}

const COMPLETED_ACK_HEARTBEAT_MS u32 = 20

func (r *rx) maybeMakeAck(objectIndex, objectGen u16, nowMs u32, ackEvery u16, ackIntervalMs u32) ([]u8, bool) {
	r.mu.Lock()
	defer r.mu.Unlock()

	slot := &r.slots[objectIndex&0xff]
	if slot.State == RXSTATEIDLE || slot.ObjectGen != objectGen {
		return nil, false
	}
	receivingHeartbeatDue := slot.State == RXSTATERECEIVING && ackIntervalMs > 0 && slot.BlocksReceived > 0 && (nowMs-slot.LastAckMs) >= ackIntervalMs
	completedHeartbeatDue := slot.State == RXSTATECOMPLETEDAWAITCONFIRM && (nowMs-slot.LastAckMs) >= COMPLETED_ACK_HEARTBEAT_MS
	if slot.AckState != rxAckStatePending && !receivingHeartbeatDue && !completedHeartbeatDue {
		return nil, false
	}

	dueToFirst := slot.AckState == rxAckStatePending && slot.AckCount == 0
	dueToCount := slot.AckState == rxAckStatePending && ackEvery > 0 && slot.PacketsSinceAck >= ackEvery
	dueToTime := slot.AckState == rxAckStatePending && ackIntervalMs > 0 && slot.PacketsSinceAck > 0 && (nowMs-slot.LastAckMs) >= ackIntervalMs
	dueToComplete := slot.State == RXSTATECOMPLETEDAWAITCONFIRM && slot.AckState == rxAckStatePending
	dueToReceivingHeartbeat := receivingHeartbeatDue
	dueToCompletedHeartbeat := completedHeartbeatDue

	if !dueToFirst && !dueToCount && !dueToTime && !dueToComplete && !dueToReceivingHeartbeat && !dueToCompletedHeartbeat {
		return nil, false
	}
	coalesced := u16(0)
	if slot.AckState == rxAckStatePending {
		coalesced = slot.PacketsSinceAck
	}

	// Build bitmap ACK from current receiver state so duplicates can still be acknowledged.
	prefix := bitmapPrefixLen(slot.Bitmap, slot.NumBlocks)

	const MAX_ACK_BITS = 32
	remaining := slot.NumBlocks - prefix
	nBits := remaining
	if nBits > MAX_ACK_BITS {
		nBits = MAX_ACK_BITS
	} else if nBits < 0 {
		nBits = 0
	}

	ackBitmapBytes := (nBits + 7) / 8
	ackLen := 8 + int(ackBitmapBytes)
	ack := make([]u8, ackLen)

	binary.LittleEndian.PutUint16(ack[0:], MSG_OBJECT_ACK)
	binary.LittleEndian.PutUint16(ack[2:], objectIndex)
	binary.LittleEndian.PutUint16(ack[4:], objectGen)
	binary.LittleEndian.PutUint16(ack[6:], u16(prefix))

	for i := i32(0); i < nBits; i++ {
		if bitmapTest(slot.Bitmap, u16(prefix+i)) {
			ack[8+(i>>3)] |= 1 << (i & 7)
		}
	}

	if slot.AckState == rxAckStatePending {
		slot.AckState = rxAckStateClean
		slot.PacketsSinceAck = 0
	}
	slot.AckCount++
	slot.LastAckMs = nowMs
	r.stats.AckPacketsSent++
	if coalesced > 1 {
		r.stats.AckCoalesced += u64(coalesced - 1)
	}

	return ack, true
}

func (r *rx) step(pktData []u8, nowMs u32, ackEvery u16, ackIntervalMs u32) ([]u8, bool) {
	if len(pktData) < 2 {
		return nil, false
	}

	msgID := binary.LittleEndian.Uint16(pktData[0:2])
	switch msgID {
	case MSG_OBJECT_INFO:
		if info, ok := decodeObjectInfo(pktData); ok {
			r.onObjectInfo(info)
		}
		return nil, false

	case MSG_OBJECT_ACK_CONFIRM:
		if objIdx, objGen, ok := decodeAckConfirm(pktData); ok {
			r.onAckConfirm(objIdx, objGen)
		}
		return nil, false

	case MSG_OBJECT_DATA:
		msg, payload, ok := decodeObjectData(pktData)
		if !ok {
			return nil, false
		}
		r.onObjectData(msg, payload)
		return r.maybeMakeAck(msg.ObjectIndex, msg.ObjectGen, nowMs, ackEvery, ackIntervalMs)
	}

	return nil, false
}

// ============================================================
// TX engine (per object)
// ============================================================

type tx struct {
	ObjectIndex u16
	ObjectGen   u16
	BlockSize   u16
	NumBlocks   u16
	Data        []u8
	State       txState
	AckedBlocks u16

	AckBitmap  []u8
	SentBitmap []u8 // optional helper for stats

	InFlight       u16
	InflightLimit  u16
	MinInflight    u16
	MaxInflight    u16
	AckTimeoutMs   u32
	LastProgressMs u32

	InfoTxCount  u32 // NEW: count how many times OBJECT_INFO was sent
	LastInfoTxMs u32 // NEW: last time OBJECT_INFO was sent
	NextSendIdx  u16

	stats *TxStats
}

const INFO_RESEND_MS u32 = 20

func newTx(stats *TxStats) *tx {
	return &tx{
		MinInflight:  1,
		MaxInflight:  16,
		AckTimeoutMs: 50,
		stats:        stats,
	}
}

func (t *tx) start(index, gen u16, data []u8, blockSize u16) {
	t.ObjectIndex = index
	t.ObjectGen = gen
	t.Data = data
	t.BlockSize = blockSize
	t.State = TXSTATEINIT
	t.NumBlocks = u16((len(data) + int(blockSize) - 1) / int(blockSize))
	t.AckBitmap = make([]u8, bitmapBytes(i32(t.NumBlocks)))
	t.SentBitmap = make([]u8, bitmapBytes(i32(t.NumBlocks))) // optional helper for stats
	bitmapClear(t.AckBitmap)
	t.AckedBlocks = 0
	t.InFlight = 0
	t.InflightLimit = t.MinInflight
	t.stats.ObjectsStarted++
	t.InfoTxCount = 0
	t.LastInfoTxMs = 0
	t.NextSendIdx = 0
}

func (t *tx) isFullyAcked() bool {
	return t.AckedBlocks == t.NumBlocks
}

func (t *tx) sentOnce(blockIdx u16) bool {
	return bitmapTest(t.SentBitmap, blockIdx)
}

func (t *tx) onAck(blockIdx u16, nowMs u32) {
	if bitmapTest(t.AckBitmap, blockIdx) {
		return
	}
	bitmapSet(t.AckBitmap, blockIdx)
	t.AckedBlocks++
	if t.InFlight > 0 {
		t.InFlight--
	}
	t.LastProgressMs = nowMs
	t.stats.AcksReceived++

	if t.InflightLimit < t.MaxInflight {
		t.InflightLimit++
	}
}

func (t *tx) onAckBitmap(blockStart u16, blockBitmap []u8, nowMs u32) {
	// ACK contiguous prefix
	for i := u16(0); i < blockStart; i++ {
		t.onAck(i, nowMs)
	}

	for i := u16(0); i < u16(len(blockBitmap)*8); i++ {
		byteIdx := int(i >> 3)
		bitIdx := i & 7
		if (blockBitmap[byteIdx]>>bitIdx)&1 != 0 {
			t.onAck(blockStart+i, nowMs)
		}
	}

	if t.isFullyAcked() {
		t.State = TXSTATEAWAITINGACK
	}
}

func (t *tx) checkTimeout(nowMs u32) bool {
	if nowMs-t.LastProgressMs > t.AckTimeoutMs {
		t.InflightLimit >>= 1
		if t.InflightLimit < t.MinInflight {
			t.InflightLimit = t.MinInflight
		}
		t.LastProgressMs = nowMs
		t.stats.AcksTimeout++
		return true
	}
	return false
}

func (t *tx) maybeSendInfo(now u32, p *pipe) {
	if t.InfoTxCount == 0 || (t.AckedBlocks == 0 && (now-t.LastInfoTxMs) >= INFO_RESEND_MS) {
		p.sendA2B(packet{encodeObjectInfo(t.ObjectIndex, t.ObjectGen, u32(len(t.Data)), t.BlockSize)})
		t.LastInfoTxMs = now
		t.InfoTxCount++
	}
}

func (t *tx) maybeSendData(now u32, forceRetransmit bool, p *pipe) bool {
	if t.isFullyAcked() {
		return false
	}

	var sendIdx u16
	haveSend := false
	allowNew := t.InFlight < t.InflightLimit
	allowRetransmit := t.InFlight < t.InflightLimit || forceRetransmit

	for n := u16(0); n < t.NumBlocks; n++ {
		i := (t.NextSendIdx + n) % t.NumBlocks
		if bitmapTest(t.AckBitmap, i) {
			continue
		}

		if !t.sentOnce(i) && allowNew {
			sendIdx = i
			haveSend = true
			break
		}

		if t.sentOnce(i) && allowRetransmit {
			sendIdx = i
			haveSend = true
			break
		}
	}

	if !haveSend {
		return false
	}

	resent := t.sentOnce(sendIdx)
	if resent {
		t.stats.Retransmits++
	} else {
		bitmapSet(t.SentBitmap, sendIdx)
		t.InFlight++
		if t.InFlight > t.stats.MaxInFlight {
			t.stats.MaxInFlight = t.InFlight
		}
	}
	t.NextSendIdx = (sendIdx + 1) % t.NumBlocks

	t.stats.DataSent++

	offset := int(sendIdx) * int(t.BlockSize)
	end := offset + int(t.BlockSize)
	if end > len(t.Data) {
		end = len(t.Data)
	}

	payload := t.Data[offset:end]
	p.sendA2B(packet{encodeObjectData(t.ObjectIndex, t.ObjectGen, sendIdx, payload)})
	return true
}

func (t *tx) stepTick(now u32, p *pipe) {
	if t.State == TXSTATECONFIRMED {
		return
	}
	if t.isFullyAcked() {
		p.sendA2B(packet{encodeAckConfirm(t.ObjectIndex, t.ObjectGen)})
		t.State = TXSTATECONFIRMED
		return
	}

	t.maybeSendInfo(now, p)
	timedOut := t.checkTimeout(now)
	sent := t.maybeSendData(now, timedOut, p)

	if !sent && t.InFlight > 0 {
		t.State = TXSTATEAWAITINGACK
		return
	}

	t.State = TXSTATESENDING
}

func (t *tx) stepOnAck(prefix u16, ackBitmap []u8, now u32) {
	t.onAckBitmap(prefix, ackBitmap, now)
	if t.isFullyAcked() {
		t.State = TXSTATEAWAITINGACK
	} else if t.InFlight > 0 {
		t.State = TXSTATEAWAITINGACK
	} else {
		t.State = TXSTATESENDING
	}
}

// ============================================================
// UDP pipe simulator
// ============================================================

type packet struct{ Data []u8 }

type pipe struct {
	a2bSend chan packet
	b2aSend chan packet
	a2b     chan packet
	b2a     chan packet
	drop    float64
	reorder float64
	latMs   int
	jitMs   int

	stats *PipeStats
}

func newPipe(drop, reorder float64, latMs, jitMs int, stats *PipeStats) *pipe {
	p := &pipe{
		a2bSend: make(chan packet, 4096),
		b2aSend: make(chan packet, 4096),
		a2b:     make(chan packet, 4096),
		b2a:     make(chan packet, 4096),
		drop:    drop,
		reorder: reorder,
		latMs:   latMs,
		jitMs:   jitMs,
		stats:   stats,
	}
	go p.run(p.a2bSend, p.a2b)
	go p.run(p.b2aSend, p.b2a)
	return p
}

func (p *pipe) trySend(ch chan packet, pkt packet) bool {
	ch <- pkt
	return true
}

func (p *pipe) sendA2B(pkt packet) {
	p.trySend(p.a2bSend, pkt)
}

func (p *pipe) sendB2A(pkt packet) {
	p.trySend(p.b2aSend, pkt)
}

func (p *pipe) run(in, out chan packet) {
	for pkt := range in {
		p.stats.PacketsIn++

		if rand.Float64() < p.drop {
			p.stats.Dropped++
			continue
		}
		time.Sleep(time.Duration(p.latMs+rand.Intn(p.jitMs+1)) * time.Millisecond)

		if rand.Float64() < p.reorder {
			p.stats.Reordered++
			go func(pkt packet) {
				time.Sleep(2 * time.Millisecond)
				if p.trySend(out, pkt) {
					p.stats.PacketsOut++
				}
			}(pkt)
			continue
		}
		if p.trySend(out, pkt) {
			p.stats.PacketsOut++
		}
	}
}

func findTx(txs []*tx, objectIndex, objectGen u16) *tx {
	for _, txi := range txs {
		if txi.ObjectIndex == objectIndex && txi.ObjectGen == objectGen {
			return txi
		}
	}
	return nil
}

// ============================================================
// Main simulation
// ============================================================

func main() {
	var drop, reorder float64
	var lat, jit int
	var seed int64
	var ackEvery int
	var ackInterval int

	flag.Float64Var(&drop, "drop", 0, "drop probability")
	flag.Float64Var(&reorder, "reorder", 0, "reorder probability")
	flag.IntVar(&lat, "latency", 0, "latency ms")
	flag.IntVar(&jit, "jitter", 0, "jitter ms")
	flag.Int64Var(&seed, "seed", 1, "rng seed")
	flag.IntVar(&ackEvery, "ack-every", 4, "send ACK every N received data packets per object")
	flag.IntVar(&ackInterval, "ack-interval", 5, "send ACK at least every N ms when data was received")
	flag.Parse()

	if ackEvery < 1 {
		ackEvery = 1
	}
	if ackInterval < 0 {
		ackInterval = 0
	}

	rand.Seed(seed)

	stats := &Stats{}

	p := newPipe(drop, reorder, lat, jit, &stats.Pipe)
	r := newRx(&stats.Rx)

	numObjects := 3
	txs := make([]*tx, numObjects)

	for i := 0; i < numObjects; i++ {
		data := make([]u8, 4096)
		for j := range data {
			data[j] = u8(i ^ j)
		}
		t := newTx(&stats.Tx)
		t.start(u16(i), 1, data, 256)
		txs[i] = t
	}

	start := time.Now()

	for {
		now := u32(time.Since(start).Milliseconds())

		for _, t := range txs {
			t.stepTick(now, p)
		}

		for processed := 0; processed < 64; processed++ {
			handled := true

			select {
			case pkt := <-p.a2b:
				if ack, ok := r.step(pkt.Data, now, u16(ackEvery), u32(ackInterval)); ok {
					p.sendB2A(packet{ack})
				}

			case pkt := <-p.b2a:
				objIdx, objGen, prefix, ackBitmap, ok := decodeAck(pkt.Data)
				if !ok {
					break
				}

				t := findTx(txs, objIdx, objGen)
				if t == nil {
					break
				}

				t.stepOnAck(prefix, ackBitmap, now)

			default:
				handled = false
			}

			if !handled {
				break
			}
		}

		if stats.Rx.CompletedObjects == uint64(numObjects) {
			break
		}

		time.Sleep(1 * time.Millisecond)
	}

	elapsed := time.Since(start)
	elapsedSec := elapsed.Seconds()
	stats.Benchmark.ElapsedMs = u64(elapsed.Milliseconds())
	stats.Benchmark.ElapsedSeconds = elapsedSec
	stats.Benchmark.PacketsPerSecond = ratePerSecond(stats.Pipe.PacketsOut, elapsedSec)
	stats.Benchmark.DataPacketsPerSecond = ratePerSecond(stats.Tx.DataSent, elapsedSec)
	stats.Benchmark.RetransmitsPerSecond = ratePerSecond(stats.Tx.Retransmits, elapsedSec)
	stats.Benchmark.AckPacketsPerSecond = ratePerSecond(stats.Rx.AckPacketsSent, elapsedSec)
	stats.Benchmark.AckCoalescedPerSecond = ratePerSecond(stats.Rx.AckCoalesced, elapsedSec)

	out, _ := json.MarshalIndent(stats, "", "  ")
	fmt.Println(string(out))

}
