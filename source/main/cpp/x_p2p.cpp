#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "xp2p\x_p2p.h"
#include "xp2p\x_peer.h"
#include "xp2p\x_msg.h"
#include "xp2p\private\x_allocator.h"
#include "xp2p\private\x_peer_registry.h"
#include "xp2p\private\x_netio.h"
#include "xp2p\private\x_netio_proto.h"

namespace xcore
{
	namespace xp2p
	{
		class peer_connection;

		class peer : public ipeer
		{
		public:
			peer() 
				: is_remote_(false)
				, status_(INACTIVE)
				, endpoint_()
			{
			}

			// peer interface
			virtual bool		is_remote() const			{ return is_remote_; }
			virtual estatus		get_status() const			{ return status_; }
			virtual netip4		get_ip4() const				{ return endpoint_; }

			inline bool			is_not_connected() const	{ estatus s = get_status(); return s==0 || s>=DISCONNECT; }
			inline void			set_status(estatus s)		{ status_ = s; }

		protected:
			bool				is_remote_;
			estatus				status_;
			netip4				endpoint_;
		};

		class node::node_imp : public peer, public io_protocol, public ns_event, public ns_allocator
		{
		public:
								node_imp(iallocator* _allocator, imessage_allocator* _message_allocator);
								~node_imp();

			ipeer*				start(netip4 _endpoint);
			void				stop();

			ipeer*				register_peer(netip4 _endpoint);
			void				unregister_peer(ipeer*);

			void				connect_to(ipeer* _peer);
			void				disconnect_from(ipeer* _peer);

			u32					connections(ipeer** _out_peers, u32 _in_max_peers);

			void				send(outgoing_messages&);

			void				event_wakeup();
			bool				event_loop(incoming_messages*& _rcvd, outgoing_messages*& _sent, u32 _ms_to_wait);

			// ns_event interface
			virtual void		ns_callback(ns_connection *, event, void *evp);

			// ns_allocator interface
			virtual void*		ns_allocate(u32 _size, u32 _alignment);
			virtual void		ns_deallocate(void* _old);

			// io_protocol interface
			virtual io_connection io_open(void* _key, netip4 _ip);
			virtual void		io_close(io_connection);

			virtual bool		io_needs_write(io_connection);
			virtual bool		io_needs_read(io_connection);

			virtual s32			io_write(io_connection, io_writer*);
			virtual s32			io_read(io_connection, io_reader*);

			XCORE_CLASS_PLACEMENT_NEW_DELETE

			iallocator*			allocator_;
			imessage_allocator*	message_allocator_;
			ns_server*			server_;
			
			lqueue<peer_connection>* inactive_peers_;
			lqueue<peer_connection>* active_peers_;

			incoming_messages	incoming_messages_;
		};

		class peer_connection : public peer, public lqueue<peer_connection>
		{
		public:
			peer_connection() 
				: peer()
				, lqueue<peer_connection>(this) 
				, connection_(NULL)
			{
			}

			XCORE_CLASS_PLACEMENT_NEW_DELETE

			ns_connection*		connection_;

			outgoing_messages	outgoing_messages_;
			incoming_messages	incoming_messages_;
		};


		node::node_imp::node_imp(iallocator* _allocator, imessage_allocator* _message_allocator)
			: peer()
			, allocator_(_allocator)
			, message_allocator_(_message_allocator)
			, inactive_peers_(NULL)
			, active_peers_(NULL)
		{

		}

		node::node_imp::~node_imp()
		{

		}

		ipeer*	node::node_imp::start(netip4 _endpoint)
		{
			ns_server_init(this, server_, this, this, this);
			return this;
		}

		void	node::node_imp::stop()
		{
			ns_server_free(server_);
		}

		peer_connection*	_find_peer(lqueue<peer_connection>* _peers, netip4 _endpoint)
		{
			peer_connection* peer = NULL;
			if (_peers == NULL)
				return peer;

			peer_connection* iter = _peers->get();
			while (iter != NULL)
			{
				if (iter->get_ip4() == _endpoint)
				{
					peer = iter;
					break;
				}
				iter = iter->get_next();
			}
			return peer;
		}

		ipeer*	node::node_imp::register_peer(netip4 _endpoint)
		{
			peer_connection* peer = _find_peer(inactive_peers_, _endpoint);
			if (peer == NULL)
			{
				peer = _find_peer(active_peers_, _endpoint);
			}

			if (peer == NULL)
			{
				void * mem = allocator_->allocate(sizeof(peer_connection), sizeof(void*));
				peer = new (mem) peer_connection();
			}

			return peer;
		}

		void	node::node_imp::unregister_peer(ipeer* _peer)
		{
			peer_connection* p = (peer_connection*) _peer;
			if (p == active_peers_)
				active_peers_ = active_peers_->get_next();
			else if (p == inactive_peers_)
				inactive_peers_ = inactive_peers_->get_next();
			p->dequeue();
		}

		void	node::node_imp::connect_to(ipeer* _peer)
		{
			if (_peer == this)	// Do not connect to ourselves
				return;
			
			peer_connection* p = (peer_connection*) _peer;
			if (p->connection_ == NULL)
			{
				p->set_status(ipeer::CONNECT);
				p->connection_ = ns_connect(server_, _peer->get_ip4(), (void*)_peer);

			}
		}

		void	node::node_imp::disconnect_from(ipeer* _peer)
		{
			peer_connection* p = (peer_connection*) _peer;
			ns_disconnect(server_, p->connection_);
		}

		u32		node::node_imp::connections(ipeer** _out_peers, u32 _in_max_peers)
		{
			u32 i = 0;
			if (active_peers_ != NULL)
			{
				peer_connection* iter = active_peers_->get();
				while (iter != NULL && i < _in_max_peers)
				{
					_out_peers[i++] = iter;
					iter = iter->get_next();
				}
			}
			return i;
		}

		void	node::node_imp::send(outgoing_messages& _msg)
		{
			message* m = _msg.dequeue();
			while (m != NULL)
			{
				peer_connection* to = (peer_connection*)(m->get_to());
				to->outgoing_messages_.enqueue(m);
				m = _msg.dequeue();
			}
		}

		void	node::node_imp::event_wakeup()
		{
			ns_server_wakeup(server_);
		}

		bool	node::node_imp::event_loop(incoming_messages*&, outgoing_messages*& _sent, u32 _ms_to_wait)
		{
			return false;
		}

		// -------------------------------------------------------------------------------------------

		void	node::node_imp::ns_callback(ns_connection * conn, event e, void *evp)
		{
			switch (e)
			{
				case ns_event::EVENT_POLL: 
					break;
				case ns_event::EVENT_ACCEPT: 
					break;
				case ns_event::EVENT_CONNECT: 
					break;
				case ns_event::EVENT_RECV: 
					break;
				case ns_event::EVENT_SEND: 
					break;
				case ns_event::EVENT_CLOSE: 
					break;
			}
		}

		// -------------------------------------------------------------------------------------------

		void*		node::node_imp::ns_allocate(u32 _size, u32 _alignment)
		{
			return allocator_->allocate(_size, _alignment);
		}

		void		node::node_imp::ns_deallocate(void* _old)
		{
			allocator_->deallocate(_old);
		}

		// -------------------------------------------------------------------------------------------

		io_connection node::node_imp::io_open(void* _key, netip4 _ip)
		{
			// Search in the peer registry, if not found create a new peer connection
			// Set the key on the peer connection so that we can find it next time

			return NULL;
		}

		void		node::node_imp::io_close(io_connection)
		{

		}

		bool		node::node_imp::io_needs_write(io_connection)
		{
			return false;
		}

		bool		node::node_imp::io_needs_read(io_connection)
		{
			return false;
		}

		s32			node::node_imp::io_write(io_connection, io_writer*)
		{
			return 0;
		}

		s32			node::node_imp::io_read(io_connection, io_reader*)
		{
			return 0;
		}


		// -------------------------------------------------------------------------------------------
		// P2P Node
		// -------------------------------------------------------------------------------------------

		node::node(iallocator* _allocator, imessage_allocator* _message_allocator) 
			: allocator_(_allocator)
			, message_allocator_(_message_allocator)
			, imp_(NULL)
		{
		}

		ipeer*				node::start(netip4 _endpoint)
		{
			void * mem = allocator_->allocate(sizeof(node::node_imp), sizeof(void*));
			imp_ = new (mem) node::node_imp(allocator_, message_allocator_);
			return imp_->start(_endpoint);
		}

		void				node::stop()
		{
			imp_->stop();
		}

		ipeer*				node::register_peer(netip4 _endpoint)
		{
			return imp_->register_peer(_endpoint);
		}

		void				node::unregister_peer(ipeer* _peer)
		{
			imp_->unregister_peer(_peer);
		}

		void				node::connect_to(ipeer* _peer)
		{
			imp_->connect_to(_peer);
		}

		void				node::disconnect_from(ipeer* _peer)
		{
			imp_->disconnect_from(_peer);
		}

		u32					node::connections(ipeer** _out_peers, u32 _in_max_peers)
		{
			return imp_->connections(_out_peers, _in_max_peers);
		}

		void				node::send(outgoing_messages& _msg)
		{
			imp_->send(_msg);
		}

		void				node::event_wakeup()
		{
			imp_->event_wakeup();
		}

		bool				node::event_loop(incoming_messages*& _msgs, outgoing_messages*& _sent, u32 _wait_in_ms)
		{
			return imp_->event_loop(_msgs, _sent, _wait_in_ms);
		}
	}
}