#include "ccore/c_target.h"
#include "ccore/c_memory.h"

#include "cp2p/udt/c_udt_sequence_map.h"

namespace ncore
{
    namespace nudt
    {
        void g_sequence_map_init(sequence_map_t* map, u32* words, u32 word_count, u32 max_seq, sequence_map_mode_t mode)
        {
            ASSERT(map != nullptr && words != nullptr && max_seq <= word_count * 32);

            map->m_bitmap               = words;
            map->m_bitmap_size_in_words = word_count;
            map->m_max_seq              = max_seq;
            map->m_mode                 = mode;
            map->m_rr_cursor            = 0;
            map->m_set_count            = 0;
            map->m_seq_bounds[0]        = max_seq;
            map->m_seq_bounds[1]        = 0;

            for (u32 i = 0; i < word_count; ++i)
                map->m_bitmap[i] = 0;

            if (mode == SEQMAP_MODE_RANDOM)
            {
                u64 seed = (u64)map;
                xor_random_reset(map->m_xor_random_s[0], map->m_xor_random_s[1], seed);
            }
        }

        void g_seq_serialize(const void* map, u8* out_buf, u32* out_buf_len)
        {
            // what is the minimum we need to serialize/deserialize a sequence map?
            // At minimum, we need to serialize the bitmap words that have set bits,
            // which can be optimized by only serializing the range of words that
            // contain set bits (using m_seq_bounds).
            // Mode is not necessary to serialize, since the user knows the mode,
            // this includes all the mode variables.
            // We can use a simple format:
            // [3 bytes] start_seq (m_seq_bounds[0])
            // [3 bytes] end_seq (m_seq_bounds[1])
            // [N bytes] bitmap data for bytes in the range [start_seq >> 5, (end_seq + 31) >> 5]
        }

        void g_seq_deserialize(void* map, const u8* buf, u32 buf_len)
        {
            // ...
        }
    }  // namespace nudt
}  // namespace ncore
