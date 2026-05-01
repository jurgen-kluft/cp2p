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

        void rx_init(rx_t& r, const rx_ops_t& ops, void* user_ctx)
        {
            g_memset(&r, 0, sizeof(r));
            r.m_ops      = &ops;
            r.m_user_ctx = user_ctx;
        }

        // ============================================================
        // Helpers
        // ============================================================

        static inline rx_slot_t& slot_for(rx_t& r, u16 object_index) { return r.m_slots[object_index & 0xFF]; }

        // ============================================================
        // OBJECT_INFO
        // ============================================================

        void rx_on_object_info(rx_t& r, const object_info_t& info)
        {
            rx_slot_t& slot = slot_for(r, info.m_object_index);
            rx_ctx_t&  ctx  = slot.m_ctx;

            if (ctx.m_active)
            {
                r.m_ops->m_object_abort(r.m_user_ctx, info.m_object_index, ctx.m_object_gen, ctx.m_data_ptr, ctx.m_bitmap_ptr);
            }

            g_memset(&ctx, 0, sizeof(ctx));

            u16 num_blocks = (info.m_object_size + info.m_block_size - 1) / info.m_block_size;

            void* data = r.m_ops->m_alloc_object_data(r.m_user_ctx, info.m_object_index, info.m_object_gen, info.m_object_size, info.m_block_size);
            if (!data)
                return;

            u16   bitmap_bytes = 0;
            void* bitmap       = r.m_ops->m_alloc_object_bitmap(r.m_user_ctx, info.m_object_index, info.m_object_gen, num_blocks, &bitmap_bytes);
            if (!bitmap)
            {
                r.m_ops->m_object_abort(r.m_user_ctx, info.m_object_index, info.m_object_gen, data, nullptr);
                return;
            }

            ctx.m_object_gen      = info.m_object_gen;
            ctx.m_object_size     = info.m_object_size;
            ctx.m_block_size      = info.m_block_size;
            ctx.m_num_blocks      = num_blocks;
            ctx.m_data_ptr        = static_cast<u8*>(data);
            ctx.m_bitmap_ptr      = static_cast<u8*>(bitmap);
            ctx.m_bitmap_bytes    = bitmap_bytes;
            ctx.m_blocks_received = 0;
            ctx.m_active          = true;

            bitmap_clear(ctx.m_bitmap_ptr, ctx.m_bitmap_bytes);
        }

        // ============================================================
        // OBJECT_DATA
        // ============================================================

        void rx_on_object_data(rx_t& r, const object_data_t& msg, const u8* payload)
        {
            rx_ctx_t& ctx = slot_for(r, msg.m_object_index).m_ctx;

            if (!ctx.m_active)
                return;

            if (ctx.m_object_gen != msg.m_object_gen)
                return;

            if (msg.m_block_idx >= ctx.m_num_blocks)
                return;

            u32 h = r.m_ops->m_hash32(r.m_user_ctx, payload, msg.m_block_len);

            if (h != msg.m_hash32)
                return;

            if (!bitmap_test(ctx.m_bitmap_ptr, msg.m_block_idx))
            {
                u8* dst = ctx.m_data_ptr + u32(msg.m_block_idx) * ctx.m_block_size;

                g_memcpy(dst, payload, msg.m_block_len);

                bitmap_set(ctx.m_bitmap_ptr, msg.m_block_idx);
                ctx.m_blocks_received++;
            }

            if (ctx.m_blocks_received == ctx.m_num_blocks)
            {
                r.m_ops->m_object_complete(r.m_user_ctx, msg.m_object_index, ctx.m_object_gen, ctx.m_data_ptr, ctx.m_object_size);
                g_memset(&ctx, 0, sizeof(ctx));
            }
        }

    }  // namespace nuobp
}  // namespace ncore
