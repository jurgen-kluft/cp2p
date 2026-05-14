package main

import (
	"slices"
	"testing"
)

func TestSequenceMapPushHasRemoveAndSize(t *testing.T) {
	m := NewSequenceMap(128, SeqMapLowest, 1)

	if !m.Push(7) {
		t.Fatalf("expected first push to succeed")
	}
	if m.Push(7) {
		t.Fatalf("expected duplicate push to fail")
	}
	if m.Push(128) {
		t.Fatalf("expected out-of-range push to fail")
	}
	if !m.Has(7) {
		t.Fatalf("expected map to contain 7")
	}
	if m.Size() != 1 {
		t.Fatalf("expected size 1, got %d", m.Size())
	}

	m.Remove(7)
	if m.Has(7) {
		t.Fatalf("expected 7 to be removed")
	}
	if m.Size() != 0 {
		t.Fatalf("expected size 0, got %d", m.Size())
	}
}

func TestSequenceMapRemoveRangeAndRemoveAll(t *testing.T) {
	m := NewSequenceMap(32, SeqMapLowest, 1)
	for i := uint32(0); i < 10; i++ {
		m.Push(i)
	}

	m.RemoveRange(3, 7)
	for _, s := range []uint32{0, 1, 2, 7, 8, 9} {
		if !m.Has(s) {
			t.Fatalf("expected %d to remain", s)
		}
	}
	for _, s := range []uint32{3, 4, 5, 6} {
		if m.Has(s) {
			t.Fatalf("expected %d to be removed", s)
		}
	}

	m.RemoveAll()
	if m.Size() != 0 {
		t.Fatalf("expected size 0 after RemoveAll, got %d", m.Size())
	}
	if got := m.Pop(); got != -1 {
		t.Fatalf("expected pop -1 from empty map, got %d", got)
	}
}

func TestSequenceMapPopLowest(t *testing.T) {
	m := NewSequenceMap(64, SeqMapLowest, 1)
	for _, s := range []uint32{10, 2, 35, 6} {
		m.Push(s)
	}

	want := []int32{2, 6, 10, 35}
	for _, w := range want {
		if got := m.Pop(); got != w {
			t.Fatalf("expected pop %d, got %d", w, got)
		}
	}
	if got := m.Pop(); got != -1 {
		t.Fatalf("expected empty pop -1, got %d", got)
	}
}

func TestSequenceMapPopRoundRobin(t *testing.T) {
	m := NewSequenceMap(16, SeqMapRoundRobin, 1)
	for _, s := range []uint32{2, 5, 7} {
		m.Push(s)
	}

	if got := m.Pop(); got != 2 {
		t.Fatalf("expected first pop 2, got %d", got)
	}

	m.Push(1)
	m.Push(3)

	if got := m.Pop(); got != 3 {
		t.Fatalf("expected second pop 3, got %d", got)
	}
	if got := m.Pop(); got != 5 {
		t.Fatalf("expected third pop 5, got %d", got)
	}
	if got := m.Pop(); got != 7 {
		t.Fatalf("expected fourth pop 7, got %d", got)
	}
	if got := m.Pop(); got != 1 {
		t.Fatalf("expected fifth pop 1, got %d", got)
	}
}

func TestSequenceMapPopRandomRemovesAllMembers(t *testing.T) {
	m := NewSequenceMap(64, SeqMapRandom, 99)
	for _, s := range []uint32{4, 8, 15, 16, 23, 42} {
		m.Push(s)
	}

	got := make([]uint32, 0, 6)
	for m.Size() > 0 {
		v := m.Pop()
		if v < 0 {
			t.Fatalf("unexpected -1 while size > 0")
		}
		got = append(got, uint32(v))
	}

	slices.Sort(got)
	want := []uint32{4, 8, 15, 16, 23, 42}
	if !slices.Equal(got, want) {
		t.Fatalf("unexpected popped set, got %v want %v", got, want)
	}
}

func TestSequenceMapToSlice(t *testing.T) {
	// Empty map returns empty slice.
	m := NewSequenceMap(128, SeqMapLowest, 1)
	if got := m.ToSlice(); len(got) != 0 {
		t.Fatalf("expected empty slice from empty map, got %v", got)
	}

	// Values spanning a word boundary (word 0: bits 0-63, word 1: bits 64-127).
	for _, s := range []uint32{0, 1, 62, 63, 64, 65, 126, 127} {
		m.Push(s)
	}
	got := m.ToSlice()
	want := []uint32{0, 1, 62, 63, 64, 65, 126, 127}
	if !slices.Equal(got, want) {
		t.Fatalf("unexpected ToSlice output, got %v want %v", got, want)
	}

	// ToSlice must not mutate the map.
	if m.Size() != uint32(len(want)) {
		t.Fatalf("ToSlice mutated the map: size is now %d", m.Size())
	}

	// After removing some entries the slice reflects the updated state.
	m.Remove(63)
	m.Remove(64)
	got = m.ToSlice()
	want = []uint32{0, 1, 62, 65, 126, 127}
	if !slices.Equal(got, want) {
		t.Fatalf("unexpected ToSlice after remove, got %v want %v", got, want)
	}

	// maxSeq boundary: sequence equal to maxSeq must never appear.
	m2 := NewSequenceMap(128, SeqMapLowest, 1)
	m2.Push(127)
	slice := m2.ToSlice()
	if len(slice) != 1 || slice[0] != 127 {
		t.Fatalf("expected [127], got %v", slice)
	}
}

func TestSequenceMapSearchContiguesSetInvalidRange(t *testing.T) {
	m := NewSequenceMap(128, SeqMapLowest, 1)

	if seq, ok := m.SearchContiguesSet(10, 10); ok || seq != 0 {
		t.Fatalf("expected invalid empty range to return (0,false), got (%d,%t)", seq, ok)
	}

	if seq, ok := m.SearchContiguesSet(20, 10); ok || seq != 0 {
		t.Fatalf("expected invalid reversed range to return (0,false), got (%d,%t)", seq, ok)
	}

	if seq, ok := m.SearchContiguesSet(0, 129); ok || seq != 0 {
		t.Fatalf("expected out-of-range end to return (0,false), got (%d,%t)", seq, ok)
	}
}

func TestSequenceMapSearchContiguesSetAllSet(t *testing.T) {
	m := NewSequenceMap(128, SeqMapLowest, 1)
	for i := uint32(5); i < 73; i++ {
		m.Push(i)
	}

	seq, ok := m.SearchContiguesSet(5, 73)
	if !ok || seq != 72 {
		t.Fatalf("expected fully set range to return (72,true), got (%d,%t)", seq, ok)
	}
}

func TestSequenceMapSearchContiguesSetGapInHeadWord(t *testing.T) {
	m := NewSequenceMap(128, SeqMapLowest, 1)
	for _, seq := range []uint32{5, 6, 7, 9, 10, 11} {
		m.Push(seq)
	}

	got, ok := m.SearchContiguesSet(5, 12)
	if !ok || got != 11 {
		t.Fatalf("expected highest set seq 11 for partial head word, got (%d,%t)", got, ok)
	}
}

func TestSequenceMapSearchContiguesSetGapInMiddleWord(t *testing.T) {
	m := NewSequenceMap(192, SeqMapLowest, 1)
	for i := uint32(8); i < 140; i++ {
		if i == 96 {
			continue
		}
		m.Push(i)
	}

	got, ok := m.SearchContiguesSet(8, 140)
	if !ok || got != 127 {
		t.Fatalf("expected highest set seq 127 for middle-word gap, got (%d,%t)", got, ok)
	}
}

func TestSequenceMapSearchContiguesSetGapInTailWord(t *testing.T) {
	m := NewSequenceMap(160, SeqMapLowest, 1)
	for i := uint32(64); i < 100; i++ {
		if i == 98 {
			continue
		}
		m.Push(i)
	}

	got, ok := m.SearchContiguesSet(64, 100)
	if !ok || got != 99 {
		t.Fatalf("expected highest set seq 99 for tail-word gap, got (%d,%t)", got, ok)
	}
}

func TestSequenceMapMergeCloneSerializeDeserialize(t *testing.T) {
	a := NewSequenceMap(128, SeqMapLowest, 1)
	b := NewSequenceMap(128, SeqMapLowest, 2)
	for _, s := range []uint32{1, 3, 8} {
		a.Push(s)
	}
	for _, s := range []uint32{3, 5, 13} {
		b.Push(s)
	}

	a.Merge(b)
	merged := a.ToSlice()
	wantMerged := []uint32{1, 3, 5, 8, 13}
	if !slices.Equal(merged, wantMerged) {
		t.Fatalf("unexpected merged contents, got %v want %v", merged, wantMerged)
	}

	clone := a.Clone()
	clone.Remove(8)
	if !a.Has(8) {
		t.Fatalf("expected clone mutation not to affect original")
	}

	buf := a.Serialize()
	d := NewSequenceMap(128, SeqMapLowest, 3)
	if ok := d.Deserialize(buf); !ok {
		t.Fatalf("deserialize should succeed")
	}
	if !slices.Equal(d.ToSlice(), wantMerged) {
		t.Fatalf("unexpected deserialized contents, got %v want %v", d.ToSlice(), wantMerged)
	}

	wrongMax := NewSequenceMap(64, SeqMapLowest, 4)
	if wrongMax.Deserialize(buf) {
		t.Fatalf("deserialize should fail when maxSeq differs")
	}
	if d.Deserialize([]byte{1, 2, 3}) {
		t.Fatalf("deserialize should fail on short buffer")
	}
}
