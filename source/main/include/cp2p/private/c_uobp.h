#ifndef UOBP_H
#define UOBP_H

namespace ncore
{
    namespace nuobp
    {
        // ============================================================
        // Message IDs
        // ============================================================

        enum
        {
            MSG_OBJECT_INFO = 1,
            MSG_OBJECT_DATA = 2,
            MSG_OBJECT_ACK  = 3
        };

        // ============================================================
        // Wire structures (little-endian on the wire)
        // ============================================================

        struct object_info_t
        {
            u16 m_msg_id;
            u16 m_object_index;
            u16 m_object_gen;
            u16 m_block_size;
            u32 m_object_size;
        };

        struct object_data_t
        {
            u16 m_msg_id;
            u16 m_object_index;
            u16 m_object_gen;
            u16 m_block_idx;
            u16 m_block_len;
            u16 m_reserved0;
            u32 m_hash32;
        };

        struct object_ack_t
        {
            u16 m_msg_id;
            u16 m_object_index;
            u16 m_object_gen;
            u8  m_symbol_rb[2];
            u16 m_block_start;
            u16 m_ack_len;
            // Followed by SRLEN-compressed bitmap payload
        };

        // ============================================================
        // Bitmap helpers (bit = block-index)
        // ============================================================

        inline u16  bitmap_bytes(u16 num_bits) { return (num_bits + 7) >> 3; }
        inline void bitmap_clear(u8* bitmap, u16 bytes)
        {
            for (u16 i = 0; i < bytes; i++)
                bitmap[i] = 0;
        }

        inline bool bitmap_test(const u8* bitmap, u16 bit) { return (bitmap[bit >> 3] >> (bit & 7)) & 1; }
        inline void bitmap_set(u8* bitmap, u16 bit) { bitmap[bit >> 3] |= u8(1u << (bit & 7)); }

        // ============================================================
        // ACK bitmap decoding (SRLEN -> bitmap)
        // ============================================================
        //
        // Decodes an ACK bitmap window into the receiver bitmap.
        // Returns the number of bits that transitioned from 0 -> 1.
        //

        u16 decode_ack_bitmap(u8* m_dst_bitmap, u16 m_total_blocks, u16 m_block_start, const u8* m_ack_data, u16 m_ack_len);

        // ============================================================
        // Receiver public API (definitions in rx.cpp)
        // ============================================================

        // ============================================================
        // Function pointer typedefs
        // ============================================================

        typedef u32 (*hash32_fn_t)(void* m_user_ctx, const u8* m_data, u32 m_len);
        typedef void* (*alloc_object_data_fn_t)(void* m_user_ctx, u16 m_object_index, u16 m_object_gen, u32 m_object_size, u16 m_block_size);
        typedef void* (*alloc_object_bitmap_fn_t)(void* m_user_ctx, u16 m_object_index, u16 m_object_gen, u16 m_num_blocks, u16* m_out_bitmap_bytes);
        typedef void (*object_complete_fn_t)(void* m_user_ctx, u16 m_object_index, u16 m_object_gen, void* m_object_data, u32 m_object_size);
        typedef void (*object_abort_fn_t)(void* m_user_ctx, u16 m_object_index, u16 m_object_gen, void* m_object_data, void* m_bitmap_data);

        // ============================================================
        // RX ops
        // ============================================================

        struct rx_ops_t
        {
            alloc_object_data_fn_t   m_alloc_object_data;
            alloc_object_bitmap_fn_t m_alloc_object_bitmap;
            hash32_fn_t              m_hash32;
            object_complete_fn_t     m_object_complete;
            object_abort_fn_t        m_object_abort;
        };

        // ============================================================
        // Configuration
        // ============================================================

        static constexpr u32 RX_INDEX_SPACE = 256;

        // ============================================================
        // Per-object RX context
        // ============================================================

        struct rx_ctx_t
        {
            u16 m_active;
            u16 m_object_gen;
            u32 m_object_size;
            u8* m_data_ptr;
            u8* m_bitmap_ptr;
            u16 m_bitmap_bytes;
            u16 m_blocks_received;
            u16 m_block_size;
            u16 m_num_blocks;
        };

        struct rx_slot_t
        {
            rx_ctx_t m_ctx;
        };

        // ============================================================
        // RX instance
        // ============================================================

        struct rx_t
        {
            const rx_ops_t* m_ops;
            void*           m_user_ctx;
            rx_slot_t       m_slots[RX_INDEX_SPACE];
        };

        void rx_init(rx_t& m_rx, const rx_ops_t& m_ops, void* m_user_ctx);
        void rx_on_object_info(rx_t& m_rx, const object_info_t& m_info);
        void rx_on_object_data(rx_t& m_rx, const object_data_t& m_msg, const u8* m_payload);

        // ============================================================
        // Sender public API (definitions in tx.cpp)
        // ============================================================

        // ============================================================
        // Function pointer typedefs
        // ============================================================

        using hash32_fn_t      = u32 (*)(void* m_user_ctx, const u8* m_data, u32 m_len);
        using send_packet_fn_t = void (*)(void* m_user_ctx, const void* m_header, u32 m_header_len, const void* m_payload, u32 m_payload_len);

        // ============================================================
        // TX ops
        // ============================================================

        struct tx_ops_t
        {
            hash32_fn_t      m_hash32;
            send_packet_fn_t m_send_packet;
        };

        // ============================================================
        // TX instance
        // ============================================================

        struct tx_t
        {
            u16             m_object_index;
            u16             m_object_gen;
            u32             m_object_size;
            u16             m_block_size;
            u16             m_num_blocks;
            u16             m_inflight_limit;
            u16             m_in_flight;
            u16             m_min_inflight;
            u16             m_max_inflight;
            u16             m_ack_timeout_ms;
            u16             m_reserved1;
            const u8*       m_data_ptr;
            u8*             m_ack_bitmap;
            u32             m_last_progress_ms;
            u32             m_reserved0;
            const tx_ops_t* m_ops;
            void*           m_user_ctx;
        };

        void tx_init(tx_t& m_tx, const tx_ops_t& m_ops, void* m_user_ctx);
        void tx_configure(tx_t& m_tx, u16 m_min_inflight, u16 m_max_inflight, u16 m_ack_timeout_ms);
        void tx_start(tx_t& m_tx, u16 m_object_index, u16 m_object_gen, const u8* m_object_data, u32 m_object_size, u16 m_block_size, u8* m_ack_bitmap);
        void tx_on_ack(tx_t& m_tx, const object_ack_t& m_ack, const u8* m_ack_data, u16 m_ack_len, u32 m_now_ms);
        void tx_check_timeouts(tx_t& m_tx, u32 m_now_ms);
        void tx_pump(tx_t& m_tx);

    }  // namespace nuobp
}  // namespace ncore

#endif
