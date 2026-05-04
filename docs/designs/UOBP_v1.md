
# UDP Object Build Protocol (UOBP) v1

**Status:** Frozen specification  
**Version:** v1.0  
**Last updated:** 2026-04-30

UOBP is a reliable, ordered *object construction* protocol layered on top of UDP. Instead of providing a byte stream, UOBP deterministically builds fixed‑size objects at a remote endpoint using independently acknowledged data blocks.

---

## 1. Design Goals

- Reliable object delivery over UDP
- Deterministic reconstruction at receiver
- Idempotent data transfer
- Tolerant to loss, duplication, and reordering
- Embedded‑ and MCU‑friendly (ESP32‑class)
- No packet ordering or per‑packet ACKs

---

## 2. Object Identity

Each object is uniquely identified by the tuple:

```text
(object_index, object_gen)
```

- `object_index` identifies the logical object slot
- `object_gen` identifies the generation (version)

Packets with unknown or stale `(object_index, object_gen)` **MUST be ignored**.

---

## 3. Object Model

- Each object has a known total size (`object_size`)
- Data is divided into fixed‑size blocks (`block_size`)
- The final block **MAY** be smaller than `block_size`

The total number of blocks is computed as:

```text
num_blocks = ceil(object_size / block_size)
```

---

## 4. Message Types

All messages are carried directly in UDP payloads.

### 4.1 Object Data (`udp_obj_msg_t`)

**Receiver behavior:**

- Only if object hasn't been initialized yet, use 'object index', 'object gen', 'block count', and 'block size' to:
  - Request object memory and bitmap memory from the user layer
  - Zero‑initialize the received‑block bitmap before accepting data
  - Discard any previous object state with lower generation

If memory allocation for either the object buffer or bitmap fails, the receiver **MUST silently reject** the object and **MUST NOT emit ACKs** for it.


```c
struct udp_obj_msg_t
{
    u8  m_object_index;  // (bit 7 = 1) OBJECT_DATA
    u8  m_object_gen;    // generation of the object being built
    u16 m_block_count;   // zero‑based, total number of blocks in object
    u16 m_block_idx;     // zero‑based, block index of this payload
    u16 m_block_size;    // fixed block size in bytes (same as OBJECT_INFO)
    u16 m_block_len;     // payload length in bytes
    u16 m_hash;          // hash of payload (little‑endian)
    // u8 payload[m_block_len]
};
```

**Receiver behavior:**
- Validate object existence and generation
- Recompute and validate `hash` over the payload
- On hash mismatch: discard the block and do **not** mark it received
- Copy payload directly into user memory at:

```text
offset = m_block_idx * block_size
```

- Mark block as received **only after successful validation**
- Duplicate and out‑of‑order blocks are harmless

---

### 4.2 Object Acknowledgment (`udp_obj_ack_t`)

```c
struct udp_obj_ack_t
{
    u8  m_object_index;   // (bit 7 = 0) OBJECT_ACK
    u8  m_object_gen;     // generation of the object being built
    u8  m_symbol_rb[2];   // SRLEN run‑bits for 0 and 1
    u16 m_block_start;    // starting block index
    u16 m_ack_req_level;  // ACK request level
    u16 m_ack_len;        // length of compressed ack data in bytes
    // u8 m_ack_data[];   // SRLEN‑compressed bitmap
};
```

Receivers **MUST NOT** acknowledge a block until it has been fully validated and written into user memory.

ACK emission timing carries **no protocol meaning** and MAY be delayed arbitrarily.

---

## 5. ACK Bitmap Semantics

- The decoded SRLEN bitstream represents a contiguous bitmap window
- Bit `i` corresponds to block:

```text
block_index = m_block_start + i
```

**Bit meaning:**
- `1` — block present and validated
- `0` — block missing or invalid

ACK bitmaps represent **authoritative receiver truth**.

---

## 6. Reliability Model

- No per‑packet acknowledgments
- Retransmissions are driven exclusively by ACK bitmaps
- Loss, duplication, and reordering are fully tolerated
- Convergence is guaranteed as long as packets eventually flow

---

## 7. Completion

An object is complete when all blocks are marked received.

Completion detection is implicit when the ACK bitmap resolves all bits to `1`.

---

## 8. Fault Handling

- Stale or unknown generation: drop packet
- Hash validation failure: discard block
- Allocation failure: silently reject object
- Receiver restart: new generation restarts object build

---

## 9. Timing and Flow Control

- Receivers **SHOULD** emits an ACK for every unique 'ack req level' value
- ACK frequency is an implementation detail and **MUST NOT** be relied upon
- Senders **SHOULD** limit the number of outstanding unacknowledged blocks

---

## 10. Wire Format & Encoding Rules (Normative)

### 10.1 Endianness

All multi‑byte integer fields **MUST** be encoded in little‑endian byte order.

Example (`0x1234`):

```text
34 12
```

### 10.2 Alignment and Packing

- 64‑bit C ABI‑style packing rules apply
- Natural alignment is used
- Fields are serialized explicitly in declaration order
- Raw in‑memory structures **MUST NOT** be transmitted

### 10.3 Maximum UDP MTU

- Maximum assumed UDP MTU: **(1280+32) bytes**
- All UDP payloads **MUST** fit within this limit
- IP fragmentation **MUST NOT** be relied upon

### 10.4 SRLEN Bitstream Ordering

- SRLEN uses a byte accumulator
- Bits are written from **LSB to MSB**
- Byte accumulator flushes after 8 bits
- Bitstreams are byte‑aligned only at end

---

## 11. Zero‑Copy Object Construction and Validation

### 11.1 Object Memory Ownership

The **user layer owns all object memory and bitmap memory**.

- UOBP requests memory for both object buffer and block bitmap on `OBJECT_INFO`
- The bitmap **MUST be zero‑initialized** before any OBJECT_DATA is processed
- UOBP **MUST NOT** allocate, free, resize, or reallocate either buffer
- All memory **MUST** remain valid for the duration of the build

---

### 11.2 Direct Block Placement (Zero‑Copy)

Incoming blocks are written directly into user memory at:

```text
offset = block_index * block_size
```

No intermediate buffering or object‑level copying is performed.

---

### 11.3 Partial Object Visibility

Partial object visibility **is explicitly allowed and encouraged**.

The user layer **MAY**:
- Inspect partially constructed object memory
- Observe the received‑block bitmap
- Act on incremental object state

UOBP imposes no isolation on partially received objects.

---

### 11.4 Completion Notification

When all blocks are validated and received, the receiver **MUST** notify the user layer that the object is complete.

Ownership of the object memory remains with the user layer.

---

### 11.5 Abort Semantics

If an object build is aborted due to timeout, restart, or generation mismatch, the receiver **MUST** notify the user layer so that object and bitmap memory can be reclaimed.

---

### 11.6 Per‑Block Integrity Validation

Every `OBJECT_DATA` packet **MUST** include a **32‑bit hash (hash)** of the block payload.

Rules:
- The hash algorithm is implementation‑defined but **MUST** be consistent across sender and receiver
- The hash covers only the payload bytes
- Hash value is encoded little‑endian on the wire
- Receiver recomputes the hash after reception
- Blocks failing validation **MUST** be discarded
- Discarded blocks remain marked as missing and are retransmitted

Only **validated** blocks **MAY** be acknowledged.

---

## 12. Normative Flow‑Control Model

### 12.1 Flow‑Control Unit

Flow control in UOBP operates in units of **blocks**, not bytes or packets.

Each block represents an independently validated unit of receiver capacity.

---

### 12.2 In‑Flight Block Window

The sender **MUST** maintain a limit on the number of **in‑flight blocks** (sent but not yet acknowledged):

```text
MIN_INFLIGHT <= inflight_limit <= MAX_INFLIGHT
```

Exceeding `inflight_limit` is a protocol violation.

---

### 12.3 Receiver‑Driven Credits

ACK bitmaps represent authoritative receiver acceptance status:

- Each acknowledged block consumes one unit of receiver credit
- Only validated blocks grant credit
- Absence of ACKs **MUST** be interpreted as lack of credit

No implicit or inferred credit is permitted.

---

### 12.4 Dynamic Adjustment Rules

Senders **SHOULD** adapt `inflight_limit` dynamically:

- **Increase (slow start):** increment by a small constant (e.g. +1) when new blocks are acknowledged, up to `MAX_INFLIGHT`
- **Decrease (fast backoff):** reduce (typically halve) `inflight_limit` if no new blocks are acknowledged within `ACK_TIMEOUT`, but never below `MIN_INFLIGHT`

---

### 12.5 Interaction with UDP Socket State

UDP socket writability or kernel send‑buffer availability **MUST NOT** be used as a flow‑control signal.

Socket backpressure (e.g. `EAGAIN` / `EWOULDBLOCK`) **MAY** be used only as a local pacing hint and **MUST NOT** affect protocol state or credit calculations.

---

### 12.6 Correctness Guarantees

Under this flow‑control model:

- Receiver memory usage is bounded
- Sender behavior adapts to receiver and network conditions
- No head‑of‑line blocking occurs
- Protocol correctness does not depend on timing assumptions

---

*End of UOBP v1 Specification*
