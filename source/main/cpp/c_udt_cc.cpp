#include "ccore/c_target.h"
#include "ccore/c_memory.h"

#include "cp2p/udt/c_udt.h"
#include "cp2p/udt/c_udt_cc.h"

namespace ncore
{
    namespace nudt
    {
        static inline u32 clamp_u32(u32 v, u32 lo, u32 hi) { return (v < lo) ? lo : ((v > hi) ? hi : v); }

        static inline u64 clamp_u64(u64 v, u64 lo, u64 hi) { return (v < lo) ? lo : ((v > hi) ? hi : v); }

        static inline u32 pseudo_random_u32(u32 seed)
        {
            // Deterministic mix so behavior is reproducible across runs.
            u32 x = seed ^ 0x9E3779B9u;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            return x;
        }

        static inline u64 ceil_mul_div_u64(u64 value, u64 mul_num, u64 div_den)
        {
            return (value * mul_num + (div_den - 1)) / div_den;
        }

        void udt_cc_t::init(udt_cc_t* cc, cc_ops_t* ops)
        {
            ASSERT(cc != nullptr);
            g_memset(cc, 0, sizeof(udt_cc_t));

            // UDT-style bootstrap: small initial period, moderate cwnd, and
            // periodic (10ms) rate-control updates.
            cc->m_min_cwnd                    = 2;
            cc->m_max_cwnd                    = 8192;
            cc->m_cwnd                        = 16;
            cc->m_in_flight                   = 0;
            cc->m_pacing_interval_us          = 1;
            cc->m_next_send_ts                = 0;
            cc->m_last_ack_seq                = 0;
            cc->m_has_last_ack                = 0;
            cc->m_slow_start                  = 1;
            cc->m_loss_since_last_rc          = 0;

            cc->m_rc_interval_us              = 10000;
            cc->m_last_rc_ts                  = 0;

            cc->m_last_dec_seq                = 0;
            cc->m_last_dec_pacing_interval_us = cc->m_pacing_interval_us;
            cc->m_nak_count                   = 0;
            cc->m_avg_nak_num                 = 0;
            cc->m_dec_random                  = 1;
            cc->m_dec_count                   = 0;

            cc->m_min_pacing_interval_us      = 1;
            cc->m_max_pacing_interval_us      = 100000;

            ASSERT(ops != nullptr);
            ops->on_packet_sent           = &udt_cc_t::on_packet_sent;
            ops->on_packet_received       = &udt_cc_t::on_packet_received;
            ops->on_ack                   = &udt_cc_t::on_ack;
            ops->on_loss                  = &udt_cc_t::on_loss;
            ops->on_timeout               = &udt_cc_t::on_timeout;
            ops->budget_before_congestion = &udt_cc_t::budget_before_congestion;
            ops->pacing_timeout_ts        = &udt_cc_t::pacing_timeout_ts;
        }

        void udt_cc_t::on_packet_sent(void* cc_ctx, u32 seq, u64 now_ts)
        {
            (void)seq;
            udt_cc_t* cc = static_cast<udt_cc_t*>(cc_ctx);
            ASSERT(cc != nullptr);

            if (cc->m_in_flight < 0xFFFFFFFFu)
                cc->m_in_flight += 1;

            cc->m_next_send_ts = now_ts + cc->m_pacing_interval_us;
        }

        void udt_cc_t::on_packet_received(void* cc_ctx, u32 seq, u64 now_ts)
        {
            (void)cc_ctx;
            (void)seq;
            (void)now_ts;
            // Placeholder for arrival-rate and bandwidth estimators.
        }

        void udt_cc_t::on_ack(void* cc_ctx, u32 ack_seq, u64 now_ts)
        {
            udt_cc_t* cc = static_cast<udt_cc_t*>(cc_ctx);
            ASSERT(cc != nullptr);

            u32 acked = 0;
            if (ack_seq > cc->m_last_ack_seq)
            {
                acked = ack_seq - cc->m_last_ack_seq;
            }

            cc->m_last_ack_seq = ack_seq;

            if (acked == 0)
            {
                if (cc->m_next_send_ts < now_ts)
                    cc->m_next_send_ts = now_ts;
                return;
            }

            cc->m_in_flight = (acked >= cc->m_in_flight) ? 0 : (cc->m_in_flight - acked);

            if (cc->m_slow_start)
            {
                // UDT slow start uses ACKed packets to open the window quickly.
                const u32 new_cwnd = cc->m_cwnd + acked * 2u;
                cc->m_cwnd         = clamp_u32(new_cwnd, cc->m_min_cwnd, cc->m_max_cwnd);

                if (cc->m_cwnd >= cc->m_max_cwnd)
                    cc->m_slow_start = 0;

                if (cc->m_pacing_interval_us > cc->m_min_pacing_interval_us)
                {
                    u64 step = (u64)acked * 2u;
                    if (step > 64u)
                        step = 64u;
                    cc->m_pacing_interval_us = (cc->m_pacing_interval_us > step)
                                                 ? (cc->m_pacing_interval_us - step)
                                                 : cc->m_min_pacing_interval_us;
                }

                if (cc->m_next_send_ts < now_ts)
                    cc->m_next_send_ts = now_ts;
                return;
            }

            // UDT uses a periodic rate-control interval for additive increase.
            if (cc->m_last_rc_ts != 0 && (now_ts - cc->m_last_rc_ts) < cc->m_rc_interval_us)
            {
                if (cc->m_next_send_ts < now_ts)
                    cc->m_next_send_ts = now_ts;
                return;
            }

            cc->m_last_rc_ts = now_ts;

            // Skip one increase round right after a loss event.
            if (cc->m_loss_since_last_rc)
            {
                cc->m_loss_since_last_rc = 0;
                if (cc->m_next_send_ts < now_ts)
                    cc->m_next_send_ts = now_ts;
                return;
            }

            // Additive increase: lower pacing interval slowly, and gently raise cwnd.
            const u64 step = (cc->m_pacing_interval_us >> 5) + 1;  // ~3.1%
            if (cc->m_pacing_interval_us > cc->m_min_pacing_interval_us)
                cc->m_pacing_interval_us = (cc->m_pacing_interval_us > step)
                                             ? (cc->m_pacing_interval_us - step)
                                             : cc->m_min_pacing_interval_us;

            const u32 cwnd_inc = (cc->m_cwnd < 1024u) ? 1u : (cc->m_cwnd >> 10);
            cc->m_cwnd         = clamp_u32(cc->m_cwnd + cwnd_inc, cc->m_min_cwnd, cc->m_max_cwnd);

            if (cc->m_next_send_ts < now_ts)
                cc->m_next_send_ts = now_ts;
        }

        void udt_cc_t::on_loss(void* cc_ctx, u32 seq, u32 loss_count, u64 now_ts)
        {
            udt_cc_t* cc = static_cast<udt_cc_t*>(cc_ctx);
            ASSERT(cc != nullptr);

            if (loss_count == 0)
                loss_count = 1;

            if (cc->m_slow_start)
                cc->m_slow_start = 0;

            cc->m_loss_since_last_rc = 1;

            // UDT-style loss epoch handling: one full decrease, then randomized
            // additional decreases with a cap per congestion epoch.
            if (seq > cc->m_last_dec_seq)
            {
                cc->m_last_dec_pacing_interval_us = cc->m_pacing_interval_us;
                cc->m_pacing_interval_us          = ceil_mul_div_u64(cc->m_pacing_interval_us, 9, 8);  // x1.125

                const u32 blended = (7u * cc->m_avg_nak_num + cc->m_nak_count + 7u) >> 3;             // ~0.875/0.125 EWMA
                cc->m_avg_nak_num  = blended;
                cc->m_nak_count    = loss_count;
                cc->m_dec_count    = 1;
                cc->m_last_dec_seq = seq;

                u32 rnd = pseudo_random_u32(seq ^ (loss_count * 0x45d9f3bu));
                u32 th  = (cc->m_avg_nak_num > 0) ? ((rnd % cc->m_avg_nak_num) + 1) : 1;
                cc->m_dec_random = (th == 0) ? 1 : th;
            }
            else
            {
                cc->m_nak_count += loss_count;
                if (cc->m_dec_count < 5u && (cc->m_nak_count % cc->m_dec_random) == 0u)
                {
                    cc->m_pacing_interval_us = ceil_mul_div_u64(cc->m_pacing_interval_us, 9, 8);  // x1.125
                    cc->m_last_dec_seq       = seq;
                    cc->m_dec_count += 1;
                }
            }

            cc->m_pacing_interval_us = clamp_u64(cc->m_pacing_interval_us, cc->m_min_pacing_interval_us, cc->m_max_pacing_interval_us);

            // Do not cut cwnd too aggressively on isolated NAK bursts.
            const u32 reduced = cc->m_cwnd - (cc->m_cwnd >> 3);  // -12.5%
            cc->m_cwnd        = clamp_u32(reduced, cc->m_min_cwnd, cc->m_max_cwnd);

            cc->m_next_send_ts = now_ts + cc->m_pacing_interval_us;
        }

        void udt_cc_t::on_timeout(void* cc_ctx, u64 now_ts)
        {
            udt_cc_t* cc = static_cast<udt_cc_t*>(cc_ctx);
            ASSERT(cc != nullptr);

            // Timeout terminates slow start and triggers a conservative restart.
            cc->m_slow_start = 0;

            const u32 half = (cc->m_cwnd > 1) ? (cc->m_cwnd >> 1) : 1;
            cc->m_cwnd      = clamp_u32(half, cc->m_min_cwnd, cc->m_max_cwnd);

            // Timeout is severe; restart with no assumed flight.
            cc->m_in_flight = 0;
            cc->m_loss_since_last_rc = 1;

            cc->m_pacing_interval_us = clamp_u64(cc->m_pacing_interval_us << 1, cc->m_min_pacing_interval_us, cc->m_max_pacing_interval_us);

            cc->m_next_send_ts = now_ts + cc->m_pacing_interval_us;
        }

        u32 udt_cc_t::budget_before_congestion(void* cc_ctx)
        {
            udt_cc_t* cc = static_cast<udt_cc_t*>(cc_ctx);
            ASSERT(cc != nullptr);

            if (cc->m_in_flight >= cc->m_cwnd)
                return 0;

            return cc->m_cwnd - cc->m_in_flight;
        }

        u64 udt_cc_t::pacing_timeout_ts(void* cc_ctx, u64 now_ts, u64 last_tx_ts)
        {
            udt_cc_t* cc = static_cast<udt_cc_t*>(cc_ctx);
            ASSERT(cc != nullptr);

            // last_tx_ts is provided by the transport and should always be respected.
            const u64 min_next_from_last_tx = last_tx_ts + cc->m_pacing_interval_us;

            u64 next_ts = cc->m_next_send_ts;
            if (next_ts < min_next_from_last_tx)
                next_ts = min_next_from_last_tx;

            // Initialize immediately on first use.
            if (next_ts == 0)
                next_ts = now_ts;

            return next_ts;
        }

    }  // namespace nudt
}  // namespace ncore
