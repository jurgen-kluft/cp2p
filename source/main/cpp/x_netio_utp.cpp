#include "xbase/x_allocator.h"
#include "xbase/x_bit_field.h"

#include "xp2p/private/x_netio.h"
#include "xp2p/private/x_netio_proto.h"
#include "xp2p/private/x_allocator.h"

#include "xp2p/libutp/utp.h"
#include "xp2p/libutp/utp_callbacks.h"

namespace xcore
{
	namespace xp2p
	{
		typedef void*	ns_socket_t;

		class ns_server_utp : public ns_iserver
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

			ns_connection*		connect(netip4 ip);
			void				disconnect(ns_connection*);

			s32					poll(s32 milli);

			void				wakeup();
			void				wakeup_ex(void *, u32);

			void				foreach_connection(void *param);

			ns_connection*		accept_conn();
			ns_connection*		create_connection();

			ns_connection*		add_sock(ns_socket_t sock);
			void				add_conn(ns_connection *c);
			void				close_conn(u32 conn_index);
			void				close_conn(ns_connection * conn);
			s32					index_of(ns_connection *conn);
			void				remove_conn(u32 ci);

			s32					read_from_socket(ns_connection * _conn);
			s32					write_to_socket(ns_connection * _conn);

			void				callback(ns_connection * conn, io_protocol::event ev, void *p);

			s32					write_data_to_socket();

			ns_allocator *		allocator;
			io_protocol *		protocol;

			s32					num_active_connections;
			ns_connection*		active_connections[SERVER_MAX_CONNECTIONS];

			utp_context*		_utp_ctx;
			utp_socket*			_utp_socket;

			xbyte*				_send_buffer;
			size_t				_send_buffer_length;
			size_t				_send_buffer_cursor;

			XCORE_CLASS_PLACEMENT_NEW_DELETE
		};

		class my_utp_callbacks : public utp_callbacks
		{
			utp_context*		_utp_ctx;
			utp_socket*			_utp_socket;

		public:
			virtual int on_firewall(utp_context *ctx, const struct sockaddr *address, socklen_t address_len)
			{
				bool o_listen = false;

				if (!o_listen)
				{
					//debug("Firewalling unexpected inbound connection in non-listen mode\n");
					return 1;
				}

				if (_utp_socket)
				{
					//debug("Firewalling unexpected second inbound connection\n");
					return 1;
				}

				//debug("Firewall allowing inbound connection\n");
				return 0;
			}

			virtual void on_accept(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len)
			{
				assert(!s);
				//debug("Accepted inbound socket %p\n", s);
				write_data();
			}

			virtual void on_connect(utp_context *ctx, utp_socket *s)
			{

			}

			virtual void on_error(utp_context *ctx, utp_socket *s, int error_code)
			{
				//fprintf(stderr, "Error: %s\n", utp_error_code_names[a->error_code]);
				utp_close(s);
				s = NULL;
				//quit_flag = 1;
				//exit_code++;
			}

			virtual void on_read(utp_context *ctx, utp_socket *s, const byte *buf, size_t len)
			{
				
				// Do something with the packet

				utp_read_drained(s);
			}

			virtual void on_overhead_statistics(utp_context *ctx, utp_socket *s, int send, size_t len, int type)
			{

			}

			virtual void on_delay_sample(utp_context *ctx, utp_socket *s, int sample_ms)
			{

			}

			virtual void on_state_change(utp_context *ctx, utp_socket *s, int state)
			{
				//debug("state %d: %s\n", a->state, utp_state_names[a->state]);
				utp_socket_stats *stats;

				switch (state)
				{
				case UTP_STATE_CONNECT:
				case UTP_STATE_WRITABLE:
					write_data();
					break;

				case UTP_STATE_EOF:
					//debug("Received EOF from socket; closing\n");
					utp_close(s);
					break;

				case UTP_STATE_DESTROYING:
					//debug("UTP socket is being destroyed; exiting\n");

					stats = utp_get_stats(s);
					if (stats) {
						//debug("Socket Statistics:\n");
						//debug("    Bytes sent:          %d\n", stats->nbytes_xmit);
						//debug("    Bytes received:      %d\n", stats->nbytes_recv);
						//debug("    Packets received:    %d\n", stats->nrecv);
						//debug("    Packets sent:        %d\n", stats->nxmit);
						//debug("    Duplicate receives:  %d\n", stats->nduprecv);
						//debug("    Retransmits:         %d\n", stats->rexmit);
						//debug("    Fast Retransmits:    %d\n", stats->fastrexmit);
						//debug("    Best guess at MTU:   %d\n", stats->mtu_guess);
					}
					else
					{
						// debug("No socket statistics available\n");
					}

					s = NULL;
					//quit_flag = 1;
					break;
				}
			}

			virtual uint16 get_udp_mtu(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len)
			{
				return 1440;
			}

			virtual uint16 get_udp_overhead(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len)
			{
				return 0;
			}

			virtual uint64 get_milliseconds(utp_context *ctx, utp_socket *s)
			{
				return 0;
			}

			virtual uint64 get_microseconds(utp_context *ctx, utp_socket *s)
			{
				return 0;
			}

			virtual uint32 get_random(utp_context *ctx, utp_socket *s)
			{
				return 0;
			}

			virtual size_t get_read_buffer_size(utp_context *ctx, utp_socket *s)
			{
				return 0;
			}

			virtual void log(utp_context *ctx, utp_socket *s, const byte *buf)
			{
				
			}

			virtual void sendto(utp_context *ctx, utp_socket *s, const byte *buf, size_t len, const struct sockaddr *address, socklen_t address_len, uint32 flags)
			{
				struct sockaddr_in *sin = (struct sockaddr_in *) address;

				//debug("sendto: %zd byte packet to %s:%d%s\n", a->len, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), (a->flags & UTP_UDP_DONTFRAG) ? "  (DF bit requested, but not yet implemented)" : "");
				sendto(fd, buf, len, 0, address, address_len);
			}

		};

		s32		ns_server_utp::write_data_to_socket()
		{
			if (_utp_socket)
			{
				while (_send_buffer_cursor < _send_buffer_length)
				{
					size_t sent = utp_write(_utp_socket, &_send_buffer[_send_buffer_cursor], _send_buffer_length - _send_buffer_cursor);
					if (sent == 0)
					{
						// socket no longer writable
						return -1;
					}

					_send_buffer_cursor += sent;
					if (_send_buffer_cursor == _send_buffer_length)
					{
						// buffer now empty
						_send_buffer_cursor = 0;
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


	}
}