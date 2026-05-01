#include "ccore/c_target.h"
#include "cbase/c_allocator.h"
#include "cbase/c_memory.h"
#include "cbase/c_bit_field.h"

#include "cp2p/private/c_netio_proto.h"
#include "cp2p/private/c_netio.h"

#include "cp2p/private/c_uobp.h"

namespace ncore
{
    namespace nuobp
    {
        // ============================================================
        // Init
        // ============================================================

        void tx_init(tx_t& t, const tx_ops_t& ops, void* user_ctx)
        {
            g_memset(&t, 0, sizeof(t));
            t.m_ops      = &ops;
            t.m_user_ctx = user_ctx;

            // Sensible defaults
            t.m_min_inflight   = 1;
            t.m_max_inflight   = 16;
            t.m_ack_timeout_ms = 50;

            t.m_inflight_limit = t.m_min_inflight;
        }

        // ============================================================
        // Configure (can be called at any time)
        // ============================================================

        void tx_configure(tx_t& m_tx, u16 m_min_inflight, u16 m_max_inflight, u16 m_ack_timeout_ms)
        {
            m_tx.m_min_inflight   = m_min_inflight;
            m_tx.m_max_inflight   = m_max_inflight;
            m_tx.m_ack_timeout_ms = m_ack_timeout_ms;

            if (m_tx.m_inflight_limit < m_min_inflight)
                m_tx.m_inflight_limit = m_min_inflight;
            else if (m_tx.m_inflight_limit > m_max_inflight)
                m_tx.m_inflight_limit = m_max_inflight;
        }

        // ============================================================
        // Start object
        // ============================================================

        void tx_start(tx_t& t, u16 index, u16 gen, const u8* data, u32 size, u16 block_size, u8* ack_bitmap)
        {
            t.m_object_index = index;
            t.m_object_gen   = gen;
            t.m_object_size  = size;
            t.m_block_size   = block_size;
            t.m_num_blocks   = (size + block_size - 1) / block_size;

            t.m_data_ptr   = data;
            t.m_ack_bitmap = ack_bitmap;
            t.m_in_flight  = 0;

            bitmap_clear(ack_bitmap, bitmap_bytes(t.m_num_blocks));

            object_info_t info = {MSG_OBJECT_INFO, index, gen, size, block_size};

            t.m_ops->m_send_packet(t.m_user_ctx, &info, sizeof(info), nullptr, 0);
        }

        // ============================================================
        // ACK handling
        // ============================================================

        void tx_on_ack(tx_t& t, const object_ack_t& ack, const u8* ack_data, u16 ack_len, u32 now_ms)
        {
            u16 newly_acked = decode_ack_bitmap(t.m_ack_bitmap, t.m_num_blocks, ack.m_block_start, ack_data, ack_len);

            if (newly_acked > 0)
            {
                t.m_in_flight -= newly_acked;
                t.m_last_progress_ms = now_ms;

                if (t.m_inflight_limit < t.m_max_inflight)
                    t.m_inflight_limit++;
            }
        }

        // ============================================================
        // Timeout backoff
        // ============================================================

        void tx_check_timeouts(tx_t& t, u32 now_ms)
        {
            if (now_ms - t.m_last_progress_ms > t.m_ack_timeout_ms)
            {
                t.m_inflight_limit >>= 1;

                if (t.m_inflight_limit < t.m_min_inflight)
                    t.m_inflight_limit = t.m_min_inflight;

                t.m_last_progress_ms = now_ms;
            }
        }

        // ============================================================
        // Pump sender
        // ============================================================

        void tx_pump(tx_t& t)
        {
            for (u16 i = 0; i < t.m_num_blocks; i++)
            {
                if (bitmap_test(t.m_ack_bitmap, i))
                    continue;

                if (t.m_in_flight >= t.m_inflight_limit)
                    return;

                u32 offset = u32(i) * t.m_block_size;
                u16 len    = t.m_block_size;

                if (offset + len > t.m_object_size)
                    len = u16(t.m_object_size - offset);

                const u8* payload = t.m_data_ptr + offset;

                object_data_t msg = {MSG_OBJECT_DATA, t.m_object_index, t.m_object_gen, i, len, t.m_ops->m_hash32(t.m_user_ctx, payload, len)};

                t.m_ops->m_send_packet(t.m_user_ctx, &msg, sizeof(msg), payload, len);

                t.m_in_flight++;
            }
        }

    }  // namespace nuobp
}  // namespace ncore
