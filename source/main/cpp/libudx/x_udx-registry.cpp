#include "xbase\x_target.h"
#include "xbase\x_memory_std.h"

#include "xp2p\x_sha1.h"

#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-alloc.h"
#include "xp2p\libudx\x_udx-address.h"
#include "xp2p\libudx\x_udx-socket.h"
#include "xp2p\libudx\x_udx-registry.h"

namespace xcore
{
	template<class KEY, class VALUE>
	class hashtable
	{
		struct hashitem
		{
			KEY			m_key;
			VALUE		m_value;
			hashitem*	m_next;
		};

		u32				m_size;
		u32				m_count;
		hashitem**		m_table;
		udx_alloc*		m_alloc;

		void			init(udx_alloc* a, u32 size)
		{
			m_alloc = a;
			m_size = size;
			m_count = 0;
			m_table = (hashitem**)m_alloc->alloc(size * sizeof(hashitem*));
			for (u32 i = 0; i < size; ++i)
				m_table[i].m_next = NULL;
		}

		void			add(KEY k, VALUE v)
		{

		}

		VALUE			find(KEY k) const
		{

		}
	};



	class udx_addresses_imp : public udx_addresses
	{
		SHA1_CTX		m_ctx;

	public:
		inline void				hash(udx_address& address)
		{
			sha1_init(&m_ctx);
			sha1_update(&m_ctx, (const u8*)address.m_data, sizeof(address.m_data));
			sha1_final(&m_ctx, (u8*)address.m_hash.m_hash);
		}

		virtual udx_address*	add(void* data, u32 size)
		{
			udx_address* pa = NULL;
			udx_socket* so = find_by_hash(a.m_hash);
			if (so == NULL)
			{
				pa = (udx_address*)m_allocator->alloc(sizeof(udx_address));
				pa->m_index = 0;
				x_memcpy(pa->m_hash, a.m_hash, sizeof(a.m_hash));
				x_memcpy(pa->m_data, a.m_data, sizeof(a.m_data));
			}
			else
			{
				pa = so->get_address();
			}
			return pa;
		}

	protected:

		static inline u32		hash_to_bucket_index(u32 hash)
		{	// Take 10 bits to be our index in the bucket array
			return (hash & 0x3FF0) >> 4;
		}

		udx_address*		find_by_hash(u32 const hash[5]) const
		{
			u32 bidx = hash_to_bucket_index(hash[0]);
			udx_address* s = m_buckets[bidx].find_by_hash(hash);
			return s;
		}

		udx_alloc*			m_allocator;

	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] udx registry of 'address' to 'socket'
	class udx_registry_imp : public udx_registry
	{
	public:
		virtual void			init(udx_alloc* allocator)
		{
			s32 num_buckets = 1024;
			m_buckets = (bucket*)allocator->alloc(num_buckets * sizeof(bucket));
			for (s32 i = 0; i < num_buckets; ++i)
			{
				m_buckets[i].init(allocator, 2);
			}
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

		udx_socket*				find_by_hash(u32 const hash[5]) const
		{
			u32 bidx = hash_to_bucket_index(hash[0]);
			udx_socket* s = m_buckets[bidx].find_by_hash(hash);
			return s;
		}

	protected:
		udx_alloc*			m_allocator;


		bucket*					m_buckets;
	};

}
