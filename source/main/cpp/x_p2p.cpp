#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "xp2p\x_p2p.h"
#include "xp2p\x_peer.h"
#include "xp2p\x_msg.h"
#include "xp2p\private\x_allocator.h"
#include "xp2p\private\x_peer_registry.h"
#include "xp2p\private\x_netio.h"
#include "xp2p\private\x_sockets.h"

#include "xp2p/libutp/utp.h"
#include "xp2p/libutp/utp_callbacks.h"
#include "xp2p/libutp/utp_utils.h"

namespace xcore
{
	namespace xp2p
	{
		class peer_connection;

		class xpeer : public peer, public lqueue<xpeer>
		{
		public:
			xpeer()
				: is_remote_(false)
				, status_(INACTIVE)
				, endpoint_()
				, lqueue<xpeer>(this)
			{
			}

			// peer interface
			virtual bool		is_remote() const { return is_remote_; }
			virtual estatus		get_status() const { return status_; }
			virtual netip const* get_ip() const { return &endpoint_; }

			inline bool			is_not_connected() const { estatus s = get_status(); return s == 0 || s >= DISCONNECT; }
			inline void			set_status(estatus s) { status_ = s; }

			enum epeer { LOCAL_PEER, REMOTE_PEER };
			void				init(epeer _peer, estatus _status, netip* _ip)
			{
				is_remote_ = _peer == REMOTE_PEER;
				status_ = _status;
				endpoint_ = *_ip;
			}

			XCORE_CLASS_PLACEMENT_NEW_DELETE

			utp_socket*			socket_;
			sockaddr*			socket_address_;
			socklen_t			socket_address_len;

			bool				is_remote_;
			estatus				status_;
			netip				endpoint_;
		};


		struct xbuffer
		{
			xbyte*				_data;
			size_t				_length;
			size_t				_cursor;
		};
		
		class xnode : public node, public xpeer, public ns_allocator, public utp_events, public utp_system, public utp_logger
		{
		public:
								xnode();
								~xnode();

			// ============================================================================================
			// node interface
			// ============================================================================================

			peer*				start(netip* endpoint, allocator* _allocator, message_allocator* _message_allocator);
			void				stop();

			peer*				register_peer(netip* endpoint);
			void				unregister_peer(peer*);

			void				connect_to(peer* peer);
			void				disconnect_from(peer* peer);

			u32					connections(peer** _out_peers, u32 _in_max_peers);
			void				send(outgoing_messages&);

			void				event_wakeup();
			bool				event_loop(incoming_messages*& _received, garbagec_messages*& _sent, u32 _ms_to_wait = 0);

			s32					write_messages();

			// ============================================================================================
			// ns_allocator interface
			// ============================================================================================

			virtual void*		ns_allocate(u32 _size, u32 _alignment);
			virtual void		ns_deallocate(void* _old);


			// ============================================================================================
			// utp events
			// ============================================================================================

			virtual int			on_firewall(utp_context *ctx, const struct sockaddr *address, socklen_t address_len);
			virtual void		on_accept(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len);
			virtual void		on_connect(utp_context *ctx, utp_socket *s);
			virtual void		on_error(utp_context *ctx, utp_socket *s, int error_code);
			virtual void		on_read(utp_context *ctx, utp_socket *s, const byte *buf, size_t len);
			virtual void		on_overhead_statistics(utp_context *ctx, utp_socket *s, int send, size_t len, int type);
			virtual void		on_delay_sample(utp_context *ctx, utp_socket *s, int sample_ms);
			virtual void		on_state_change(utp_context *ctx, utp_socket *s, int state);

			// ============================================================================================
			// utp system
			// ============================================================================================

			virtual uint16		get_udp_mtu(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len);
			virtual uint16		get_udp_overhead(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len);
			virtual uint64		get_milliseconds(utp_context *ctx, utp_socket *s);
			virtual uint64		get_microseconds(utp_context *ctx, utp_socket *s);
			virtual uint32		get_random(utp_context *ctx, utp_socket *s);
			virtual size_t		get_read_buffer_size(utp_context *ctx, utp_socket *s);
			virtual void		log(utp_context *ctx, utp_socket *s, const byte *buf);
			virtual void		sendto(utp_context *ctx, utp_socket *s, const byte *buf, size_t len, const struct sockaddr *address, socklen_t address_len, uint32 flags);

			XCORE_CLASS_PLACEMENT_NEW_DELETE

			allocator*			allocator_;
			message_allocator*	message_allocator_;

			xnet::udpsocket		udp_socket_;
			utp_context*		utp_context_;

			lqueue<xpeer>*		inactive_peers_;
			lqueue<xpeer>*		active_peers_;

			incoming_messages	incoming_messages_;
			outgoing_messages	outgoing_messages_;
		};



		xnode::xnode()
			: xpeer()
			, allocator_(NULL)
			, message_allocator_(NULL)
			, inactive_peers_(NULL)
			, active_peers_(NULL)
		{

		}

		xnode::~xnode()
		{

		}

		peer*	xnode::start(netip* _endpoint, allocator* _allocator, message_allocator* _message_allocator)
		{
			init(xpeer::LOCAL_PEER, CONNECTED, _endpoint);

			// Open the UDP socket 
			udp_socket_.open("0.0.0.0", _endpoint->get_port());

			return this;
		}

		void	xnode::stop()
		{
			
		}

		xpeer*	_find_peer(lqueue<xpeer>* _peers, netip* _endpoint)
		{
			xpeer* peer = NULL;
			if (_peers == NULL)
				return peer;

			xpeer* iter = _peers->get_this();
			while (iter != NULL)
			{
				if (iter->get_ip() == _endpoint)
				{
					peer = iter;
					break;
				}
				iter = iter->get_next();
			}
			return peer;
		}

		peer*	xnode::register_peer(netip* _endpoint)
		{
			xpeer* peer = _find_peer(inactive_peers_, _endpoint);
			if (peer == NULL)
			{
				peer = _find_peer(active_peers_, _endpoint);
			}

			if (peer == NULL)
			{
				void * mem = allocator_->allocate(sizeof(xpeer), sizeof(void*));
				peer = new (mem) xpeer();
				peer->init(xpeer::REMOTE_PEER, INACTIVE, _endpoint);
			}

			return peer;
		}

		void	xnode::unregister_peer(peer* _peer)
		{
			xpeer* p = (xpeer*) _peer;
			if (p == active_peers_)
				active_peers_ = active_peers_->get_next();
			else if (p == inactive_peers_)
				inactive_peers_ = inactive_peers_->get_next();
			p->dequeue();
		}

		void	xnode::connect_to(peer* _peer)
		{
			if (_peer == this)	// Do not connect to ourselves
				return;
			
			xpeer* p = (xpeer*) _peer;
			if (p->socket_ == NULL)
			{
				p->set_status(peer::CONNECT);
				p->socket_ = utp_create_socket(utp_context_);
				utp_connect(p->socket_, p->socket_address_, p->socket_address_len);
			}
		}

		void	xnode::disconnect_from(peer* _peer)
		{
			xpeer* p = (xpeer*) _peer;
			utp_close(p->socket_);
			p->socket_ = NULL;
			p->set_status(peer::DISCONNECTING);
		}

		u32		xnode::connections(peer** _out_peers, u32 _in_max_peers)
		{
			u32 i = 0;
			if (active_peers_ != NULL)
			{
				xpeer* iter = active_peers_->get_this();
				while (iter != NULL && i < _in_max_peers)
				{
					_out_peers[i++] = iter;
					iter = iter->get_next();
				}
			}
			return i;
		}

		void	xnode::send(outgoing_messages& _msg)
		{
			message* m = _msg.dequeue();
			while (m != NULL)
			{
				outgoing_messages_.enqueue(m);
				m = _msg.dequeue();
			}
		}

		void	xnode::event_wakeup()
		{
			
		}

		bool	xnode::event_loop(incoming_messages*&, garbagec_messages*& _sent, u32 _ms_to_wait)
		{

			
			return true;
		}
		

		s32		xnode::write_messages()
		{
			if (utp_context_ != NULL)
			{
				while (outgoing_messages_.has_message())
				{
					message* msg = outgoing_messages_.peek();
					message_block* msg_block = msg->get_block();

					// Get utp socket that is associated with where this message needs to be sent to.
					xpeer* to = (xpeer*)msg->get_to();

					size_t sent = utp_write(to->socket_, (void*)msg_block->get_data(), (size_t)msg_block->get_size());
					if (sent == 0)
					{
						break;	// socket no longer writable
					}

					outgoing_messages_.dequeue();
				}
			}
			return -1;
		}

		// -------------------------------------------------------------------------------------------
		// ns_allocator
		// -------------------------------------------------------------------------------------------

		void*		xnode::ns_allocate(u32 _size, u32 _alignment)
		{
			return allocator_->allocate(_size, _alignment);
		}

		void		xnode::ns_deallocate(void* _old)
		{
			allocator_->deallocate(_old);
		}

		// -------------------------------------------------------------------------------------------
		// utp_events
		// -------------------------------------------------------------------------------------------

		int xnode::on_firewall(utp_context *ctx, const struct sockaddr *address, socklen_t address_len)
		{
			//debug("Firewall allowing inbound connection\n");
			return 0;
		}

		void xnode::on_accept(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len)
		{
			//debug("Accepted inbound socket %p\n", s);
			write_messages();

			// Find peer that matches the [address, address_len]

		}

		void xnode::on_connect(utp_context *ctx, utp_socket *s)
		{

		}

		void xnode::on_error(utp_context *ctx, utp_socket *s, int error_code)
		{
			//fprintf(stderr, "Error: %s\n", utp_error_code_names[a->error_code]);
			utp_close(s);
			s = NULL;
			//quit_flag = 1;
			//exit_code++;
		}

		void xnode::on_read(utp_context *ctx, utp_socket *s, const byte *buffer, size_t len)
		{
			// Do something with the packet ptr which actually is a message_block*
			// The byte* buffer that is given here is the data after the utp header, we could
			// have utplib also pass the packet ptr. The packet ptr could hold a pointer just
			// before it in memory which points to the start of the message_block. We also
			// need to add a feature to message_block to hold an original offset into the
			// buffer.
			// The result of all this is that utplib will not malloc/free packets anymore but
			// use a message_allocator*, by doing this we can implement re-use of memory and
			// reduce the allocation and deallocation behaviour as well as increase performance
			// by removing memory copy calls.

			utp_read_drained(s);
		}

		void xnode::on_overhead_statistics(utp_context *ctx, utp_socket *s, int send, size_t len, int type)
		{

		}

		void xnode::on_delay_sample(utp_context *ctx, utp_socket *s, int sample_ms)
		{

		}

		void xnode::on_state_change(utp_context *ctx, utp_socket *s, int state)
		{
			//debug("state %d: %s\n", a->state, utp_state_names[a->state]);
			utp_socket_stats *stats;

			switch (state)
			{
			case UTP_STATE_CONNECT:
			case UTP_STATE_WRITABLE:
				write_messages();
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

		// -------------------------------------------------------------------------------------------
		// utp_system
		// -------------------------------------------------------------------------------------------

	}
}