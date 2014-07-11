#ifndef __XP2P_NETWORK_IO_H__
#define __XP2P_NETWORK_IO_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

namespace xcore
{
	namespace xnetio
	{
		class ns_allocator
		{
		public:
			virtual void*	alloc(u32 _size, u32 _alignment) = 0;
			virtual void	dealloc(void* _old) = 0;
		};

		// Events. Meaning of event parameter (evp) is given in the comment.
		enum ns_event 
		{
			NS_EVENT_POLL,     // Sent to each connection on each call to ns_server_poll()
			NS_EVENT_ACCEPT,   // New connection accept()-ed. socket_address * remote_addr
			NS_EVENT_CONNECT,  // connect() succeeded or failed. int *success_status
			NS_EVENT_RECV,     // A message has been received. ns_message_header * header
			NS_EVENT_SEND,     // A message has been written to a socket. ns_message_header * header
			NS_EVENT_CLOSE     // Connection is closed. NULL
		};

		// Forward declares
		struct ns_message;
		struct ns_server;
		struct ns_connection;

		// Callback function (event handler) prototype, must be defined by user.
		// Net skeleton will call event handler, passing events defined above.
		typedef void (*ns_callback_t)(ns_connection *, ns_event, void *evp);

		void			ns_server_init(ns_allocator *, ns_server *&, ns_allocator * msg_allocator, void * server_data, ns_callback_t);
		s32				ns_server_bind(ns_server *, const char * addr);
		void			ns_server_free(ns_server *);
		
		s32				ns_server_poll(ns_server *, ns_message *& _out_rcvd_messages, s32 milli);
		
		void			ns_server_wakeup(ns_server *);
		void			ns_server_wakeup_ex(ns_server *, ns_callback_t, void *, u32);
		void			ns_server_foreach_connection(ns_server *, ns_callback_t cb, void *param);

		ns_connection*	ns_connect(ns_server *, const char *host, s32 port, void * connection_param);
		void			ns_send(ns_connection *, ns_message * );
	}
}

#endif // __XP2P_NETWORK_IO_H__
