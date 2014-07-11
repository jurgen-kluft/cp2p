#include "xbase/x_allocator.h"
#include "xbase/x_bit_field.h"

#include "xp2p/private/x_netio.h"
#include "xp2p/private/x_msg.h"
#include "xp2p/private/x_allocator.h"

#undef UNICODE                  // Use ANSI WinAPI functions
#undef _UNICODE                 // Use multi-byte encoding on Windows
#define _MBCS                   // Use multi-byte encoding on Windows
#define _INTEGRAL_MAX_BITS 64   // Enable _stati64() on Windows
#define _CRT_SECURE_NO_WARNINGS // Disable deprecation warning in VS2005+
#undef WIN32_LEAN_AND_MEAN      // Let windows.h always include winsock2.h
#define _XOPEN_SOURCE 600       // For flockfile() on Linux
#define __STDC_FORMAT_MACROS    // <inttypes.h> wants this for C++
#define __STDC_LIMIT_MACROS     // C++ wants that for INT64_MAX
#define _LARGEFILE_SOURCE       // Enable fseeko() and ftello() functions
#define _FILE_OFFSET_BITS 64    // Enable 64-bit file offsets


#ifdef _MSC_VER
#pragma warning (disable : 4127)  // FD_SET() emits warning, disable it
#pragma warning (disable : 4204)  // missing c99 support
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//#include <signal.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")    // Linking with winsock library
#endif

#include <windows.h>
#include <process.h>
#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#ifndef __func__
#define STRX(x) #x
#define STR(x) STRX(x)
#define __func__ __FILE__ ":" STR(__LINE__)
#endif

#ifndef va_copy
#define va_copy(x,y) x = y
#endif // MINGW #defines va_copy

#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define sleep(x) Sleep((x) * 1000)
#define to64(x) _atoi64(x)

typedef SOCKET sock_t;


#ifdef NS_ENABLE_DEBUG
#define DBG(x) do { printf("%-20s ", __func__); printf x; putchar('\n'); fflush(stdout); } while(0)
#else
#define DBG(x)
#endif

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))


namespace xcore
{
	namespace xnetio
	{
		#define NS_SERVER_MAX_CONNECTIONS	32

		#define NSF_CONNECTING              (1 << 0)
		#define NSF_CONNECTED               (1 << 1)
		#define NSF_CLOSE_IMMEDIATELY       (1 << 2)
		#define NSF_ACCEPTED                (1 << 3)
		#define NSF_WANT_READ               (1 << 4)
		#define NSF_WANT_WRITE              (1 << 5)

		#define NSF_USER_1                  (1 << 26)
		#define NSF_USER_2                  (1 << 27)
		#define NSF_USER_3                  (1 << 28)
		#define NSF_USER_4                  (1 << 29)
		#define NSF_USER_5                  (1 << 30)
		#define NSF_USER_6                  (1 << 31)

		#ifndef NS_MALLOC
			#define NS_MALLOC		malloc
		#endif

		#ifndef NS_REALLOC
			#define NS_REALLOC		realloc
		#endif

		#ifndef NS_FREE
			#define NS_FREE			free
		#endif

		typedef s32		socklen_t;

		struct ns_socket_t
		{
			inline		ns_socket_t() : s(INVALID_SOCKET) {}
			
			inline bool	is_valid() const	{ return s != INVALID_SOCKET; }
			inline void	clear()				{ s = INVALID_SOCKET; }

			bool		create(int af, int type, int protocol)
			{
				s = socket(af, type, protocol);
				return s != INVALID_SOCKET;
			}

			SOCKET		s;
		};

		ns_connection*	ns_add_sock(ns_server *, ns_socket_t sock, void *p);

		// Utility functions
		void*			ns_start_thread(void *(*f)(void *), void *p);
		s32				ns_socketpair(ns_socket_t [2]);
		s32				ns_socketpair2(ns_socket_t [2], s32 sock_type);  // SOCK_STREAM or SOCK_DGRAM
		void			ns_set_close_on_exec(ns_socket_t);
		void			ns_sock_to_str(ns_socket_t sock, char * buf, u32 len, s32 flags);
		s32				ns_hexdump(const void * buf, s32 len, char * dst, s32 dst_len);


		union socket_address
		{
			sockaddr		sa;
			sockaddr_in		sin;
#ifdef NS_ENABLE_IPV6
			sockaddr_in6	sin6;
#else
			sockaddr		sin6;
#endif
		};


		struct ns_server
		{
			inline				ns_server()
				: server_data(NULL)
				, listening_sock()
				, callback(NULL)
				, allocator(NULL)
				, num_active_connections(0)
			{
			}

			void*				server_data;
			ns_socket_t			listening_sock;
			ns_callback_t		callback;
			ns_socket_t			ctl[2];
			ns_allocator *		allocator;
			ns_allocator *		msg_allocator;

			s32					num_active_connections;
			ns_connection*		active_connections[NS_SERVER_MAX_CONNECTIONS];

			XCORE_CLASS_PLACEMENT_NEW_DELETE
		};

		struct ns_connection
		{
			inline				ns_connection()
				: server(NULL)
				, connection_data(NULL)
				, last_io_time(0)
				, incoming_message_(NULL)
				, outgoing_messages_(NULL)
				, flags(0)
			{
				memset(&sa, 0, sizeof(socket_address));
			}

			ns_server*				server;
			ns_socket_t				sock;
			socket_address			sa;
			
			u32						connection_id;		// peer-id
			void*					connection_data;
			
			time_t					last_io_time;
			
			ns_message_io_state		header_io_state_;
			ns_message_header		header_;
			ns_message*				incoming_message_;

			ns_message*				outgoing_messages_;

			u32						flags;

			XCORE_CLASS_PLACEMENT_NEW_DELETE
		};


		struct ctl_msg 
		{
			ns_callback_t callback;
			char message[1024 * 8];
		};


		void *ns_start_thread(void *(*f)(void *), void *p)
		{
			return (void *) _beginthread((void (__cdecl *)(void *)) f, 0, p);
		}

		static void ns_add_conn(ns_server *server, ns_connection *c) 
		{
			s32 const n = server->num_active_connections;
			
			// Sorted Insert
			s32 i = 0;
			while ((c->connection_id < server->active_connections[i]->connection_id) && (i < server->num_active_connections))
				++i;

			s32 e = (n+1) - i;
			while (e > i)
			{
				server->active_connections[e] = server->active_connections[e - 1];
				--e;
			}
			server->active_connections[i] = c;
			server->num_active_connections++;
		}

		static void ns_remove_conn(ns_server *server, u32 ci) 
		{
			s32 const n = server->num_active_connections;
			s32 const m = n - ci - 1;

			for (s32 i=ci; i<m; ++i)
			{
				server->active_connections[i] = server->active_connections[i+1];
			}
			--server->num_active_connections;
		}

		static s32 ns_index_of(ns_server *server, ns_connection *conn) 
		{
			s32 const n = server->num_active_connections;
			for (s32 i=0; i<n; i++)
			{
				if (server->active_connections[i] = conn)
				{
					return i;
				}
			}
			return -1;
		}

		static ns_connection * ns_find_connection_by_id(ns_server * server, u32 id)
		{
			s32 const n = server->num_active_connections;
			for (s32 i=0; i<n; i++)
			{
				if (server->active_connections[i]->connection_id = id)
				{
					return server->active_connections[i];
				}
			}
			return NULL;
		}
				
		static void ns_call(ns_connection * conn, ns_event ev, void *p) 
		{
			if (conn->server->callback) conn->server->callback(conn, ev, p);
		}

		static void ns_close_conn(ns_server * server, u32 conn_index)
		{
			DBG(("%p %d", conn, conn->flags));
			ns_connection * conn = server->active_connections[conn_index];
			ns_call(conn, NS_EVENT_CLOSE, NULL);
			while (conn->outgoing_messages_ != NULL)
			{
				ns_message * msg = ns_message_dequeue(conn->outgoing_messages_);
				ns_message_dealloc(msg);
			}
			ns_remove_conn(server, conn_index);
			closesocket(conn->sock.s);
			NS_FREE(conn);
		}

		static void ns_close_conn(ns_server * server, ns_connection * conn)
		{
			s32 i = ns_index_of(server, conn);
			ns_close_conn(server, i);
		}

		void ns_set_close_on_exec(sock_t sock)
		{
			(void) SetHandleInformation((HANDLE) sock, HANDLE_FLAG_INHERIT, 0);
		}

		static void ns_set_non_blocking_mode(ns_socket_t sock)
		{
			unsigned long on = 1;
			ioctlsocket(sock.s, FIONBIO, &on);
		}

		inline static void ns_close_sock(ns_socket_t & _s)
		{
			if (_s.is_valid()) 
			{
				closesocket(_s.s);
				_s.clear();
			}
		}

#ifndef NS_DISABLE_SOCKETPAIR
		s32 ns_socketpair2(ns_socket_t sp[2], s32 sock_type) 
		{
			s32 ret = 0;

			sp[0].clear();
			sp[1].clear();

			ns_socket_t sock;
			if (sock.create(AF_INET, sock_type, 0))
			{
				socket_address sa;
				(void) memset(&sa, 0, sizeof(sa));
				sa.sin.sin_family = AF_INET;
				sa.sin.sin_port = htons(0);
				sa.sin.sin_addr.s_addr = htonl(0x7f000001);

				socklen_t len = sizeof(sa.sin);
				if (!bind(sock.s, &sa.sa, len))
				{
					if (sock_type == SOCK_DGRAM)
					{
						if (sp[0].create(AF_INET, sock_type, 0))
						{
							if (!getsockname(sock.s, &sa.sa, &len) && !connect(sp[0].s, &sa.sa, len))
							{
								sp[1] = sock;
								if ((!getsockname(sp[0].s, &sa.sa, &len) && !connect(sp[1].s, &sa.sa, len)))
								{
									ns_set_close_on_exec(sp[0]);
									ns_set_close_on_exec(sp[1]);
									ret = 1;
								}
							}
						}
					}
					else
					{
						if (!listen(sock.s, 1))
						{
							if (sp[0].create(AF_INET, sock_type, 0))
							{
								if (!getsockname(sock.s, &sa.sa, &len) && !connect(sp[0].s, &sa.sa, len))
								{
									sp[1].s = accept(sock.s, &sa.sa, &len);
									if (sp[1].is_valid())
									{
										ns_set_close_on_exec(sp[0]);
										ns_set_close_on_exec(sp[1]);
										ret = 1;
									}
								}
							}
						} 
					}
				}

				if (sock_type != SOCK_DGRAM)
				{
					ns_close_sock(sock);
				}

				if (ret == 0)
				{
					ns_close_sock(sp[0]);
					ns_close_sock(sp[1]);
				}
			}

			return ret;
		}

		s32 ns_socketpair(ns_socket_t sp[2]) 
		{
			return ns_socketpair2(sp, SOCK_STREAM);
		}
#endif  // NS_DISABLE_SOCKETPAIR

		// Valid listening port spec is: [ip_address:]port, e.g. "80", "127.0.0.1:3128"
		static s32 ns_parse_port_string(const char *str, union socket_address *sa)
		{
			u32 a, b, c, d, port;
			s32 len = 0;

#ifdef NS_ENABLE_IPV6
			char buf[100];
#endif

			// MacOS needs that. If we do not zero it, subsequent bind() will fail.
			// Also, all-zeros in the socket address means binding to all addresses
			// for both IPv4 and IPv6 (INADDR_ANY and IN6ADDR_ANY_INIT).
			memset(sa, 0, sizeof(*sa));
			sa->sin.sin_family = AF_INET;

			if (sscanf(str, "%u.%u.%u.%u:%u%n", &a, &b, &c, &d, &port, &len) == 5)
			{
				// Bind to a specific IPv4 address, e.g. 192.168.1.5:8080
				sa->sin.sin_addr.s_addr = htonl((a << 24) | (b << 16) | (c << 8) | d);
				sa->sin.sin_port = htons((u16) port);
#ifdef NS_ENABLE_IPV6
			} 
			else if (sscanf(str, "[%49[^]]]:%u%n", buf, &port, &len) == 2 && inet_pton(AF_INET6, buf, &sa->sin6.sin6_addr))
			{
				// IPv6 address, e.g. [3ffe:2a00:100:7031::1]:8080
				sa->sin6.sin6_family = AF_INET6;
				sa->sin6.sin6_port = htons((uint16_t) port);
#endif
			}
			else if (sscanf(str, "%u%n", &port, &len) == 1)
			{
				// If only port is specified, bind to IPv4, INADDR_ANY
				sa->sin.sin_port = htons((u16) port);
			} 
			else
			{
				port = 0;   // Parsing failure. Make port invalid.
			}

			return port <= 0xffff && str[len] == '\0';
		}

		// 'sa' must be an initialized address to bind to
		static ns_socket_t ns_open_listening_socket(socket_address *sa)
		{
			socklen_t len = sizeof(*sa);
			ns_socket_t sock;
#ifndef _WIN32
			s32 on = 1;
#endif

			if ((sock.s = socket(sa->sa.sa_family, SOCK_STREAM, 6)) != INVALID_SOCKET &&
#ifndef _WIN32
				// SO_RESUSEADDR is not enabled on Windows because the semantics of
					// SO_REUSEADDR on UNIX and Windows is different. On Windows,
						// SO_REUSEADDR allows to bind a socket to a port without error even if
							// the port is already open by another program. This is not the behavior
								// SO_REUSEADDR was designed for, and leads to hard-to-track failure
									// scenarios. Therefore, SO_REUSEADDR was disabled on Windows.
										!setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on)) &&
#endif
										!bind(sock.s, &sa->sa, sa->sa.sa_family == AF_INET ?
										sizeof(sa->sin) : sizeof(sa->sin6)) &&
										!listen(sock.s, SOMAXCONN)) 
			{
				ns_set_non_blocking_mode(sock);
				// In case port was set to 0, get the real port number
				(void) getsockname(sock.s, &sa->sa, &len);
			} 
			else 
			{
				ns_close_sock(sock);
			}

			return sock;
		}


		s32 ns_server_bind(ns_server *server, const char *str) 
		{
			socket_address sa;
			ns_parse_port_string(str, &sa);
			ns_close_sock(server->listening_sock);
			server->listening_sock = ns_open_listening_socket(&sa);
			return server->listening_sock.is_valid() ? (s32) ntohs(sa.sin.sin_port) : -1;
		}


		static ns_connection *accept_conn(ns_server *server)
		{
			ns_connection *c = NULL;

			socket_address sa;
			socklen_t len = sizeof(sa);

			// NOTE: on Windows, sock is always > FD_SETSIZE
			ns_socket_t sock;
			sock.s = accept(server->listening_sock.s, &sa.sa, &len);
			if (sock.is_valid()) 
			{
				if ((c = (ns_connection *) NS_MALLOC(sizeof(*c))) == NULL || memset(c, 0, sizeof(*c)) == NULL) 
				{
					closesocket(sock.s);
				}
				else 
				{
					ns_set_close_on_exec(sock);
					ns_set_non_blocking_mode(sock);
					c->server = server;
					c->sock = (ns_socket_t)sock;
					c->flags |= NSF_ACCEPTED;

					ns_add_conn(server, c);
					ns_call(c, NS_EVENT_ACCEPT, &sa);
					DBG(("%p %d %p %p", c, c->sock, c->ssl, server->ssl_ctx));
				}
			}

			return c;
		}

		static s32 ns_is_error(s32 n) 
		{
			return n == 0 || (n < 0 && errno != EINTR && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK 
#ifdef _WIN32
				&& WSAGetLastError() != WSAEINTR && WSAGetLastError() != WSAEWOULDBLOCK
#endif
				);
		}

		void ns_sock_to_str(sock_t sock, char *buf, u32 len, s32 flags) 
		{
			union socket_address sa;
			socklen_t slen = sizeof(sa);

			if (buf != NULL && len > 0) 
			{
				buf[0] = '\0';
				memset(&sa, 0, sizeof(sa));
				if (flags & 4) 
				{
					getpeername(sock, &sa.sa, &slen);
				}
				else 
				{
					getsockname(sock, &sa.sa, &slen);
				}

				if (flags & 1) 
				{
#if defined(NS_ENABLE_IPV6)
					inet_ntop(sa.sa.sa_family, sa.sa.sa_family == AF_INET ? (void *) &sa.sin.sin_addr : (void *) &sa.sin6.sin6_addr, buf, len);
#elif defined(_WIN32)
					// Only Windoze Vista (and newer) have inet_ntop()
					strncpy(buf, inet_ntoa(sa.sin.sin_addr), len);
#else
					inet_ntop(sa.sa.sa_family, (void *) &sa.sin.sin_addr, buf, len);
#endif
				}
				if (flags & 2) 
				{
					snprintf(buf + strlen(buf), len - (strlen(buf) + 1), "%s%d", flags & 1 ? ":" : "", (s32) ntohs(sa.sin.sin_port));
				}
			}
		}

		s32 ns_hexdump(const void *buf, s32 len, char *dst, s32 dst_len) 
		{
			const unsigned char *p = (const unsigned char *) buf;
			char ascii[17] = "";
			s32 i, idx, n = 0;

			for (i = 0; i < len; i++) 
			{
				idx = i % 16;
				if (idx == 0) 
				{
					if (i > 0) 
						n += snprintf(dst + n, dst_len - n, "  %s\n", ascii);
					n += snprintf(dst + n, dst_len - n, "%04x ", i);
				}
				n += snprintf(dst + n, dst_len - n, " %02x", p[i]);
				ascii[idx] = p[i] < 0x20 || p[i] > 0x7e ? '.' : p[i];
				ascii[idx + 1] = '\0';
			}

			while (i++ % 16) 
				n += snprintf(dst + n, dst_len - n, "%s", "   ");

			n += snprintf(dst + n, dst_len - n, "  %s\n\n", ascii);

			return n;
		}

		static int ns_read_from_socket(ns_allocator * _allocator, ns_connection * _conn) 
		{
			int result = 0;

			if (_conn->flags & NSF_CONNECTING) 
			{
				s32 ok = 1;
				socklen_t len = sizeof(ok);
				s32 ret = getsockopt(_conn->sock.s, SOL_SOCKET, SO_ERROR, (char *) &ok, &len);

				_conn->flags &= ~NSF_CONNECTING;
				DBG(("%p ok=%d", conn, ok));
				if (ok != 0) 
				{
					_conn->flags |= NSF_CLOSE_IMMEDIATELY;
					result = -1;
				}
				else
				{
					ns_call(_conn, NS_EVENT_CONNECT, &ok);
				}
			}
			else
			{
				if (_conn->header_io_state_.length_ < sizeof(ns_message_header))
				{
					// Start a new message
					s32 n = 0;
					char* rcv_buf = (char*)&_conn->header_;
					while ((n = recv(_conn->sock.s, rcv_buf + _conn->header_io_state_.length_, sizeof(ns_message_header) - _conn->header_io_state_.length_, 0)) > 0) 
					{
						_conn->header_io_state_.length_ += n;
						if (_conn->header_io_state_.length_ == sizeof(ns_message_header))
							break;
					}

					if (ns_is_error(n))
					{
						_conn->header_io_state_.length_ = 0;
						_conn->flags |= NSF_CLOSE_IMMEDIATELY;
						result = -1;
					}
					else
					{
						// Yield when we haven't received a full header
						if (_conn->header_io_state_.length_ != 0)
							return 0;

						// Validate the message header, if it is invalid
						// then terminate this connection.
						if (!_conn->header_.is_valid())
						{
							// Protocol violation
							_conn->flags |= NSF_CLOSE_IMMEDIATELY;
							result = -1;
						}
						else
						{
							// Allocate a message to receive the payload
							ns_message_system system;
							_conn->incoming_message_ = ns_message_alloc(_allocator, system, _conn->header_);
						}
					}
				}
				else
				{
					if (_conn->incoming_message_ == NULL)
					{
						ns_message_system system;
						_conn->incoming_message_ = ns_message_alloc(_allocator, system, _conn->header_);
					}
				}

				if (_conn->incoming_message_ != NULL)
				{
					s32 n = 0;
					while ((n = recv(_conn->sock.s, (char*)(&_conn->incoming_message_->payload_) + _conn->incoming_message_->io_state_.length_, _conn->incoming_message_->header_.length_ - _conn->incoming_message_->io_state_.length_, 0)) > 0) 
					{
						DBG(("%p %d <- %d bytes (PLAIN)", _conn, _conn->flags, n));
						_conn->incoming_message_->io_state_.length_ += n;
						if (_conn->incoming_message_->io_state_.length_ == _conn->incoming_message_->header_.length_)
						{
							// We have received a full message
							// Start to receive a new header
							_conn->header_io_state_.length_ = 0;
							return 1;
						}
					}

					if (ns_is_error(n))
					{
						ns_message_dealloc(_conn->incoming_message_);
						_conn->incoming_message_ = NULL;
						_conn->flags |= NSF_CLOSE_IMMEDIATELY;
						result = -1;
					}
				}
			}
			return result;
		}

		static int ns_write_to_socket(ns_connection * _conn) 
		{
			s32 result = 0;
			ns_message * msg = _conn->outgoing_messages_;
			if (msg == NULL)
				return result;

			u32 s = msg->header_.length_ + sizeof(ns_message_header) - msg->io_state_.length_;
			s32 n = send(_conn->sock.s, (const char*)&msg->header_ + msg->io_state_.length_, s, 0); 

			DBG(("%p %d -> %d bytes", conn, conn->flags, n));
			if (ns_is_error(n)) 
			{
				_conn->flags |= NSF_CLOSE_IMMEDIATELY;
				msg = NULL;
				result = -1;
			}
			else 
			{
				// Did we send a full message ?
				msg->io_state_.length_ += n;
				if (msg->io_state_.length_ == msg->header_.length_) 
				{
					ns_call(_conn, NS_EVENT_SEND, msg);
					ns_message_dequeue(_conn->outgoing_messages_);
					ns_message_dealloc(msg);
					result = 1;
				}
			}
			return result;
		}

		static void ns_add_to_set(ns_socket_t sock, fd_set *set, sock_t *max_fd) 
		{
			if (sock.is_valid()) 
			{
				FD_SET(sock.s, set);
				if (*max_fd == INVALID_SOCKET || sock.s > *max_fd) 
				{
					*max_fd = sock.s;
				}
			}
		}

		s32 ns_server_poll(ns_server * server, ns_message *& _out_rcvd_messages, s32 milli) 
		{
			if (!server->listening_sock.is_valid() || server->num_active_connections == 0) 
				return 0;

			sock_t max_fd = INVALID_SOCKET;
			fd_set read_set, write_set, excp_set;
			FD_ZERO(&read_set);
			FD_ZERO(&write_set);
			FD_ZERO(&excp_set);
			ns_add_to_set(server->listening_sock, &read_set, &max_fd);
			ns_add_to_set(server->ctl[1], &read_set, &max_fd);

			// Mark all the connections that need READ/WRITE
			for (s32 i=0; i<server->num_active_connections; ++i) 
			{
				ns_connection* c = server->active_connections[i];
				xbfSet(c->flags, NSF_WANT_READ);

				if (xbfIsSet(c->flags, NSF_CONNECTING))
				{
					xbfSet(c->flags, NSF_WANT_WRITE);
				}
				else if (c->outgoing_messages_!=NULL)
				{
					xbfSet(c->flags, NSF_WANT_WRITE);
				}
			}

			// populate the read and write set for the 'select' poller
			time_t current_time = time(NULL);
			for (s32 i=0; i<server->num_active_connections; ) 
			{
				ns_connection * conn = server->active_connections[i];
				ns_call(conn, NS_EVENT_POLL, &current_time);
				
				if (xbfIsSet(conn->flags, NSF_CLOSE_IMMEDIATELY))
				{
					ns_close_conn(server, i);
					// closing the connection will remove it from the active
					// connections so we have do not increment the index.
					continue;
				}
				else
				{
					if (xbfIsSet(conn->flags, NSF_CONNECTING))
					{
						//DBG(("%p write_set", conn));
						ns_add_to_set(conn->sock, &write_set, &max_fd);
					}
					else
					{
						if (xbfIsSet(conn->flags, NSF_WANT_READ)) 
						{
							//DBG(("%p read_set", conn));
							ns_add_to_set(conn->sock, &read_set, &max_fd);
						}
						if (xbfIsSet(conn->flags, NSF_WANT_WRITE)) 
						{
							//DBG(("%p write_set", conn));
							ns_add_to_set(conn->sock, &write_set, &max_fd);
						}
					}

					++i;
				}
			}

			timeval tv;
			tv.tv_sec = milli / 1000;
			tv.tv_usec = (milli % 1000) * 1000;

			if (select((s32) max_fd + 1, &read_set, &write_set, &excp_set, &tv) > 0) 
			{
				// select() might have been waiting for a long time, reset current_time
				// now to prevent last_io_time being set to the past.
				current_time = time(NULL);

				// @TODO: Handle exceptions of the listening and ctl[] sockets, we basically
				//        have to restart the server when this happens and the user needs
				//        to know this.
				//        Restarting should have a time-guard so that we don't try and restart
				//        every call.

				// Accept new connections
				if (server->listening_sock.is_valid() && FD_ISSET(server->listening_sock.s, &read_set)) 
				{
					// We're not looping here, and accepting just one connection at
					// a time. The reason is that eCos does not respect non-blocking
					// flag on a listening socket and hangs in a loop.
					ns_connection * conn = accept_conn(server);
					if (conn != NULL) 
					{
						conn->last_io_time = current_time;
					}
				}
				 
				// Read wakeup messages
				if (server->ctl[1].is_valid() && FD_ISSET(server->ctl[1].s, &read_set)) 
				{
					ctl_msg ctl_msg;
					s32 len = recv(server->ctl[1].s, (char *) &ctl_msg, sizeof(ctl_msg), 0);
					send(server->ctl[1].s, ctl_msg.message, 1, 0);
					if (len >= (s32) sizeof(ctl_msg.callback) && ctl_msg.callback != NULL) 
					{
						ns_server_foreach_connection(server, ctl_msg.callback, ctl_msg.message);
					}
				}

				for (s32 i=0; i<server->num_active_connections; ) 
				{
					ns_connection * conn = server->active_connections[i];

					if (xbfIsSet(conn->flags, NSF_CONNECTING))
					{
						// The socket was in the state of connecting and was added to the write set.
						// If it appears in the exception set we will close and remove it.
						if (FD_ISSET(conn->sock.s, &excp_set)) 
						{
							xbfSet(conn->flags, NSF_CLOSE_IMMEDIATELY);
						}
					}

					if (!xbfIsSet(conn->flags, NSF_CLOSE_IMMEDIATELY))
					{
						if (FD_ISSET(conn->sock.s, &read_set)) 
						{
							conn->last_io_time = current_time;
							while (ns_read_from_socket(server->msg_allocator, conn) == 1)
							{
								ns_message* incoming_msg = conn->incoming_message_;
								conn->incoming_message_ = NULL;
								ns_message_enqueue(_out_rcvd_messages, incoming_msg);
							}
						}

						if (FD_ISSET(conn->sock.s, &write_set))
						{
							if (conn->flags & NSF_CONNECTING) 
							{
								xbfClear(conn->flags, NSF_CONNECTING);
								xbfSet(conn->flags, NSF_CONNECTED);
								s32 status = 1;
								ns_call(conn, NS_EVENT_CONNECT, &status);
							} 

							conn->last_io_time = current_time;

							// Write as many messages to the socket
							while (ns_write_to_socket(conn)==1)
							{
							}
						}
					}
				}
			}

			for (s32 i=0; i<server->num_active_connections; ) 
			{
				ns_connection * conn = server->active_connections[i];

				if (conn->flags & NSF_CLOSE_IMMEDIATELY) 
				{
					ns_close_conn(server, i);
				}
				else
				{
					++i;
				}
			}
			//DBG(("%d active connections", num_active_connections));

			return server->num_active_connections;
		}

		ns_connection * ns_connect(ns_server *server, const char *host, s32 port, void *param) 
		{
			ns_socket_t sock;
			sockaddr_in sin;
			hostent *he = NULL;
			ns_connection *conn = NULL;
			s32 connect_ret_val;

			if (host == NULL || (he = gethostbyname(host)) == NULL || (sock.s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
			{
				DBG(("gethostbyname(%s) failed: %s", host, strerror(errno)));
				return NULL;
			}

			sin.sin_family = AF_INET;
			sin.sin_port = htons((u16) port);
			sin.sin_addr = * (in_addr *) he->h_addr_list[0];

			// ! connect in non-blocking mode
			ns_set_non_blocking_mode(sock);
			connect_ret_val = connect(sock.s, (sockaddr *) &sin, sizeof(sin));
			if (ns_is_error(connect_ret_val)) 
			{
				closesocket(sock.s);
				return NULL;
			}
			else 
			{
				void * conn_mem = server->allocator->alloc(sizeof(ns_connection), sizeof(void*));
				if (conn_mem == NULL) 
				{
					closesocket(sock.s);
					return NULL;
				}
				conn = new (conn_mem) ns_connection();
			}

			memset(conn, 0, sizeof(*conn));
			conn->server = server;
			conn->sock = sock;
			conn->connection_data = param;
			conn->flags = NSF_CONNECTING;
			conn->last_io_time = time(NULL);

			ns_add_conn(server, conn);
			DBG(("%p %s:%d %d %p", conn, host, port, conn->sock, conn->ssl));

			return conn;
		}

		void			ns_send(ns_connection * conn, ns_message * msg)
		{
			ns_message_enqueue(conn->outgoing_messages_, msg);
		}

		ns_connection * ns_add_sock(ns_server *s, ns_socket_t sock, void *p) 
		{
			ns_connection *conn;
			if ((conn = (ns_connection *) NS_MALLOC(sizeof(*conn))) != NULL)
			{
				memset(conn, 0, sizeof(*conn));
				ns_set_non_blocking_mode(sock);
				conn->sock = sock;
				conn->connection_data = p;
				conn->server = s;
				conn->last_io_time = time(NULL);
				ns_add_conn(s, conn);
				DBG(("%p %d", conn, sock));
			}
			return conn;
		}

		void ns_server_foreach_connection(ns_server *server, ns_callback_t cb, void *param)
		{
			for (s32 i=0; i<server->num_active_connections; ++i) 
			{
				ns_connection * conn = server->active_connections[i];
				cb(conn, NS_EVENT_POLL, param);
			}
		}

		void ns_server_wakeup_ex(ns_server *server, ns_callback_t cb, void *data, u32 len) 
		{
			ctl_msg lctl_msg;
			if (server->ctl[0].is_valid() && data != NULL && len < sizeof(lctl_msg.message)) 
			{
				lctl_msg.callback = cb;
				memcpy(lctl_msg.message, data, len);
				send(server->ctl[0].s, (char *) &lctl_msg, sizeof(ns_callback_t) + len, 0);
				recv(server->ctl[0].s, (char *) &len, 1, 0);
			}
		}

		void ns_server_wakeup(ns_server *server) 
		{
			ns_server_wakeup_ex(server, NULL, (void *) "", 0);
		}

		void ns_server_init(ns_allocator * _allocator, ns_server *& server, ns_allocator * msg_allocator, void * server_data, ns_callback_t cb) 
		{
			void * ip_mem = _allocator->alloc(sizeof(ns_server), sizeof(void*));
			server = new (ip_mem) ns_server();
			server->allocator = _allocator;
			server->msg_allocator = msg_allocator;

			server->listening_sock.clear();
			server->ctl[0].clear();
			server->ctl[1].clear();
			server->server_data = server_data;
			server->callback = cb;

			{ WSADATA data; WSAStartup(MAKEWORD(2, 2), &data); }

			do 
			{
				ns_socketpair2(server->ctl, SOCK_DGRAM);
			} while (!server->ctl[0].is_valid());

		}

		void ns_server_free(ns_server * s) 
		{
			DBG(("%p", s));
			if (s == NULL) 
				return;

			// Do one last poll, see https://github.com/cesanta/mongoose/issues/286
			ns_message * incoming_messages = NULL;
			ns_server_poll(s, incoming_messages, 0);

			// Release all incoming messages
			while (incoming_messages != NULL)
			{
				ns_message * msg = ns_message_dequeue(incoming_messages);
				ns_message_dealloc(msg);
			}

			ns_close_sock(s->listening_sock);
			ns_close_sock(s->ctl[0]);
			ns_close_sock(s->ctl[1]);

			for (s32 i=0; i<s->num_active_connections; ++i) 
			{
				ns_close_conn(s, i);
			}

			s->allocator->dealloc(s);
		}

	}
}