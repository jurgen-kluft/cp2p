#ifndef __CP2P_UDT_SEQUENCE_MAP_BITMAP_H__
#define __CP2P_UDT_SEQUENCE_MAP_BITMAP_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace nudt
    {
        // -----------------------------------------------------------------------------
        // Sequence map modes
        // -----------------------------------------------------------------------------
        enum sequence_map_mode_t
        {
            SEQMAP_MODE_LOWEST = 0,   // Pop the lowest set sequence number
            SEQMAP_MODE_ROUND_ROBIN,  // Pop sequence numbers in a round-robin fashion
            SEQMAP_MODE_RANDOM        // Pop sequence numbers in a random order
        };

        // -----------------------------------------------------------------------------
        // XORShift random number generator (used for SEQMAP_MODE_RANDOM)
        // -----------------------------------------------------------------------------

        static inline u64 xor_random_next(u64& s0, u64& s1)
        {
            u64 ss1    = s0;
            u64 ss0    = s1;
            u64 result = ss0 + ss1;
            s0         = ss0;
            ss1 ^= ss1 << 23;
            s1 = ss1 ^ ss0 ^ (ss1 >> 18) ^ (ss0 >> 5);
            return result;
        }

        static inline void xor_random_reset(u64& s0, u64& s1, s64 seed)
        {
            s0 = seed + 6364136223846793;
            s1 = 6364136223846793005;
            xor_random_next(s0, s1);
            xor_random_next(s0, s1);
        }

        // -----------------------------------------------------------------------------
        // Sequence map operations interface
        // -----------------------------------------------------------------------------
        struct sequence_ops_t
        {
            void (*push)(void* map, u32 seq);
            void (*remove)(void* map, u32 seq);
            void (*remove_range)(void* map, u32 start_seq, u32 end_seq);
            void (*remove_all)(void* map);
            bool (*has)(const void* map, u32 seq);
            i32 (*pop)(void* map);
            void (*merge)(void* dst, const void* src);
            u32 (*size)(const void* map);
            void (*serialize)(const void* map, u8* out_buf, u32* out_buf_len);
            void (*deserialize)(void* map, const u8* buf, u32 buf_len);
        };

        // -----------------------------------------------------------------------------
        // Bitmap-backed sequence map with round-robin support
        // -----------------------------------------------------------------------------
        struct sequence_map_t
        {
            u32*                m_bitmap;                // Bitmap (each bit represents a sequence number)
            u32                 m_bitmap_size_in_words;  // Number of words allocated for the bitmap (must be >= (m_max_seq + 31) / 32)
            u32                 m_max_seq;               // Maximum sequence number that can be tracked (must be <= m_bitmap_size_in_words * 32)
            u32                 m_set_count;             // Number of currently set bits in the bitmap (for quick emptiness checks)
            u32                 m_seq_bounds[2];         // Current range (min/max) we know we have set bits in, used for optimization
            u32                 m_rr_cursor;             // Cursor for round-robin popping (valid only if m_mode == SEQMAP_MODE_ROUND_ROBIN)
            u64                 m_xor_random_s[2];       // State for XORShift random number generator (used if m_mode == SEQMAP_MODE_RANDOM)
            sequence_map_mode_t m_mode;                  // SEQMAP_MODE_ROUND_ROBIN or SEQMAP_MODE_RANDOM
        };

        // -----------------------------------------------------------------------------
        // Bitmap helpers
        // -----------------------------------------------------------------------------
        static inline bool s_bitmap_test_bit(const u32* words, u32 bit) { return (words[bit >> 5] >> (bit & 31)) & 1u; }
        static inline void s_bitmap_set_bit(u32* words, u32 bit) { words[bit >> 5] |= (1u << (bit & 31)); }
        static inline void s_bitmap_clr_bit(u32* words, u32 bit) { words[bit >> 5] &= ~(1u << (bit & 31)); }

        // -----------------------------------------------------------------------------
        // Initialization
        // -----------------------------------------------------------------------------
        void g_sequence_map_init(sequence_map_t* map, u32* words, u32 word_count, u32 max_seq, sequence_map_mode_t mode);

        // -----------------------------------------------------------------------------
        // sequence_ops_t implementation
        // -----------------------------------------------------------------------------
        static inline void s_seq_push(void* map, u32 seq)
        {
            sequence_map_t* m = (sequence_map_t*)map;
            if (seq >= m->m_max_seq)
                return;
            if (!s_bitmap_test_bit(m->m_bitmap, seq))
            {
                // update bounds
                if (seq < m->m_seq_bounds[0])
                    m->m_seq_bounds[0] = seq;
                if (seq > m->m_seq_bounds[1])
                    m->m_seq_bounds[1] = seq;

                s_bitmap_set_bit(m->m_bitmap, seq);
                ++m->m_set_count;
            }
        }

        static inline void s_seq_remove(void* map, u32 seq)
        {
            sequence_map_t* m = (sequence_map_t*)map;
            if (seq >= m->m_max_seq)
                return;
            if (s_bitmap_test_bit(m->m_bitmap, seq))
            {
                s_bitmap_clr_bit(m->m_bitmap, seq);
                --m->m_set_count;
            }
        }

        static inline void s_seq_remove_range(void* map, u32 start_seq, u32 end_seq)
        {
            sequence_map_t* m = (sequence_map_t*)map;
            if (start_seq >= m->m_max_seq || end_seq > m->m_max_seq || start_seq >= end_seq)
                return;

            for (u32 seq = start_seq; seq < end_seq; ++seq)
            {
                if (s_bitmap_test_bit(m->m_bitmap, seq))
                {
                    s_bitmap_clr_bit(m->m_bitmap, seq);
                    --m->m_set_count;
                }
            }
        }

        static inline void s_seq_remove_all(void* map)
        {
            sequence_map_t* m = (sequence_map_t*)map;
            for (u32 i = m->m_seq_bounds[0] >> 5; i <= (m->m_seq_bounds[1] >> 5) && i < m->m_bitmap_size_in_words; ++i)
                m->m_bitmap[i] = 0;
            m->m_seq_bounds[0] = m->m_max_seq;
            m->m_seq_bounds[1] = 0;
            m->m_rr_cursor     = 0;
            m->m_set_count     = 0;
        }

        static inline bool s_seq_has(const void* map, u32 seq)
        {
            const sequence_map_t* m = (const sequence_map_t*)map;
            if (seq >= m->m_max_seq)
                return false;
            return s_bitmap_test_bit(m->m_bitmap, seq);
        }

        static i32 s_seq_pop(void* map)
        {
            sequence_map_t* m = (sequence_map_t*)map;
            if (m->m_set_count == 0 || m->m_max_seq == 0)
                return -1;

            if (m->m_mode == SEQMAP_MODE_ROUND_ROBIN)
            {
                u32 start = m->m_rr_cursor;
                u32 i     = start;

                // optimize: search word by word, not bit by bit
                do
                {
                    if (s_bitmap_test_bit(m->m_bitmap, i))
                    {
                        s_bitmap_clr_bit(m->m_bitmap, i);
                        --m->m_set_count;
                        m->m_rr_cursor = (i + 1) % m->m_max_seq;
                        return (i32)i;
                    }
                    i++;
                    if (i == m->m_max_seq)
                        i = 0;
                } while (i != start);
            }
            else if (m->m_mode == SEQMAP_MODE_RANDOM)
            {
                const u64 rnd          = xor_random_next(m->m_xor_random_s[0], m->m_xor_random_s[1]);
                const u32 seq_range    = m->m_seq_bounds[1] - m->m_seq_bounds[0] + 1;
                u32       random_index = (u32)(rnd % seq_range) + m->m_seq_bounds[0];
                // Start from random_index and search for a set bit, wrap around if needed
                // optimize: search word by word, not bit by bit
                for (u32 offset = 0; offset < seq_range; ++offset)
                {
                    u32 i = (random_index + offset) % m->m_max_seq;
                    if (s_bitmap_test_bit(m->m_bitmap, i))
                    {
                        s_bitmap_clr_bit(m->m_bitmap, i);
                        --m->m_set_count;
                        return (i32)i;
                    }
                }
            }

            // SEQMAP_MODE_LOWEST
            for (u32 i = 0; i < m->m_max_seq; ++i)
            {
                if (s_bitmap_test_bit(m->m_bitmap, i))
                {
                    s_bitmap_clr_bit(m->m_bitmap, i);
                    --m->m_set_count;
                    return (i32)i;
                }
            }

            return -1;
        }

        static void s_seq_merge(void* dst, const void* src)
        {
            sequence_map_t*       d = (sequence_map_t*)dst;
            const sequence_map_t* s = (const sequence_map_t*)src;

            u32 n = d->m_bitmap_size_in_words;
            if (s->m_bitmap_size_in_words < n)
                n = s->m_bitmap_size_in_words;

            // TODO, OPTIMIZE; we know the range of sequences in the src map,
            //                 so we can limit our merge to that range.

            for (u32 w = 0; w < n; ++w)
            {
                u32 before = d->m_bitmap[w];
                u32 merged = before | s->m_bitmap[w];
                if (merged != before)
                {
                    u32 diff = merged & ~before;
                    d->m_set_count += __builtin_popcount(diff);
                    d->m_bitmap[w] = merged;
                }
            }
        }

        static inline u32 s_seq_size(const void* map)
        {
            const sequence_map_t* m = (const sequence_map_t*)map;
            return m->m_set_count;
        }

        void g_seq_serialize(const void* map, u8* out_buf, u32* out_buf_len);
        void g_seq_deserialize(void* map, const u8* buf, u32 buf_len);

        // -----------------------------------------------------------------------------
        // Exported ops table
        // -----------------------------------------------------------------------------
        static const sequence_ops_t g_sequence_ops_rr = {s_seq_push, s_seq_remove, s_seq_remove_range, s_seq_remove_all, s_seq_has, s_seq_pop, s_seq_merge, s_seq_size, g_seq_serialize, g_seq_deserialize};

    }  // namespace nudt
}  // namespace ncore

#endif  // __CP2P_UDT_SEQUENCE_MAP_BITMAP_H__
