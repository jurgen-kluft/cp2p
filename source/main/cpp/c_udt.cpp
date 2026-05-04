#include "ccore/c_target.h"
#include "ccore/c_memory.h"

#include "cp2p/udt/c_udt.h"

namespace ncore
{
    namespace nudt
    {
        // -----------------------------------------------------------------------------
        // UDT CORE INVARIANTS
        // -----------------------------------------------------------------------------
        //
        // Architectural Scope
        // -------------------
        // 1. A udt_t instance corresponds to exactly ONE object generation.
        //    A udt_t instance MUST NOT span multiple object generations.
        //    On object generation mismatch, the owning layer MUST discard the
        //    udt_t instance and create a new one.
        //
        // 2. UDT is a transport engine operating solely on sequence numbers.
        //    UDT has no knowledge of object layout, block semantics, hashes,
        //    payload content, or wire format.
        //
        //
        // Sequence Identity and Lifetime
        // ------------------------------
        // 3. Each sequence number uniquely identifies a logical data unit
        //    within the lifetime of a single udt_t instance.
        //
        // 4. A retransmission MUST reuse the same sequence number as the
        //    original transmission. UDT MUST NOT allocate a new sequence
        //    number for retransmitted data.
        //
        // 5. Sequence numbers are scoped to the range [0, m_max_seq).
        //    UDT MUST NOT schedule transmissions outside this range.
        //
        //
        // Sequence Map Semantics
        // ----------------------
        // 6. m_tx_missing_map represents the set of sequences known to be
        //    missing and eligible for retransmission.
        //
        // 7. A sequence MUST be removed from m_tx_missing_map once it is
        //    selected for retransmission scheduling via pop().
        //
        // 8. The ordering and fairness of retransmission scheduling are
        //    implementation-defined by sequence_ops_t::pop(). UDT imposes
        //    no ordering guarantees but relies on the implementation to
        //    avoid starvation.
        //
        // 9. UDT MUST NOT infer missing sequences implicitly; missing
        //    sequences enter m_tx_missing_map only via explicit loss
        //    signaling (e.g. NAK) or receiver-side gap detection.
        //
        //
        // ACK and NAK Processing
        // ----------------------
        // 10. ACK processing conveys cumulative receiver progress and
        //     authoritative flow control information.
        //
        // 11. NAK processing conveys loss information and authoritative
        //     flow control information.
        //
        // 12. On receipt of a NAK, UDT MUST merge the provided sequence map
        //     into m_tx_missing_map using sequence_ops_t::merge(). The user
        //     MUST NOT perform this merge on UDT’s behalf.
        //
        //
        // Transmission Scheduling
        // -----------------------
        // 13. If m_tx_missing_map is non-empty, retransmissions MUST take
        //     precedence over sending new sequence numbers.
        //
        // 14. UDT MUST NOT schedule any transmission if the number of
        //     in-flight packets (m_tx_in_flight) is greater than or equal to
        //     the advertised receiver flow window (m_rx_flow_window).
        //
        // 15. Congestion control further constrains scheduling. UDT MUST
        //     respect the send budget returned by cc_ops_t::get_send_budget().
        //
        //
        // Receiver-Side Semantics
        // -----------------------
        // 16. Receiving DATA for a sequence already present in
        //     m_rx_received_map MUST be idempotent and produce no side effects.
        //
        // 17. Receiver-side sequence state MUST NOT influence sender-side
        //     retransmission scheduling except through explicit ACK/NAK events.
        //
        //
        // Reset and Reinitialization
        // --------------------------
        // 18. On udt_t initialization, all internal counters and state
        //     variables are assumed to start from a clean state.
        //
        // 19. If udt_t instances are recreated per generation (the required
        //     model), sequence_ops_t::remove_all() is optional and not required
        //     for correctness.
        //
        //
        // Ownership and Memory
        // --------------------
        // 20. UDT does not own packet memory, sequence maps, or congestion
        //     control state. All such resources are owned and managed by the
        //     user or UOBP layer.
        //
        // 21. UDT MUST NOT allocate, free, or reinterpret memory passed to it;
        //     it may only access memory through explicitly provided callbacks.
        //
        // -----------------------------------------------------------------------------
        // End of UDT core invariants
        // -----------------------------------------------------------------------------

        void init_config(udt_config_t* config)
        {
            ASSERT(config != nullptr);

            // UDT timestamps are expressed in microseconds in the paper. We bootstrap
            // with the same practical defaults used by the reference implementation:
            // 10 ms ACK pacing, 300 ms initial EXP/RTO, and a 25,600-packet flow window.
            config->m_tx_max_seq              = 0x000FFFFFu;
            config->m_tx_initial_exp_interval = 300000u;
            config->m_tx_initial_flow_window  = 25600u;

            config->m_rx_initial_flow_window = 25600u;
            config->m_rx_ack_interval        = 10000u;
            config->m_rx_nak_interval        = 300000u;
            config->m_rx_exp_interval        = 300000u;
        }

        // -----------------------------------------------------------------------------
        // init
        // -----------------------------------------------------------------------------
        void init(udt_t* udt, void* user_ctx, const packet_ops_t* pkt_ops, const sequence_ops_t* seq_ops, const sequence_maps_t* seq_maps, void* cc_ctx, const cc_ops_t* cc_ops, const udt_config_t* config)
        {
            ASSERT(udt != nullptr && user_ctx != nullptr && config != nullptr);

            // Validate that packet ops are provided for all packet types.
            ASSERT(pkt_ops != nullptr);
            ASSERT(pkt_ops->m_build_data != nullptr);
            ASSERT(pkt_ops->m_build_ack != nullptr);
            ASSERT(pkt_ops->m_build_nak != nullptr);
            ASSERT(pkt_ops->m_send_packet != nullptr);

            // Validate that sequence ops are provided for all maps.
            ASSERT(seq_ops != nullptr);
            ASSERT(seq_ops->push != nullptr);
            ASSERT(seq_ops->remove != nullptr);
            ASSERT(seq_ops->remove_range != nullptr);
            ASSERT(seq_ops->has != nullptr);
            ASSERT(seq_ops->remove_all != nullptr);
            ASSERT(seq_ops->pop != nullptr);
            ASSERT(seq_ops->merge != nullptr);
            ASSERT(seq_ops->size != nullptr);

            // Validate that all sequence maps are provided.
            ASSERT(seq_maps != nullptr);
            ASSERT(seq_maps->m_tx_in_flight_map != nullptr);
            ASSERT(seq_maps->m_tx_missing_map != nullptr);
            ASSERT(seq_maps->m_rx_received_map != nullptr);
            ASSERT(seq_maps->m_rx_missing_map != nullptr);

            // Validate that all required callbacks are provided.
            ASSERT(cc_ctx != nullptr);
            ASSERT(cc_ops != nullptr);
            ASSERT(cc_ops->on_packet_sent != nullptr);
            ASSERT(cc_ops->on_packet_received != nullptr);
            ASSERT(cc_ops->on_ack != nullptr);
            ASSERT(cc_ops->on_loss != nullptr);
            ASSERT(cc_ops->on_timeout != nullptr);
            ASSERT(cc_ops->budget_before_congestion != nullptr);
            ASSERT(cc_ops->pacing_timeout_ts != nullptr);

            g_memset(udt, 0, sizeof(udt_t));

            udt->m_user_ctx = user_ctx;
            udt->m_pkt_ops  = const_cast<packet_ops_t*>(pkt_ops);
            udt->m_seq_ops  = const_cast<sequence_ops_t*>(seq_ops);
            udt->m_seq_maps = const_cast<sequence_maps_t*>(seq_maps);
            udt->m_cc_ctx   = cc_ctx;
            udt->m_cc_ops   = const_cast<cc_ops_t*>(cc_ops);

            // TX parameters
            udt->m_tx_next_seq    = 0;  // RX expects the first sequence to be 0
            udt->m_tx_last_acked  = 0;
            udt->m_tx_state       = TX_STATE_ACTIVE;
            udt->m_tx_max_seq     = config->m_tx_max_seq;
            udt->m_tx_flow_window = config->m_tx_initial_flow_window;

            udt->m_tx_last_send_ts   = 0;
            udt->m_tx_last_ack_ts    = 0;
            udt->m_tx_exp_interval   = config->m_tx_initial_exp_interval;
            udt->m_tx_exp_timeout_ts = 0;

            // RX parameters
            udt->m_rx_highest_contig = 0xFFFFFFFF;  // Start from 0xFFFFFFFF, so that first expected sequence is 0xFFFFFFFF + 1 = 0
            udt->m_rx_flow_window    = config->m_rx_initial_flow_window;
            udt->m_rx_ack_interval   = config->m_rx_ack_interval;
            udt->m_rx_nak_interval   = config->m_rx_nak_interval;
            udt->m_rx_exp_interval   = config->m_rx_exp_interval;
        }

        u64 tx_next_tick_ts(udt_t* udt, u64 now_ts)
        {
            // If we are currently in WAIT state, the next tick should be at the expiration timeout
            if (udt->m_tx_state == TX_STATE_WAIT)
                return udt->m_tx_exp_timeout_ts;

            // CC pacing timeout ts

            const u64 pacing_timeout_ts = udt->m_cc_ops->pacing_timeout_ts(udt->m_cc_ctx, now_ts, udt->m_tx_last_send_ts);
            if (pacing_timeout_ts > 0)
                return pacing_timeout_ts;

            // If no specific timeout is required, return 0 to indicate immediate tick
            return 0;
        }

        // -----------------------------------------------------------------------------
        // tx_tick, return the next timestamp at which tx_tick should be called again.
        // Events related to on_tx_ack_received and on_tx_nak_received may cause immediate
        // state changes that require tx_tick to be called again without waiting for the
        // next scheduled timestamp, so these event handlers return void and do not return
        // a timestamp.
        // -----------------------------------------------------------------------------
        void tx_tick(udt_t* udt, u64 now_ts)
        {
            // latched WAIT
            if (udt->m_tx_state == TX_STATE_WAIT)
            {
                if (now_ts < udt->m_tx_exp_timeout_ts)
                    return;

                // On timeout, we assume the worst case and treat all in-flight packets as lost,
                // moving them to the missing map for retransmission scheduling.
                // This is to ensure forward progress even in the absence of explicit loss signals
                // (e.g. NAKs) from the receiver.
                if (udt->m_seq_ops->size(udt->m_seq_maps->m_tx_missing_map) == 0)
                {
                    udt->m_seq_ops->merge(udt->m_seq_maps->m_tx_missing_map, udt->m_seq_maps->m_tx_in_flight_map);
                }

                udt->m_tx_state = TX_STATE_ACTIVE;

                udt->m_cc_ops->on_timeout(udt->m_cc_ctx, now_ts);
            }

            // Congestion control
            if (udt->m_cc_ops && udt->m_cc_ops->budget_before_congestion(udt->m_cc_ctx) == 0)
            {
                udt->m_tx_state          = TX_STATE_WAIT;
                udt->m_tx_exp_timeout_ts = now_ts + udt->m_tx_exp_interval;
                return;
            }

            // Pacing / Send rate
            const u64 pacing_timeout_ts = udt->m_cc_ops->pacing_timeout_ts(udt->m_cc_ctx, now_ts, udt->m_tx_last_send_ts);
            if (now_ts < pacing_timeout_ts)
            {
                return;
            }

            // Retransmissions
            if (udt->m_seq_ops->size(udt->m_seq_maps->m_tx_missing_map) > 0)
            {
                const i32 seq = udt->m_seq_ops->pop(udt->m_seq_maps->m_tx_missing_map);
                if (seq >= 0)
                {
                    packet_t pkt;
                    if (udt->m_pkt_ops->m_build_data(udt->m_user_ctx, (u32)seq, &pkt))
                    {
                        udt->m_cc_ops->on_packet_sent(udt->m_cc_ctx, (u32)seq, now_ts);

                        udt->m_pkt_ops->m_send_packet(udt->m_user_ctx, &pkt);

                        udt->m_tx_last_send_ts = now_ts;

                        return;
                    }
                    else
                    {
                        udt->m_seq_ops->push(udt->m_seq_maps->m_tx_missing_map, (u32)seq);
                    }
                }
            }
            else if (udt->m_tx_next_seq < udt->m_tx_max_seq)
            {
                // Flow control does apply to new data packet transmissions
                const u32 current_in_flight = udt->m_seq_ops->size(udt->m_seq_maps->m_tx_in_flight_map);
                if (current_in_flight < udt->m_tx_flow_window)
                {
                    packet_t pkt;
                    if (udt->m_pkt_ops->m_build_data(udt->m_user_ctx, udt->m_tx_next_seq, &pkt))
                    {
                        // Note that we update in-flight count and last progress timestamp only on
                        // when we are sending a new data packet, not on retransmissions. This is
                        // to ensure that the retransmission timeout is based on new transmissions
                        // and ACKs, not on retransmission scheduling which may be bursty due to
                        // ACK/NAK events.
                        udt->m_seq_ops->push(udt->m_seq_maps->m_tx_in_flight_map, udt->m_tx_next_seq);
                        udt->m_cc_ops->on_packet_sent(udt->m_cc_ctx, udt->m_tx_next_seq, now_ts);
                        udt->m_pkt_ops->m_send_packet(udt->m_user_ctx, &pkt);

                        udt->m_tx_last_send_ts = now_ts;
                        udt->m_tx_next_seq += 1;
                        return;
                    }
                }
            }

            // If we have no new data to send and no retransmissions to make, we can enter WAIT state
            // until the next ACK/NAK event or timeout that opens up the flow window or adds new
            // retransmissions.
            udt->m_tx_state          = TX_STATE_WAIT;
            udt->m_tx_exp_timeout_ts = now_ts + udt->m_tx_exp_interval;
        }

        // -----------------------------------------------------------------------------
        // on_tx_ack_received
        // -----------------------------------------------------------------------------
        void on_tx_ack_received(udt_t* udt, u32 ack_seq, u32 flow_window, u64 now_ts)
        {
            // ACK value must not acknowledge beyond what has been sent.
            if (ack_seq > udt->m_tx_next_seq)
            {
                udt->m_tx_state = TX_STATE_ACTIVE;
                return;
            }

            // Ignore stale/non-advancing ACKs.
            if (ack_seq <= udt->m_tx_last_acked)
            {
                udt->m_tx_state = TX_STATE_ACTIVE;
                return;
            }

            if (ack_seq > udt->m_tx_last_acked)
            {
                // Remove a range from the in-flight map corresponding to the cumulative ACK
                const u32 in_flight = udt->m_seq_ops->size(udt->m_seq_maps->m_tx_in_flight_map);
                udt->m_seq_ops->remove_range(udt->m_seq_maps->m_tx_in_flight_map, udt->m_tx_last_acked, ack_seq);

                // Also remove the same range from the missing map, in case any of those sequences were marked
                // missing due to out-of-order ACKs/NAKs
                udt->m_seq_ops->remove_range(udt->m_seq_maps->m_tx_missing_map, udt->m_tx_last_acked, ack_seq);

                udt->m_tx_last_acked  = ack_seq;
                udt->m_tx_last_ack_ts = now_ts;
            }

            udt->m_tx_flow_window = flow_window;
            udt->m_tx_state       = TX_STATE_ACTIVE;

            udt->m_cc_ops->on_ack(udt->m_cc_ctx, ack_seq, now_ts);
        }

        // -----------------------------------------------------------------------------
        // on_tx_nak_received
        // -----------------------------------------------------------------------------
        void on_tx_nak_received(udt_t* udt, const void* nak_map, u32 flow_window, u64 now_ts)
        {
            // Merge missing sequences into sender loss map
            udt->m_seq_ops->merge(udt->m_seq_maps->m_tx_missing_map, nak_map);

            const u32 loss_count_hint = udt->m_seq_ops->size(nak_map);

            // Signal congestion once per NAK event instead of once per retransmitted packet.
            // The sequence value is advisory for CC epoch bookkeeping.
            udt->m_cc_ops->on_loss(udt->m_cc_ctx, udt->m_tx_last_acked + 1u, loss_count_hint, now_ts);

            udt->m_tx_flow_window    = flow_window;
            udt->m_tx_state          = TX_STATE_ACTIVE;
            udt->m_tx_exp_timeout_ts = now_ts + udt->m_tx_exp_interval;
        }

        // -----------------------------------------------------------------------------
        // rx_tick: ACK / NAK / EXP timers
        // -----------------------------------------------------------------------------
        void rx_tick(udt_t* udt, u64 now_ts)
        {
            // ACK timer: send cumulative ACK periodically
            if ((now_ts - udt->m_rx_last_ack_sent_ts) >= udt->m_rx_ack_interval)
            {
                if (udt->m_rx_highest_contig < 0xFFFFFFFF)
                {
                    const u32 ack_seq = udt->m_rx_highest_contig + 1u;
                    packet_t ack_pkt;
                    if (udt->m_pkt_ops->m_build_ack(udt->m_user_ctx, ack_seq, udt->m_rx_flow_window, &ack_pkt))
                    {
                        udt->m_pkt_ops->m_send_packet(udt->m_user_ctx, &ack_pkt);
                        udt->m_rx_last_ack_sent_ts = now_ts;
                    }
                }
            }

            // NAK timer: send selective loss information if any gaps exist
            if (udt->m_seq_ops->size(udt->m_seq_maps->m_rx_missing_map) > 0 && (now_ts - udt->m_rx_last_nak_sent_ts) >= udt->m_rx_nak_interval)
            {
                packet_t nak_pkt;
                if (udt->m_pkt_ops->m_build_nak(udt->m_user_ctx, udt->m_seq_maps->m_rx_missing_map, udt->m_rx_flow_window, &nak_pkt))
                {
                    udt->m_pkt_ops->m_send_packet(udt->m_user_ctx, &nak_pkt);
                    udt->m_rx_last_nak_sent_ts = now_ts;
                }
            }

            // EXP timer: expire RX missing state to avoid unbounded growth
            if ((now_ts - udt->m_rx_last_progress_ts) >= udt->m_rx_exp_interval)
            {
                // Conservative expiration: clear RX missing map
                // Owning layer may choose a more precise policy
                // udt->m_seq_ops->remove_all(udt->m_seq_maps->m_rx_missing_map);
                udt->m_rx_last_progress_ts = now_ts;
            }
        }

        // -----------------------------------------------------------------------------
        // rx_next_tick_ts: returns the next timestamp at which rx_tick should be called
        // again based on ACK/NAK/EXP timers.
        // on_rx_data_received may cause timer changes so that require rx_tick needs to
        // be called again as well as rx_next_tick_ts to reschedule the next tick.
        // -----------------------------------------------------------------------------
        u64 rx_next_tick_ts(udt_t* udt, u64 now_ts)
        {
            const u64 next_ack_ts = udt->m_rx_last_ack_sent_ts + udt->m_rx_ack_interval;
            const u64 next_nak_ts = udt->m_rx_last_nak_sent_ts + udt->m_rx_nak_interval;

            u64 next_tick_ts = next_ack_ts;
            if (next_nak_ts < next_tick_ts)
                next_tick_ts = next_nak_ts;

            const u64 next_exp_ts = udt->m_rx_last_progress_ts + udt->m_rx_exp_interval;
            if (next_exp_ts < next_tick_ts)
                next_tick_ts = next_exp_ts;

            return next_tick_ts;
        }

        // -----------------------------------------------------------------------------
        // on_rx_data_received
        // -----------------------------------------------------------------------------
        void on_rx_data_received(udt_t* udt, u32 seq, u64 now_ts)
        {
            sequence_ops_t* ops = udt->m_seq_ops;

            udt->m_cc_ops->on_packet_received(udt->m_cc_ctx, seq, now_ts);

            if (!ops->push(udt->m_seq_maps->m_rx_received_map, seq))
            {
                // Sequence was already present in received map
                return;
            }

            if (seq > (udt->m_rx_highest_contig + 1))
            {
                // A gap is detected between the received sequence and the highest contiguous sequence.
                // Mark missing packets in sequence map, these stay here until we receive those missing
                // packets or they get expired by the EXP timer.
                for (u32 s = udt->m_rx_highest_contig + 1; s < seq; ++s)
                    ops->push(udt->m_seq_maps->m_rx_missing_map, s);

                // UDT reports loss immediately on gap detection; periodic NAK is only for repeats.
                packet_t nak_pkt;
                if (udt->m_pkt_ops->m_build_nak(udt->m_user_ctx, udt->m_seq_maps->m_rx_missing_map, udt->m_rx_flow_window, &nak_pkt))
                {
                    udt->m_pkt_ops->m_send_packet(udt->m_user_ctx, &nak_pkt);
                    udt->m_rx_last_nak_sent_ts = now_ts;
                }
            }

            ops->remove(udt->m_seq_maps->m_rx_missing_map, seq);

            while (ops->has(udt->m_seq_maps->m_rx_received_map, udt->m_rx_highest_contig + 1))
            {
                udt->m_rx_highest_contig += 1;
            }

            udt->m_rx_last_progress_ts = now_ts;
        }

    }  // namespace nudt
}  // namespace ncore
