#ifndef __XP2P_NETWORK_IO_H__
#define __XP2P_NETWORK_IO_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

namespace xcore
{
	namespace xp2p
	{
		class ns_allocator
		{
		public:
			virtual void*	ns_allocate(u32 _size, u32 _alignment) = 0;
			virtual void	ns_deallocate(void* _old) = 0;
		};

		// Forward declares
		struct ns_server;
		class ns_connection;
		class io_protocol;

		void			ns_server_init(ns_allocator *, ns_server *&, io_protocol * , void * server_data);
		s32				ns_server_bind(ns_server *, const char * addr);
		void			ns_server_free(ns_server *);
		
		s32				ns_server_poll(ns_server *, s32 milli);

		ns_connection*	ns_connect(ns_server *, netip4 ip, void * connection_param);
		void			ns_disconnect(ns_server *, ns_connection*);

		void			ns_server_wakeup(ns_server *);
		void			ns_server_wakeup_ex(ns_server *, void *, u32);
		void			ns_server_foreach_connection(ns_server *, void *param);
	}
}

#endif // __XP2P_NETWORK_IO_H__
