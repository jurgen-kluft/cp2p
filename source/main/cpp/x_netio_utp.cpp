#include "xbase/x_allocator.h"
#include "xbase/x_bit_field.h"

#include "xp2p/private/x_netio.h"
#include "xp2p/private/x_netio_proto.h"
#include "xp2p/private/x_allocator.h"
#include "xp2p/private/x_sockets.h"

#include "xp2p/libutp/utp.h"
#include "xp2p/libutp/utp_callbacks.h"
#include "xp2p/libutp/utp_utils.h"

namespace xcore
{
	namespace xp2p
	{
		typedef void*	ns_socket_t;

		struct buffer
		{
			xbyte*				_data;
			size_t				_length;
			size_t				_cursor;
		};

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

		class ns_server_utp : public ns_iserver, public utp_events, public utp_system, public utp_logger
		{
		public:
			inline				ns_server_utp()
				: allocator(NULL)
				, protocol(NULL)
				, num_active_connections(0)
			{
			}

			enum
			{
				SERVER_MAX_CONNECTIONS = 1024
			};

			void				start(ns_allocator *, io_protocol *);
			void				release();

			s32					bind(const char * addr);

			ns_connection*		connect(netip* ip);
			void				disconnect(ns_connection*);

			s32					poll(s32 milli);

			void				wakeup();
			void				wakeup_ex(void *, u32);

			void				foreach_connection(void *param);

			ns_connection*		create_connection();
			s32					write_data_to_socket();

			ns_allocator *		allocator;
			io_protocol *		protocol;

			s32					num_active_connections;
			ns_connection*		active_connections[SERVER_MAX_CONNECTIONS];

			utp_context*		_utp_ctx;
			utp_socket*			_utp_socket;
			xnet::udpsocket		_udp_socket;

			buffer				_send_buffer;

			XCORE_CLASS_PLACEMENT_NEW_DELETE


		};

		s32		ns_server_utp::write_data_to_socket()
		{
			if (_utp_socket)
			{
				while (_send_buffer._cursor < _send_buffer._length)
				{
					size_t sent = utp_write(_utp_socket, &_send_buffer._data[_send_buffer._cursor], _send_buffer._length - _send_buffer._cursor);
					if (sent == 0)
					{
						// socket no longer writable
						return -1;
					}

					_send_buffer._cursor += sent;
					if (_send_buffer._cursor == _send_buffer._length)
					{
						// buffer now empty
						_send_buffer._cursor = 0;
						return 0;
					}
					else
					{
						// n bytes left in buffer
						return 1;
					}
				}
			}
			return -1;
		}

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

		class ns_connection 
		{
		public:
			inline				ns_connection()
				: server(NULL)
				, last_io_time(0)
				, flags(0)
			{
				memset(&sa, 0, sizeof(socket_address));
			}

			ns_server_utp*			server;
			utp_socket*				sock;
			socket_address			sa;
			time_t					last_io_time;
			u32						flags;

			virtual s32				read(xbyte* _buffer, u32 _size)
			{
				s32 bytes_read = 0;
				

				return bytes_read;
			}

			virtual s32				write(xbyte const* _buffer, u32 _size)
			{
				s32 bytes_written = 0;

				return bytes_written;
			}

			XCORE_CLASS_PLACEMENT_NEW_DELETE
		};




		ns_connection*	ns_server_utp::create_connection()
		{
			void * conn_mem = this->allocator->ns_allocate(sizeof(ns_connection), sizeof(void*));
			if (conn_mem == NULL)
			{
				return NULL;
			}
			return new (conn_mem) ns_connection();
		}


		// ============================================================================================
		// utp callbacks
		// ============================================================================================


		uint16 ns_server_utp::get_udp_mtu(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len)
		{
			return 1440;
		}

		uint16 ns_server_utp::get_udp_overhead(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len)
		{
			return 0;
		}

		uint64 ns_server_utp::get_milliseconds(utp_context *ctx, utp_socket *s)
		{
			return 0;
		}

		uint64 ns_server_utp::get_microseconds(utp_context *ctx, utp_socket *s)
		{
			return 0;
		}

		uint32 ns_server_utp::get_random(utp_context *ctx, utp_socket *s)
		{
			return 0;
		}

		size_t ns_server_utp::get_read_buffer_size(utp_context *ctx, utp_socket *s)
		{
			return 0;
		}

		void ns_server_utp::log(utp_context *ctx, utp_socket *s, const byte *buf)
		{

		}

		void ns_server_utp::sendto(utp_context *ctx, utp_socket *s, const byte *buf, size_t len, const struct sockaddr *address, socklen_t address_len, uint32 flags)
		{
			struct sockaddr_in *sin = (struct sockaddr_in *) address;

			//debug("sendto: %zd byte packet to %s:%d%s\n", a->len, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), (a->flags & UTP_UDP_DONTFRAG) ? "  (DF bit requested, but not yet implemented)" : "");
			//sendto(fd, buf, len, 0, address, address_len);
			//_udp_socket.sendTo(buf, len, ad)
		}
	}
}