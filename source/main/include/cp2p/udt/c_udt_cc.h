#ifndef __CP2P_UDT_CC_H__
#define __CP2P_UDT_CC_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

// ------------------------------------------------------------
// UDT Congestion Control
// ------------------------------------------------------------

namespace ncore
{
    namespace nudt
    {
        struct cc_ops_t;

        // -----------------------------------------------------------------------------
        // UDT Congestion Control
        // -----------------------------------------------------------------------------
        struct udt_cc_t
        {
            static void init(udt_cc_t* cc, cc_ops_t* ops);

            static void on_packet_sent(void* cc_ctx, u32 seq, u64 now_ts);
            static void on_packet_received(void* cc_ctx, u32 seq, u64 now_ts);
            static void on_ack(void* cc_ctx, u32 ack_seq, u64 now_ts);
            static void on_loss(void* cc_ctx, u32 seq, u32 loss_count, u64 now_ts);
            static void on_timeout(void* cc_ctx, u64 now_ts);

            // Returns whether we are currently allowed to send a packet before hitting
            // congestion control limits.
            static u32 budget_before_congestion(void* cc_ctx);

            // If pacing is supported, returns the time until which we should wait before
            // sending the next packet. If the current time (now_ts) is greater than or
            // equal to the returned timestamp, we are allowed to send a packet.
            static u64 pacing_timeout_ts(void* cc_ctx, u64 now_ts, u64 last_tx_ts);

            u32 m_cwnd;
            u32 m_min_cwnd;
            u32 m_max_cwnd;
            u32 m_in_flight;

            u64 m_pacing_interval_us;
            u64 m_next_send_ts;

            u32 m_last_ack_seq;
            u8  m_has_last_ack;
            u8  m_slow_start;
            u8  m_loss_since_last_rc;
            u8  m_padding0;

            u64 m_rc_interval_us;
            u64 m_last_rc_ts;

            u32 m_last_dec_seq;
            u64 m_last_dec_pacing_interval_us;
            u32 m_nak_count;
            u32 m_avg_nak_num;
            u32 m_dec_random;
            u32 m_dec_count;

            u64 m_min_pacing_interval_us;
            u64 m_max_pacing_interval_us;
        };

    }  // namespace nudt
}  // namespace ncore

#endif  // __CP2P_UDT_H__
