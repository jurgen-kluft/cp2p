package main

// --------------------------------------------------------------------------------
// --- Sequence Map Window --------------------------------------------------------
// --------------------------------------------------------------------------------

// Sequence Map:
// A sequence map is a bitmap that identifies acknowledged packets, combined with a
// sequence map that identifies missing packets, the sender is able to determine which
// packets have been received and which are missing, and can retransmit missing packets
// as needed.
// In UDX, a session is responsible of sending data packets range by [0, maxSeqNum), where
// maxSeqNum is determined by the block size and size of the object that needs to be sent.
// Example: A 1 MiB object with a block size of 1 KiB would have a maxSeqNum of 1024.

// Sequence Map Window:
// In UDX the sender missing sequence map is initialized as all packets empty, and the
// sender also maintains a in-flight sequence map that tracks which packets have been
// transmitted but not yet acknowledged. The first round of transmissions, the sender
// transmits packets from 0 to maxSeqNum, and marks those packets as in-flight in the
// in-flight sequence map as well as building the first ladder for the Sequence Map Window.
// Any further rounds of transmissions, the sender goes through the Sequence Map Window.
// At the receiver side, the receiver maintains a received sequence map that tracks
// which packets have been received, and by sending ACKs the receiver acknowledges packets,
// the sender updates both the sequence maps to mark those packets as received.
// This allows the sender to efficiently track which packets/ need to be retransmitted and
// which have been successfully delivered.
// The sender is going in a round-robin manner through the missing sequence map, and for
// each missing packet, it will (re)transmit that packet and then move on to the next
// missing packet in the map.
// When the sender reaches the end of the missing sequence map, before it basically
// restarts from the beginning of the map, it rotates the current ladder with the next
// free ladder, and then resets this ladder to start the new round of transmissions.
// This way, the sender is able to efficiently manage the missing sequence map and
// ensure that all packets are eventually delivered, while also allowing for efficient
// retransmissions of missing packets.
//
// So why are we using this ladder structure per round-robin cycle?
//
// In the ladder structure we are grouping N packets together in a rung, and a rung holds
// the following fields:
//     - start sequence number
//     - end sequence number
//     - number of packets transmitted
//     - time of the last sequence number that was transmitted in this rung
//     - packet pacing during the construction of this rung
//
// So the main reason for the ladder structure is this, the end-time of a rung tells the
// sender if that rung has the chance of still being acknowledged. So if the current time
// of the sender is greater than rung-end-time + factor*RTT, then the sender can be sure
// that any packet in that rung that exists in the missing sequence map is very likely to
// be missing at the receiver, and thus the sender can prioritize retransmitting those
// packets in that rung, instead of retransmitting packets in a more recent rung that might
// still be acknowledged by the receiver.
// So in practise this acts as a flow control mechanism, where the sender is blocked by
// processing a rung because of the end-time of that rung, it will have to wait until the
// current-time + factor*RTT is greater than the end-time of that rung, before it can move
// on to process that rung.

// Exact send priority rule:
// Round 1 of transmissions is only new data, no retransmissions, so the sender just goes
// through the sequence map in order and transmits packets from 0 to maxSeqNum.
// Round > 1 are always retransmissions, so the sender goes through the sequence map in a
// round-robin manner, and for each missing packet it finds, it will (re)transmit that packet
// and then move on to the next missing packet in the map.

// Eligibility rule per rung: if nowUs >= rungEndUs + reorderFactor times srttUs.

// ACK-driven cleanup:
//   When ACK bitmap marks packets received, remove them from missing immediately.
// RTT robustness:
//   Use smoothed RTT plus variance for rung gate and timeout safety, not raw sample only.
// Dead-rung prevention:
//   If a rung has no remaining missing packets, skip it in the round-robin cursor.
// Multi-loss epoch handling:
//   Apply one congestion backoff per recovery epoch, not per missing packet.
// ACK-driven retransmissions:
//   Even though ACKs are part of the receiver policy to send at a certain interval, the
//   sender will eventually go to the next round and will send retransmissions of missing
//   packets, so the sender is NOT relying on ACKs to trigger retransmissions, but it mainly
//   relies on the round-robin cycle through the missing sequence map, and the ladder structure
//   to order retransmissions of older missing packets.
