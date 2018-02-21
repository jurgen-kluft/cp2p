#include "xbase/x_target.h"
#include "xbase/x_string_ascii.h"
#include "xbase/x_memory_std.h"

#include "xp2p/x_sha1.h"

#include "xp2p/libudx/x_udx.h"
#include "xp2p/libudx/x_udx-address.h"
#include "xp2p/libudx/x_udx-registry.h"
#include "xp2p/libudx/x_udx-peer.h"
#include "xp2p/libudx/x_udx-alloc.h"

#include "xp2p/private/x_sockets.h"

#ifdef PLATFORM_PC
#include <winsock2.h>         // For socket(), connect(), send(), and recv()
#include <ws2tcpip.h>
typedef int socklen_t;
typedef char raw_type;       // Type used for raw data on this platform
#else
#include <sys/types.h>       // For data types
#include <sys/socket.h>      // For socket(), connect(), send(), and recv()
#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()
#include <netinet/in.h>      // For sockaddr_in
#include <cstring>	       // For memset()
#include <cstdlib>	       // For atoi()
typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <errno.h>             // For errno


namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PRIVATE] API
	class udx_haddress : public udx_address
	{
	public:
		virtual bool 				from_string(const char*);
		virtual s32					to_string(char* str, u32 maxstrlen) const;
		virtual udx_addrin const&	get_addrin() const;

		udx_haddress*				m_next;
		u32							m_index;
		udx_addrin					m_addrin;
		udx_hash					m_hash;
	};
	
	bool	udx_haddress::from_string(const char* str)
	{
		s32 result = m_addrin.from_string(str);
		return result == 0;
	}

	s32		udx_haddress::to_string(char* str, u32 maxstrlen) const
	{
		// @TODO
		return 0;
	}

	udx_addrin const&	udx_haddress::get_addrin() const
	{
		return m_addrin;
	}


	class udx_address_factory_imp : public udx_address_factory
	{
		SHA1_CTX		m_ctx;

	public:
		void					init()
		{
			m_count = 0;
			m_size = 4096;
			m_mask = (m_size - 1);
		}

		virtual udx_hash		compute_hash(void* addrin, u32 addrinlen)
		{
			udx_hash hash;
			sha1_init(&m_ctx);
			sha1_update(&m_ctx, (const u8*)addrin, addrinlen);
			sha1_final(&m_ctx, (u8*)hash.m_hash);
		}

		virtual udx_address*	create(void* addrin, u32 addrinlen)
		{
			udx_hash hash = compute_hash(addrin, addrinlen);
			udx_haddress* h = find_by_hash(hash);
			if (h == nullptr)
			{
				h = (udx_haddress*)m_allocator->alloc(sizeof(udx_haddress));
				h->m_hash = hash;
				h->m_addrin.m_len = addrinlen;
				x_memcpy(h->m_addrin.m_data, addrin, addrinlen);
				add(h);
			}
			return h;
		}

		virtual void			destroy(udx_address* addr)
		{
			m_allocator->dealloc(addr);
		}
		
		virtual bool			get_assoc(void* addrin, u32 addrinlen, udx_address*& assoc)
		{
			udx_hash hash = compute_hash(addrin, addrinlen);
			assoc = find_by_hash(hash);
			return assoc!=nullptr;
		}

		virtual	void			set_assoc(void* addrin, u32 addrinlen, udx_address* addr)
		{
			udx_hash hash = compute_hash(addrin, addrinlen);
			udx_haddress* h = find_by_hash(hash);
			if (h == nullptr)
			{
				add(h);
			}
		}

		virtual	void			del_assoc(void* addrin, u32 addrinlen)
		{
			udx_hash hash = compute_hash(addrin, addrinlen);
			udx_haddress* h = find_by_hash(hash);
			if (h != nullptr)
			{
				remove(h);
				destroy(h);
			}
		}

		virtual bool			get_assoc(udx_address* adr, u32& assoc)
		{
			udx_haddress* hadr = (udx_haddress*)adr;
			assoc = hadr->m_index;
			return true;
		}

		virtual	void			set_assoc(udx_address* addr, u32 idx)
		{
			udx_haddress* haddr = (udx_haddress*)addr;
			haddr->m_index = idx;
		}

		virtual void			del_assoc(udx_address* adr)
		{

		}

	protected:
		u32					hash_to_index(udx_hash const & hash)
		{
			u32 i = (u32)(hash.m_hash[8]) | (u32)(hash.m_hash[9] << 8) | (u32)(hash.m_hash[10] << 16) | (u32)(hash.m_hash[11] << 24);
			i = i & m_mask;
			return i;
		}

		udx_haddress*		find_by_hash(udx_hash const & hash)
		{
			u32 const i = hash_to_index(hash);
			udx_haddress* e = m_hashtable[i];
			while (e != nullptr)
			{
				if (e->m_hash.m_len == hash.m_len && memcmp(e->m_hash.m_hash, hash.m_hash, sizeof(hash)) == 0)
					break;
				e = e->m_next;
			}
			return e;
		}

		void				add(udx_haddress* h)
		{
			u32 const i = hash_to_index(h->m_hash);
			udx_haddress** p = &m_hashtable[i];
			udx_haddress* e = m_hashtable[i];
			while (e != nullptr)
			{
				s32 r = memcmp(e->m_hash.m_hash, h->m_hash.m_hash, sizeof(udx_haddress::m_hash));
				if (r == -1)
				{
					break;
				}
				p = &e->m_next;
				e = e->m_next;
			}
			*p = h;
			h->m_next = e;
		}

		void				remove(udx_haddress* h)
		{
			u32 const i = hash_to_index(h->m_hash);
			udx_haddress** p = &m_hashtable[i];
			udx_haddress* e = m_hashtable[i];
			while (e != nullptr)
			{
				s32 r = memcmp(e->m_hash.m_hash, h->m_hash.m_hash, sizeof(udx_haddress::m_hash));
				if (r == 0)
				{
					*p = e->m_next;
					e = e->m_next;
					break;
				}
				p = &e->m_next;
				e = e->m_next;
			}
			h->m_next = nullptr;
		}

		udx_alloc*			m_allocator;

		u32					m_count;
		u32					m_size;
		u32					m_mask;
		udx_haddress**		m_hashtable;
	};

#ifdef PLATFORM_PC

	s32		udx_addrin::from_string(const char* addr)
	{
		struct addrinfo hints;
		struct addrinfo *result = NULL;
		struct addrinfo *ptr = NULL;

		//--------------------------------
		// Setup the hints address info structure which is passed to the getaddrinfo() function
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_flags = AI_NUMERICHOST;
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;

		//--------------------------------
		m_len = 0;
		ZeroMemory(m_data, sizeof(m_data));

		//--------------------------------
		// Call getaddrinfo(). If the call succeeds, the result variable will hold a linked list
		// of addrinfo structures containing response information
		DWORD dwRetval = getaddrinfo(addr, NULL, &hints, &result);
		if (dwRetval != 0)
		{
			//printf("getaddrinfo failed with error: %d\n", dwRetval);
			return -1;
		}

		// Retrieve each address and print out the hex bytes
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
		{
			if (ptr->ai_socktype == SOCK_DGRAM && ptr->ai_protocol == IPPROTO_UDP)
			{
				m_len = ptr->ai_addrlen;
				u8 const* ai_addr = (u8 const*)ptr->ai_addr;
				for (s32 i = 0; i < m_len; i++)
				{
					m_data[i] = ai_addr[i];
				}
				break;
			}
		}

		freeaddrinfo(result);
		return 0;
	}

	s32		udx_addrin::to_string(char* str, u32 maxstrlen) const
	{
		// @TODO: NOT IMPLEMENTED !
		return -1;
	}

#else
	s32		udx_addrin::from_string(const char* addr)
	{
		return -1;
	}

	s32		udx_addrin::to_string(char* str, u32 maxstrlen) const
	{
		return -1;
	}

#endif

}
