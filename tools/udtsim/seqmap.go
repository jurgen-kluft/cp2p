package main

import (
	"encoding/binary"
	"math/bits"
	"math/rand"
)

type SeqMapMode int

const (
	SeqMapLowest SeqMapMode = iota
	SeqMapRoundRobin
	SeqMapRandom
)

type SequenceMap struct {
	bits     []uint64
	maxSeq   uint32
	setCount uint32
	minSet   uint32
	maxSet   uint32
	cursor   uint32
	mode     SeqMapMode
	rnd      *rand.Rand
}

func NewSequenceMap(maxSeq uint32, mode SeqMapMode, seed int64) *SequenceMap {
	words := (maxSeq + 63) / 64
	return &SequenceMap{
		bits:   make([]uint64, words),
		maxSeq: maxSeq,
		minSet: maxSeq,
		maxSet: 0,
		mode:   mode,
		rnd:    rand.New(rand.NewSource(seed)),
	}
}

func (m *SequenceMap) inRange(seq uint32) bool {
	return seq < m.maxSeq
}

func (m *SequenceMap) Has(seq uint32) bool {
	if !m.inRange(seq) {
		return false
	}
	w := seq >> 6
	b := seq & 63
	return ((m.bits[w] >> b) & 1) == 1
}

func (m *SequenceMap) Push(seq uint32) bool {
	if !m.inRange(seq) {
		return false
	}
	w := seq >> 6
	b := seq & 63
	mask := uint64(1) << b
	if (m.bits[w] & mask) != 0 {
		return false
	}
	m.bits[w] |= mask
	m.setCount++
	if seq < m.minSet {
		m.minSet = seq
	}
	if seq > m.maxSet {
		m.maxSet = seq
	}
	return true
}

func (m *SequenceMap) Remove(seq uint32) {
	if !m.inRange(seq) {
		return
	}
	w := seq >> 6
	b := seq & 63
	mask := uint64(1) << b
	if (m.bits[w] & mask) == 0 {
		return
	}
	m.bits[w] &^= mask
	m.setCount--
}

func (m *SequenceMap) RemoveRange(start, end uint32) {
	if start >= end || start >= m.maxSeq {
		return
	}
	if end > m.maxSeq {
		end = m.maxSeq
	}

	startWord := start >> 6
	endWord := (end - 1) >> 6
	for w := startWord; w <= endWord; w++ {
		word := m.bits[w]
		if word == 0 {
			continue
		}

		firstBit := uint32(0)
		lastBit := uint32(63)
		if w == startWord {
			firstBit = start & 63
		}
		if w == endWord {
			lastBit = (end - 1) & 63
		}

		width := lastBit - firstBit + 1
		mask := ((uint64(1) << width) - 1) << firstBit
		toRemove := word & mask
		if toRemove == 0 {
			continue
		}

		m.bits[w] = word &^ mask
		m.setCount -= uint32(bits.OnesCount64(toRemove))
	}
}

func (m *SequenceMap) RemoveAll() {
	if m.setCount == 0 {
		m.cursor = 0
		return
	}

	startWord := m.minSet >> 6
	endWord := m.maxSet >> 6
	if endWord >= uint32(len(m.bits)) {
		endWord = uint32(len(m.bits) - 1)
	}
	for i := startWord; i <= endWord; i++ {
		m.bits[i] = 0
	}
	m.setCount = 0
	m.minSet = m.maxSeq
	m.maxSet = 0
	m.cursor = 0
}

func (m *SequenceMap) Pop() int32 {
	if m.setCount == 0 || m.maxSeq == 0 {
		return -1
	}
	switch m.mode {
	case SeqMapRoundRobin:
		if seq, ok := m.findNextSetFrom(m.cursor); ok {
			m.clearBit(seq)
			m.cursor = (seq + 1) % m.maxSeq
			return int32(seq)
		}
		return -1
	case SeqMapRandom:
		rangeSize := m.maxSeq
		if m.minSet <= m.maxSet {
			rangeSize = (m.maxSet - m.minSet) + 1
		}
		randStart := uint32(m.rnd.Int63n(int64(rangeSize)))
		if m.minSet <= m.maxSet {
			randStart += m.minSet
		}
		if seq, ok := m.findNextSetFrom(randStart); ok {
			m.clearBit(seq)
			return int32(seq)
		}
		return -1
	default:
		if seq, ok := m.findNextSetFrom(0); ok {
			m.clearBit(seq)
			return int32(seq)
		}
		return -1
	}
}

func (m *SequenceMap) Merge(src *SequenceMap) {
	n := len(m.bits)
	if len(src.bits) < n {
		n = len(src.bits)
	}
	for i := 0; i < n; i++ {
		before := m.bits[i]
		merged := before | src.bits[i]
		if merged != before {
			diff := merged &^ before
			m.bits[i] = merged
			m.setCount += uint32(popcnt64(diff))
		}
	}
}

func (m *SequenceMap) Size() uint32 { return m.setCount }

func (m *SequenceMap) ToSlice() []uint32 {
	out := make([]uint32, 0, m.setCount)
	for w := uint32(0); w < uint32(len(m.bits)); w++ {
		word := m.bits[w]
		if word == 0 {
			continue
		}

		if w == uint32(len(m.bits)-1) {
			usedBits := m.maxSeq & 63
			if usedBits != 0 {
				word &= (uint64(1) << usedBits) - 1
			}
		}

		for word != 0 {
			bit := uint32(bits.TrailingZeros64(word))
			out = append(out, (w<<6)+bit)
			word &= word - 1
		}
	}
	return out
}

func (m *SequenceMap) Clone() *SequenceMap {
	cp := NewSequenceMap(m.maxSeq, m.mode, 1)
	copy(cp.bits, m.bits)
	cp.setCount = m.setCount
	cp.cursor = m.cursor
	return cp
}

func (m *SequenceMap) Serialize() []byte {
	seqs := m.ToSlice()
	out := make([]byte, 8+len(seqs)*4)
	binary.LittleEndian.PutUint32(out[0:4], m.maxSeq)
	binary.LittleEndian.PutUint32(out[4:8], uint32(len(seqs)))
	off := 8
	for _, s := range seqs {
		binary.LittleEndian.PutUint32(out[off:off+4], s)
		off += 4
	}
	return out
}

func (m *SequenceMap) Deserialize(buf []byte) bool {
	if len(buf) < 8 {
		return false
	}
	maxSeq := binary.LittleEndian.Uint32(buf[0:4])
	count := binary.LittleEndian.Uint32(buf[4:8])
	if maxSeq != m.maxSeq {
		return false
	}
	need := 8 + int(count)*4
	if len(buf) < need {
		return false
	}
	m.RemoveAll()
	off := 8
	for i := uint32(0); i < count; i++ {
		s := binary.LittleEndian.Uint32(buf[off : off+4])
		off += 4
		m.Push(s)
	}
	return true
}

func popcnt64(x uint64) int {
	return bits.OnesCount64(x)
}

func (m *SequenceMap) clearBit(seq uint32) {
	w := seq >> 6
	b := seq & 63
	mask := uint64(1) << b
	if (m.bits[w] & mask) == 0 {
		return
	}
	m.bits[w] &^= mask
	m.setCount--
}

func (m *SequenceMap) findNextSetFrom(start uint32) (uint32, bool) {
	if m.setCount == 0 || m.maxSeq == 0 {
		return 0, false
	}

	if start >= m.maxSeq {
		start = 0
	}

	wordCount := uint32(len(m.bits))
	startWord := start >> 6
	startBit := start & 63

	for pass := 0; pass < 2; pass++ {
		word := startWord
		for scanned := uint32(0); scanned < wordCount; scanned++ {
			w := m.bits[word]
			if w != 0 {
				if word == startWord {
					w &= ^uint64(0) << startBit
				}
				if word == wordCount-1 {
					usedBits := m.maxSeq & 63
					if usedBits != 0 {
						w &= (uint64(1) << usedBits) - 1
					}
				}
				if w != 0 {
					bit := uint32(bits.TrailingZeros64(w))
					seq := (word << 6) + bit
					if seq < m.maxSeq {
						return seq, true
					}
				}
			}
			word++
			if word == wordCount {
				word = 0
			}
		}

		startWord = 0
		startBit = 0
	}

	return 0, false
}
