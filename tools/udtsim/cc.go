package main

type CC interface {
	OnPacketSent(seq uint32, nowUs uint64)
	OnPacketReceived(seq uint32, nowUs uint64)
	OnAck(ackSeq uint32, nowUs uint64)
	OnLoss(seq uint32, lossCount uint32, nowUs uint64)
	OnTimeout(nowUs uint64)
	BudgetBeforeCongestion() uint32
	PacingTimeoutUs(nowUs uint64, lastTxUs uint64) uint64
}

type UDTCC struct {
	cwnd         uint32
	minCwnd      uint32
	maxCwnd      uint32
	inFlight     uint32
	pacingUs     uint64
	nextSendUs   uint64
	lastAckSeq   uint32
	slowStart    bool
	lossSinceRC  bool
	rcIntervalUs uint64
	lastRCUs     uint64
}

func NewUDTCC(initialCwnd uint32) *UDTCC {
	return &UDTCC{
		cwnd:         initialCwnd,
		minCwnd:      2,
		maxCwnd:      8192,
		pacingUs:     1,
		slowStart:    true,
		rcIntervalUs: 10000,
	}
}

func (c *UDTCC) OnPacketSent(_ uint32, nowUs uint64) {
	if c.inFlight < ^uint32(0) {
		c.inFlight++
	}
	c.nextSendUs = nowUs + c.pacingUs
}

func (c *UDTCC) OnPacketReceived(_ uint32, _ uint64) {}

func (c *UDTCC) OnAck(ackSeq uint32, nowUs uint64) {
	var acked uint32 = 0
	if ackSeq > c.lastAckSeq {
		acked = ackSeq - c.lastAckSeq
	}
	c.lastAckSeq = ackSeq
	if acked == 0 {
		if c.nextSendUs < nowUs {
			c.nextSendUs = nowUs
		}
		return
	}
	if acked >= c.inFlight {
		c.inFlight = 0
	} else {
		c.inFlight -= acked
	}
	if c.slowStart {
		// Aggressive slow start: increase cwnd by 2x the acked packets
		c.cwnd = clampU32(c.cwnd+acked*2, c.minCwnd, c.maxCwnd)
		if c.cwnd >= c.maxCwnd {
			c.slowStart = false
		}
		if c.pacingUs > 1 {
			step := uint64(acked) * 2
			if step > 64 {
				step = 64
			}
			if c.pacingUs > step {
				c.pacingUs -= step
			} else {
				c.pacingUs = 1
			}
		}
		return
	}
	if c.lastRCUs != 0 && (nowUs-c.lastRCUs) < c.rcIntervalUs {
		return
	}
	c.lastRCUs = nowUs
	if c.lossSinceRC {
		c.lossSinceRC = false
		return
	}
	step := (c.pacingUs >> 5) + 1
	if c.pacingUs > step {
		c.pacingUs -= step
	}
	if c.pacingUs == 0 {
		c.pacingUs = 1
	}
	cwndInc := uint32(1)
	if c.cwnd >= 1024 {
		cwndInc = c.cwnd >> 10
	}
	c.cwnd = clampU32(c.cwnd+cwndInc, c.minCwnd, c.maxCwnd)
}

func (c *UDTCC) OnLoss(_ uint32, _ uint32, nowUs uint64) {
	c.slowStart = false
	c.lossSinceRC = true
	if c.pacingUs < 100000 {
		c.pacingUs = clampU64((c.pacingUs*9+7)/8, 1, 100000)
	}
	reduced := c.cwnd - (c.cwnd >> 3)
	c.cwnd = clampU32(reduced, c.minCwnd, c.maxCwnd)
	c.nextSendUs = nowUs + c.pacingUs
}

func (c *UDTCC) OnTimeout(nowUs uint64) {
	c.slowStart = false
	c.lossSinceRC = true
	if c.cwnd > 1 {
		c.cwnd >>= 1
	}
	c.cwnd = clampU32(c.cwnd, c.minCwnd, c.maxCwnd)
	c.inFlight = 0
	c.pacingUs = clampU64(c.pacingUs<<1, 1, 100000)
	c.nextSendUs = nowUs + c.pacingUs
}

func (c *UDTCC) BudgetBeforeCongestion() uint32 {
	if c.inFlight >= c.cwnd {
		return 0
	}
	return c.cwnd - c.inFlight
}

func (c *UDTCC) PacingTimeoutUs(nowUs uint64, lastTxUs uint64) uint64 {
	minNext := lastTxUs + c.pacingUs
	next := c.nextSendUs
	if next < minNext {
		next = minNext
	}
	if next == 0 {
		return nowUs
	}
	return next
}

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
