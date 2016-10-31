#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"

#include "xp2p\libudx\x_udx.h"

namespace xcore
{

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] udx registry of 'address' to 'socket'
	class udx_registry_imp : public udx_registry
	{
	public:
		virtual void			init(udx_allocator* allocator)
		{
			s32 num_buckets = 1024;
			m_buckets = (bucket*)allocator->alloc(num_buckets * sizeof(bucket));
			for (s32 i = 0; i < num_buckets; ++i)
			{
				m_buckets[i].init(allocator, 2);
			}
		}

		virtual udx_address*	find(void const* data, u32 size) const
		{
			u32 hash[5];
			data_to_hash(data, size, hash);
			
			udx_socket* s = find_by_hash(hash);
			if (s == NULL)
				return NULL;
			return s->get_key();
		}

		virtual udx_address*	add(void const* data, u32 size) 
		{
			u32 hash[5];
			data_to_hash(data, size, hash);

			udx_address* a = (udx_address*)m_allocator->alloc(sizeof(udx_address));
			memcpy(a->m_data, data, size);
			memcpy(a->m_hash, hash, sizeof(hash));
			a->m_index = 0;
			return a;
		}

		virtual udx_socket*		find(udx_address* k)
		{
			udx_socket* s = find_by_key(k);
			return s;
		}

		virtual void			add(udx_address* k, udx_socket* v)
		{
			u32 hash = k->m_hash[0];
			u32 bidx = hash_to_bucket_index(hash);
			m_buckets[bidx].add(m_allocator, k, v);
		}

	protected:
		static inline void		data_to_hash(void const* data, u32 size, u32* out_hash)
		{
			SHA1_CTX ctx;
			sha1_init(&ctx);
			sha1_update(&ctx, (const u8*)data, size);
			sha1_final(&ctx, (u8*)out_hash);
		}

		static inline u32		hash_to_bucket_index(u32 hash)
		{	// Take 10 bits to be our index in the bucket array
			return (hash & 0x3FF0) >> 4;
		}
		
		udx_socket*				find_by_key(udx_address* a) const
		{
			u32 bidx = hash_to_bucket_index(a->m_hash[0]);
			udx_socket* s = m_buckets[bidx].find(a);
			return s;
		}

		udx_socket*				find_by_hash(u32 hash[5]) const
		{
			u32 bidx = hash_to_bucket_index(hash[0]);
			udx_socket* s = m_buckets[bidx].find_by_hash(hash);
			return s;
		}

	protected:
		udx_allocator*			m_allocator;

		struct bucket
		{
			u32				m_size;
			u32				m_max;
			udx_socket**	m_values;

			void			init(udx_allocator* a, u32 size)
			{
				m_size = 0;
				m_max = size;
				m_values = (udx_socket**)a->alloc(size * sizeof(void*));
				for (u32 i = 0; i < size; ++i)
					m_values[i] = NULL;
			}

			void			add(udx_allocator* a, udx_address* k, udx_socket* v)
			{
				for (u32 i = 0; i < m_size; i++)
				{
					if (m_values[i] == v)
						return;
				}
				if (m_size == m_max)
				{
					m_max = m_max * 2;
					udx_socket** values = (udx_socket**)a->alloc(m_max * sizeof(void*));
					memcpy(values, m_values, m_size * sizeof(void*));
					a->dealloc(m_values);
					m_values = values;
				}
				m_values[m_size++] = v;
			}

			udx_socket*		find(udx_address* k)
			{
				for (u32 i = 0; i < m_size; i++)
				{
					if (m_values[i]->get_key() == k)
						return m_values[i];
				}
				return NULL;
			}

			udx_socket*		find_by_hash(u32 hash[5])
			{
				for (u32 i = 0; i < m_size; i++)
				{
					udx_address* a = m_values[i]->get_key();
					if (memcmp(a->m_hash, hash, sizeof(hash)) == 0)
						return m_values[i];
				}
				return NULL;
			}
		};
		bucket*					m_buckets;
	};

}
