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
		// IO buffers interface
		struct iobuf 
		{
			xbyte*		buf;
			u32			len;
			u32			size;
		};

		void		iobuf_init(iobuf *, u32 initial_size);
		void		iobuf_free(iobuf *);
		u32			iobuf_append(iobuf *, const void * data, u32 data_size);
		void		iobuf_remove(iobuf *, u32 data_size);

		// Net skeleton interface
		// Events. Meaning of event parameter (evp) is given in the comment.
		enum ns_event 
		{
			NS_POLL,     // Sent to each connection on each call to ns_server_poll()
			NS_ACCEPT,   // New connection accept()-ed. union socket_address *remote_addr
			NS_CONNECT,  // connect() succeeded or failed. int *success_status
			NS_RECV,     // Data has benn received. int *num_bytes
			NS_SEND,     // Data has been written to a socket. int *num_bytes
			NS_CLOSE     // Connection is closed. NULL
		};

		// Forward declares
		typedef	u64	ns_socket_t;
		struct ns_server;
		struct ns_connection;

		// Callback function (event handler) prototype, must be defined by user.
		// Net skeleton will call event handler, passing events defined above.
		typedef void (*ns_callback_t)(ns_connection *, ns_event, void *evp);

		void			ns_server_init(ns_server *, void * server_data, ns_callback_t);
		void			ns_server_free(ns_server *);
		int				ns_server_poll(ns_server *, s32 milli);
		void			ns_server_wakeup(ns_server *);
		void			ns_server_wakeup_ex(ns_server *, ns_callback_t, void *, u32);
		void			ns_iterate(ns_server *, ns_callback_t cb, void *param);
		ns_connection*	ns_next(ns_server *, ns_connection *);
		ns_connection*	ns_add_sock(ns_server *, ns_socket_t sock, void *p);

		int				ns_bind(ns_server *, const char * addr);
		int				ns_set_ssl_cert(ns_server *, const char * ssl_cert);
		int				ns_set_ssl_ca_cert(ns_server *, const char * ssl_ca_cert);
		ns_connection*	ns_connect(ns_server *, const char *host, s32 port, s32 ssl, void * connection_param);

		int				ns_send(ns_connection *, const void * buf, s32 len);

		// Utility functions
		void*			ns_start_thread(void *(*f)(void *), void *p);
		int				ns_socketpair(ns_socket_t [2]);
		int				ns_socketpair2(ns_socket_t [2], s32 sock_type);  // SOCK_STREAM or SOCK_DGRAM
		void			ns_set_close_on_exec(ns_socket_t);
		void			ns_sock_to_str(ns_socket_t sock, char * buf, u32 len, s32 flags);
		int				ns_hexdump(const void * buf, s32 len, char * dst, s32 dst_len);

	}
}

#endif // __XP2P_NETWORK_IO_H__
