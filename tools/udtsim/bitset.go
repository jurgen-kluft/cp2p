package main

import "math/bits"

// --------------------------------------------------------------------------------
// --- BitSet ---------------------------------------------------------------------
// --------------------------------------------------------------------------------

type BitSet struct {
	bits       []uint32
	usedCount  int32
	maxEntries int32
}

func NewBitSet(maxEntries uint32) BitSet {
	return BitSet{
		bits:       make([]uint32, (maxEntries+31)/32),
		usedCount:  0,
		maxEntries: int32(maxEntries),
	}
}

func (b *BitSet) Reset() {
	for i := range b.bits {
		b.bits[i] = 0
	}
	b.usedCount = 0
}

func (b *BitSet) FindFirstClear() (index uint32, found bool) {
	for i, word := range b.bits {
		if word != 0xFFFFFFFF {
			bit := uint32(bits.TrailingZeros32(^word))
			index = uint32(i)*32 + bit
			if index < uint32(b.maxEntries) {
				return index, true
			}
			return 0, false
		}
	}
	return 0, false
}

func (b *BitSet) RemoveFuncOnce(compare func(index uint32) bool) {
	for i, word := range b.bits {
		if word != 0 {
			bit := uint32(bits.TrailingZeros32(word))
			index := uint32(i)*32 + bit
			if index < uint32(b.maxEntries) && compare(index) {
				b.bits[i] &^= (1 << bit)
				b.usedCount--
				return
			}
		}
	}
}

func (b *BitSet) RemoveFuncMany(compare func(index uint32) bool) {
	for i, word := range b.bits {
		if word != 0 {
			for bit := uint32(0); bit < 32; bit++ {
				if (word & (1 << bit)) != 0 {
					index := uint32(i)*32 + bit
					if index < uint32(b.maxEntries) && compare(index) {
						b.bits[i] &^= (1 << bit)
						b.usedCount--
					}
				}
			}
		}
	}
}

func (b *BitSet) Set(index uint32) bool {
	if index >= uint32(b.maxEntries) {
		return false
	}
	wordIndex := index / 32
	bitIndex := index & 31
	if (b.bits[wordIndex] & (1 << bitIndex)) != 0 {
		return false // already set
	}
	b.bits[wordIndex] |= (1 << bitIndex)
	b.usedCount++
	return true
}

func (b *BitSet) Clear(index uint32) bool {
	if index >= uint32(b.maxEntries) {
		return false
	}
	wordIndex := index / 32
	bitIndex := index & 31
	if (b.bits[wordIndex] & (1 << bitIndex)) == 0 {
		return false // already clear
	}
	b.bits[wordIndex] &^= (1 << bitIndex)
	b.usedCount--
	return true
}

func (b *BitSet) IsSet(index uint32) bool {
	if index >= uint32(b.maxEntries) {
		return false
	}
	wordIndex := index / 32
	bitIndex := index & 31
	return (b.bits[wordIndex] & (1 << bitIndex)) != 0
}

func (b *BitSet) Count() int32 {
	return b.usedCount
}
