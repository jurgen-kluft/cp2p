package main

func clampU32(v, lo, hi uint32) uint32 {
	if v < lo {
		return lo
	}
	if v > hi {
		return hi
	}
	return v
}

func clampU64(v, lo, hi uint64) uint64 {
	if v < lo {
		return lo
	}
	if v > hi {
		return hi
	}
	return v
}

func ccmulU32(a uint32, b uint32) uint32 {
	if a == 0 || b == 0 {
		return 0
	}
	if a > ^uint32(0)/b {
		return ^uint32(0)
	}
	return a * b
}

func clampUrgencyU32(v, lo, hi uint32) uint32 {
	if v < lo {
		return lo
	}
	if v > hi {
		return hi
	}
	return v
}
