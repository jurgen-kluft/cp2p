
# UDP Object Build Protocol – Sender & Receiver


# Sender State Machine

- Notes:
  - It is Ok for the sender to 'wait' if that means reducing 'resent' blocks and improving overall efficiency. 
    For example: if the object to send has one block, the sender can send that block with ackReqLevel=1 and then 
    wait for the ACK before sending anything else, after a certain timeout, it can resend the block with ackReqLevel=2, and so on. The delay can backoff exponentially based on the number of resends. This allows the sender to adapt to the network conditions and avoid unnecessary resends, while still ensuring that the block is eventually delivered and acknowledged.
  - At the start of each round, compute dynamic G with exact rounding: `G = clamp(ceil(TotalBlocks / TargetBlocksPerSegment), 1, GMax)`.
  - At the start of each round, compute dynamic K with exact rounding: `K = max(ceil(NumberOfBlocksLeft / G), KMin)`.
  - Recompute G and K only on round boundaries (`STATE_SENDING` entry and `STATE_RESENDING` entry), not per packet.
  - Resend rounds operate on the live unacked set, not a frozen snapshot.
  - Last-block bump is evaluated against the current live set at send time.
  - K-based bump and last-block bump may both occur in close succession in v1.
  - Even when only one unacked block remains, retries continue to bump ackReqLevel.
  - Receiver-only sentinel: ackReqLevel == 0xFFFF in ACK means full object received.
  - Sender never transmits 0xFFFF; sender only consumes it.

### RTT and Timeout Policy

- RTT sample source: when an ACK with `ackReqLevel = L` is received, resolve the owning segment by direct lookup in the segment arrays (current, prev1, prev2), then use that segment end-time to produce the RTT sample.
- Segment arrays are the only RTT source; no separate ackReqLevel timestamp history is maintained.
- RTT estimators:
  - `SRTT = (7/8) * SRTT + (1/8) * sample` (or `SRTT = sample` for first sample)
  - `RTTVAR = (3/4) * RTTVAR + (1/4) * abs(sample - SRTT)` (or `RTTVAR = sample/2` for first sample)
- Base timeout:
  - `Tbase = SRTT + max(4 * RTTVAR, 5ms)`
  - If no RTT sample yet, use bootstrap `Tbase = 30ms`.
- Await timeout used in sender end/await states:
  - `Tawait(n) = clamp(Tbase * 2^n, 10ms, 400ms)`
  - `n` is the number of consecutive no-progress timeouts in the current await phase.
  - Reset `n = 0` on any ACK that newly acknowledges at least one block.

#### Direct Lookup Invariants

- Each segment array stores a dense and contiguous `ackReqLevel` range with explicit `beginAckReqLevel` and `endAckReqLevel`.
- Segment arrays must not overlap in `ackReqLevel` ownership at the same time.
- Direct lookup index is `idx = ackReqLevel - beginAckReqLevel` after range check.
- Direct lookup is valid only when `0 <= idx < segmentCount` and the slot is not EMPTY.
- If no owning segment is found, process ACK bitmap normally but skip RTT sampling for that ACK.
- Array rotation order is fixed: `prev2 = prev1`, `prev1 = current`, `current = prev2 (cleared)`.

#### Scheduler Model

- The sender state machine runtime provides per-state timer facilities.
- On state entry, the active state may register one or more timers.
- While active, a state may start, stop, or restart its timers.
- On timer expiry, the runtime dispatches `OnTimeOut(timerId)` to the current active state.
- On state exit, timers owned by that state are canceled automatically unless explicitly preserved by transition policy.
- `OnAckReceived` may update state immediately, independent of timer events.
- `OnTick` remains available for non-timeout periodic work (for example pacing checks and eligibility scans).

#### Pacing Policy

- Pacing is the minimum time spacing between sending one `OBJECT_BLOCK` packet and the next.
- The sender maintains `nextSendTime`, which is the earliest timestamp at which the next packet may be sent.
- A packet send is allowed only when `now >= nextSendTime` and inflight limits permit sending.
- After each packet send, update `nextSendTime = now + paceInterval`.
- `paceInterval` is computed from the current pacing rate as `paceInterval = 1000ms / pacingRatePps` and clamped to implementation bounds.
- `pacingRatePps` should increase gradually on ACK progress and decrease on timeout/no-progress windows.
- Pacing applies equally to first-send and resend traffic.

#### InFlight Control Policy

- `InFlight` is the number of sent data blocks that are not yet acknowledged.
- Increase `InFlight` when a new block is sent for the first time.
- Do not increase `InFlight` for retransmission of a block that is already counted as outstanding.
- Decrease `InFlight` only when an ACK newly acknowledges a previously unacked block.
- Maintain `InFlightLimit` as the maximum allowed outstanding blocks.
- A data packet send is allowed only when both are true: `now >= nextSendTime` and `InFlight < InFlightLimit`.
- `InFlightLimit` may increase gradually on ACK progress and should decrease on timeout/no-progress windows.
- InFlight control applies equally to first-send and resend traffic.

#### Tail Segment Await Behavior

- Within a round, the sender transmits up to K data packets before bumping ackReqLevel, and each such K-segment has an associated ackReqLevel and send-time window.
- If a round ends with fewer than K packets (including the single-packet case), the sender enters an await phase and waits for ACK progress before deciding to resend.
- The wait duration uses the RTT policy defined above: `Tawait(n) = clamp(Tbase * 2^n, 10ms, 400ms)`, where `n` is the consecutive no-progress timeout count.
- ACK progress means any ACK that newly acknowledges one or more blocks in the ACK bitmap. On progress, reset `n = 0`; on timeout without progress, increment `n` and proceed with retransmission.
- This keeps retransmissions efficient on delayed-ACK paths while still guaranteeing eventual completion.

#### Segment Age Gate for Resend Eligibility

- The sender tracks each K-segment with send-start time, send-end time, and whether the segment still has missing blocks.
- Segment duration is `Dseg = segmentEndTime - segmentStartTime`.
- A segment is eligible for resend only if all are true:
  - The segment still has missing blocks.
  - `now - segmentEndTime >= Gseg`.
  - No ACK progress has been observed for that segment during its grace window.
- Segment grace window is `Gseg = max(Tawait(n), 2 * Dseg)`.
- Segments with age less than `Gseg` are considered too recent and must not be retransmitted yet.
- Resend order should prefer oldest eligible segments first.

# Message Types

There are only 2 message types, so they are identified by just one bit in the object index field.

* `OBJECT_BLOCK`: Sent by sender, contains a block of the object data, along with object index, object generation, block index and the ackReqLevel. Also includes information to act as initialization for the object (e.g., total blocks) to allow receiver to reconstruct the object.
* `OBJECT_ACK`: Sent by receiver, contains the object index, object generation, block index being acknowledged, the ackReqLevel (`0 <= ackReqLevel < 65535`), bitmapStartBlockIndex and ACK bitmap (bitmap of received blocks).
In `OBJECT_ACK`, `AckReqLevel == 0xFFFF` is a special case that indicates receiver-complete for the same transfer identity. The sender must perform strict identity validation before acting on any ACK, including `0xFFFF`. Minimum identity match is `(objectIndex, objectGeneration)` and protocol-level transfer identity (for example object content hash or transfer id when available). If identity validation fails, ignore the ACK.

* `OBJECT_ACK_CONFIRM`: Sent by sender when it has received ACKs for all blocks, confirming that the entire object has been acknowledged by the receiver.

# States 

* `STATE_START`
* `STATE_SENDING`
* `STATE_SENDING_END`
* `STATE_SENDING_AWAITINGACK`
* `STATE_CONFIRMED`
* `STATE_ENDED`

## State Logic

`STATE_START`:
  Event(OnTick):
    - Initialize transfer context
    - Move to `STATE_SENDING`

`STATE_SENDING`:
  Event(OnTick):
    - Round number starts at 0, increment on each round start. Round 0 means that we are sending the first K blocks of the object, any round > 0 means we are resending some subset of the remaining unacked blocks.
    - Round robin through blocks once, respecting inflight limit and pacing.
    - A block send is allowed only if pacing policy allows it (`now >= nextSendTime`) and inflight limits are not exceeded.
    - For every K blocks sent, increase ackReqLevel. 
    - Before sending last block of this round, bump ackReqLevel
    - If we have sent all blocks, move to `STATE_SENDING_END`
  
  Event(OnAckReceived):
    - Validate ACK identity strictly; if invalid, ignore ACK.
    - If ACK ackReqLevel == '0xFFFF' and identity is valid, move to `STATE_CONFIRMED`
    - Process ACK data, if all blocks ACKed then move to `STATE_CONFIRMED`
    - Using ACK ackReqLevel and segment-array direct lookup, estimate RTT

`STATE_SENDING_END`:
  Event(OnTick):
    - Rotate segment arrays (prev2 = prev1, prev1 = current, current = prev2 emptied), reset current segment tracking
    - Set `n = 0`, compute `Tawait(n)`, start wait timer, and move to `STATE_SENDING_AWAITINGACK`.

`STATE_SENDING_AWAITINGACK`:
  Event(OnAckReceived):
    - Validate ACK identity strictly; if invalid, ignore ACK.
    - If ACK ackReqLevel == '0xFFFF' and identity is valid, move to `STATE_CONFIRMED`
    - Process ACK data, if all blocks ACKed then move to `STATE_CONFIRMED`
    - Using ACK ackReqLevel and segment-array direct lookup, estimate RTT
    - If ACK ackReqLevel is equal to our current ackReqLevel and at least one segment is resend-eligible, move to `STATE_SENDING`.
    - If ACK newly acknowledges one or more blocks, reset timeout exponent `n = 0`.
  
  Event(OnTimeOut):
    - If one or more segments are resend-eligible, bump ackReqLevel and move to `STATE_SENDING`.
    - If no segment is resend-eligible yet, remain in `STATE_SENDING_AWAITINGACK` and continue waiting.
    - If no new ACK progress during the previous wait window, set `n = n + 1`; otherwise keep `n = 0`.
    - Recompute timer as `Tawait(n)`.

`STATE_CONFIRMED`:
  Event(OnTick):
    - send `OBJECT_ACK_CONFIRM`
    - Move to `STATE_ENDED`

`STATE_ENDED`:
  Event(OnTick):
    - Clean up transfer context, log stats, etc.
    - Done

---

### ACK `ackReqLevel` Interpretation Rules (Normative)

1. An ACK with `ackReqLevel = L` semantically refers to sender data packets transmitted with `ackReqLevel <= L`. It MUST NOT be interpreted as acknowledging or implying progress for packets sent with `ackReqLevel > L`.
2. ACK progress is determined solely by whether the ACK bitmap newly acknowledges one or more previously unacknowledged blocks, independent of the `ackReqLevel` value.
3. ACKs with `ackReqLevel < highestAckReqLevelSeen` MAY be used for RTT sampling, provided a valid owning segment exists. The ACK bitmap carried by such ACKs MUST be ignored for block-acknowledgment purposes, as ACKs with higher `ackReqLevel` values are strictly more authoritative.
4. An ACK with `ackReqLevel > sender.currentAckReqLevel` is invalid and MUST be ignored in its entirety, including bitmap processing, RTT sampling, resend eligibility decisions, and state transitions.
5. The value `ackReqLevel == 0xFFFF` is reserved exclusively as a receiver-complete sentinel and is never transmitted by the sender.

### InFlight vs Resend Priority (Normative)

- The `InFlightLimit` applies equally to first transmissions and retransmissions.
- A sender MUST NOT transmit any data packet if `InFlight >= InFlightLimit`, even if resend-eligible segments exist.
- This rule prioritizes bounded receiver pressure and congestion safety over aggressive loss recovery.
- Recovery proceeds via ACK progress that reduces `InFlight`, followed by resend rounds when permitted.
- Implementations may dynamically tune `InFlightLimit` but MUST NOT allow resends to bypass it.

### Terminal Confirmation Behavior (Normative)

- When the receiver has received all blocks, it MUST send an `OBJECT_ACK` with `ackReqLevel == 0xFFFF` after strict identity validation.
- Upon receiving a valid `0xFFFF` ACK, the sender transitions to `STATE_CONFIRMED`, sends `OBJECT_ACK_CONFIRM`, and may then terminate the transfer without waiting for further receiver confirmation.
- `OBJECT_ACK_CONFIRM` is best-effort; loss of this message does not affect correctness.
- The receiver MUST NOT wait indefinitely for `OBJECT_ACK_CONFIRM` and SHOULD guard its final state with a timeout, after which it may safely release resources and assume transfer completion.

---

# Receiver State Machine

- Notes:
  - When receiver has received all blocks of an object, it should send an ACK with ackReqLevel=0xFFFF to confirm receipt of the object. This allows the sender to know that the entire object has been received and can stop resending any remaining blocks.

## States

* `STATE_RECEIVING`
* `STATE_RECEIVED`
* `STATE_PENDING`
* `STATE_DONE`

---

`STATE_RECEIVING`:

Event(OnObjectInfoReceived):
  - Validate object info identity strictly; if invalid, ignore packet
  - If object not initialized then initialize receive context based on the info in the object info packet (e.g., total blocks, bitmap size, etc.)
  - Otherwise, if object already initialized, ignore packet

Event(OnObjectDataReceived):
  - Validate data packet identity strictly; if invalid, ignore packet
  - If object initialized, do not initialize receive context based on the info in the data packet (e.g., total blocks, bitmap size, etc.). If object is not initialized, then initialize receive context based on the info in the data packet. 
  - If packet ackReqLevel in ackReqLevel-bitmap is marked as '0', then mark it as '1' and send `OBJECT_ACK` with `ackReqLevel` equal to the ackReqLevel of the received data packet and bitmapStartBlockIndex and ACK bitmap reflecting the newly received block
  - Process received block, update received-block-bitmap and if all blocks are now received, move to `STATE_RECEIVED`

`STATE_RECEIVED`:

Event(OnTick):
  - Send `OBJECT_ACK` with `ackReqLevel = 0xFFFF` to confirm receipt of the entire object
  - Move to `STATE_PENDING` with a timeout (200ms) to make sure `STATE_PENDING` will transition to `STATE_DONE` even if `OBJECT_ACK_CONFIRM` is lost

`STATE_PENDING`:

Event(OnObjectDataReceived):
  - Validate data packet identity strictly; if invalid, ignore packet
  - This packet MUST be a duplicate of an already received block, ignore it
  - If packet ackReqLevel in ackReqLevel-bitmap is marked as '0', then mark it as '1' and send `OBJECT_ACK` with `ackReqLevel = 0xFFFF`

Event(OnAckConfirmReceived):
  - Validate ACK identity strictly; if invalid, ignore
  - move to `STATE_DONE`

Event(OnTimeOut):
  - move to `STATE_DONE`

`STATE_DONE`:

