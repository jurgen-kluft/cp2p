#include "xbase\x_target.h"
#include "xbase\x_string_ascii.h"
#include "xbase\x_memory_std.h"

#include "xp2p\x_sha1.h"

#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-address.h"
#include "xp2p\libudx\x_udx-registry.h"
#include "xp2p\libudx\x_udx-socket.h"
#include "xp2p\libudx\x_udx-alloc.h"

#include "xp2p\private\x_sockets.h"

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
	class udx_haddress : public udx_address
	{
	public:
		enum
		{
			HASH_SIZE = 20,
		}

		udx_peer*		m_peer;
		udx_haddress*	m_next;
		u8				m_hash[HASH_SIZE];
	};

	udx_peer*	udx_address::get_peer()
	{
		udx_haddress* haddress = (udx_haddress*)this;
		return haddress->m_peer;
	}

	void		udx_address::set_peer(udx_peer* peer)
	{
		udx_haddress* haddress = (udx_haddress*)this;
		haddress->m_peer = peer;
	}

	class udx_addresses_imp : public udx_addresses
	{
		SHA1_CTX		m_ctx;

	public:
		void					init()
		{
			m_count = 0;
			m_size = 4096;
			m_mask = (m_size - 1);
		}

		inline void				compute_hash(void* data, u32 size, u8 hash[udx_haddress::HASH_SIZE])
		{
			sha1_init(&m_ctx);
			sha1_update(&m_ctx, (const u8*)data, size);
			sha1_final(&m_ctx, (u8*)hash);
		}

		virtual udx_address*	add(void* data, u32 size)
		{
			u8 hash[udx_haddress::HASH_SIZE];
			compute_hash(data, size, hash);
			udx_haddress* h = find_by_hash(hash);
			if (h == NULL)
			{
				h = (udx_haddress*)m_allocator->alloc(sizeof(udx_haddress));
				h->m_addrinlen = size;
				x_memcpy(h->m_hash, hash, sizeof(hash));
				x_memcpy(h->m_addrin, data, size);
				add(h);
			}
			return h;
		}

		virtual udx_address*	add(const char* addr)
		{
			udx_address a;
			a.from_string(addr);
			void* data;
			u32 datasize;
			a.get_addrin(data, datasize);
			return add(data, datasize);
		}

		virtual udx_address*	find(void* data, u32 size)
		{
			u8 hash[udx_haddress::HASH_SIZE];
			compute_hash(data, size, hash);
			udx_haddress* h = find_by_hash(hash);
			return h;
		}

	protected:
		u32					hash_to_index(u8 hash[udx_haddress::HASH_SIZE])
		{
			u32 i = (u32)(hash[8]) | (u32)(hash[9] << 8) | (u32)(hash[10] << 16) | (u32)(hash[11] << 24);
			i = i & m_mask;
			return i;
		}

		udx_haddress*		find_by_hash(u8 hash[udx_haddress::HASH_SIZE])
		{
			u32 const i = hash_to_index(hash);
			udx_haddress* e = m_hashtable[i];
			while (e != NULL)
			{
				if (memcmp(e->m_hash, hash, sizeof(hash)) == 0)
					break;
				e = e->m_next;
			}
			return e;
		}

		void				add(udx_haddress* h)
		{
			u32 const i = hash_to_index(h->m_hash);
			udx_haddress** p = &m_hashtable[i];
			udx_haddress* c = m_hashtable[i];
			while (c != NULL)
			{
				s32 r = memcmp(c->m_hash, h->m_hash, sizeof(udx_haddress::m_hash));
				if (r == -1)
				{
					break;
				}
				p = &c->m_next;
				c = c->m_next;
			}
			*p = h;
			h->m_next = c;
		}

		udx_alloc*			m_allocator;

		u32					m_count;
		u32					m_size;
		u32					m_mask;
		udx_haddress**		m_hashtable;
	};


#ifdef PLATFORM_PC
	s32		udx_address::from_string(const char* addr)
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
		m_addrinlen = 0;
		ZeroMemory(m_addrin, sizeof(m_addrin));

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
				m_addrinlen = ptr->ai_addrlen;
				for (s32 i = 0; i < ptr->ai_addrlen; i++)
				{
					m_addrin[i] = ptr->ai_addr[i];
				}
				break;
			}
		}

		freeaddrinfo(result);
		return 0;
	}

#else
	s32		udx_address::from_string(const char* addr)
	{
		return -1;
	}
#endif

}
