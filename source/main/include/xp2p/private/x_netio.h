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
		class ns_connection;
		class io_protocol;

		class ns_iserver
		{
		public:
			virtual void			start(ns_allocator *, io_protocol *, void * server_data) = 0;
			virtual void			release() = 0;

			virtual s32				bind(const char * addr) = 0;
			virtual ns_connection*	connect(netip4 ip, void * connection_param) = 0;
			virtual void			disconnect(ns_connection*) = 0;

			virtual s32				poll(s32 milli) = 0;

			virtual void			wakeup() = 0;
			virtual void			wakeup_ex(void *, u32) = 0;

			virtual void			foreach_connection(void *param) = 0;
		};

		ns_iserver*			ns_create_server(ns_allocator*);
	}
}

#endif // __XP2P_NETWORK_IO_H__
