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

		// Callback function (event handler) prototype, must be defined by user.
		// ns_server_poll will call event handler, passing events defined below.
		class ns_event
		{
		public:
			// Events. Meaning of event parameter (evp) is given in the comment.
			enum event 
			{
				EVENT_POLL,     // Sent to each connection on each call to ns_server_poll()
				EVENT_ACCEPT,   // New connection accept()-ed. socket_address * remote_addr
				EVENT_CONNECT,  // Connect() succeeded or failed. int *success_status
				EVENT_CLOSE     // Connection is closed. NULL
			};

			virtual void	ns_callback(io_connection, event, void *evp) = 0;
		};

		void			ns_server_init(ns_allocator *, ns_server *&, io_protocol * , void * server_data, ns_event*);
		s32				ns_server_bind(ns_server *, const char * addr);
		void			ns_server_free(ns_server *);
		
		s32				ns_server_poll(ns_server *, s32 milli);

		ns_connection*	ns_connect(ns_server *, netip4 ip, void * connection_param);
		void			ns_disconnect(ns_server *, ns_connection*);

		void			ns_server_wakeup(ns_server *);
		void			ns_server_wakeup_ex(ns_server *, ns_event*, void *, u32);
		void			ns_server_foreach_connection(ns_server *, ns_event*, void *param);
	}
}

#endif // __XP2P_NETWORK_IO_H__
