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
		class peer : public ipeer
		{
		public:
			virtual bool		is_remote() const		{ return is_remote_; }
			virtual estatus		get_status() const		{ return status_; }
			virtual netip4		get_ip4() const			{ return endpoint_; }
			virtual peerid		get_id() const			{ return peerid_; }

		protected:
			virtual				~peer() {}

			bool				is_remote_;
			estatus				status_;
			netip4				endpoint_;
			peerid				peerid_;
		};


		class node::node_imp : public xnetio::io_protocol
		{
		public:
								node_imp(iallocator* _allocator, imessage_allocator* _message_allocator);
								~node_imp();

			ipeer*				start(netip4 _endpoint);
			void				stop();

			ipeer*				register_peer(peerid _id, netip4 _endpoint);
			void				unregister_peer(ipeer*);

			void				connect_to(ipeer* _peer);
			void				disconnect_from(ipeer* _peer);

			u32					connections(ipeer** _out_peers, u32 _in_max_peers);

			void				send(outgoing_messages&);

			void				event_wakeup();
			bool				event_loop(incoming_messages*& _rcvd, outgoing_messages*& _sent, u32 _ms_to_wait);

			// io_protocol interface
			virtual xnetio::io_connection open(void* _key, netip4 _ip);
			virtual void		close(xnetio::io_connection);

			virtual bool		needs_write(xnetio::io_connection);
			virtual bool		needs_read(xnetio::io_connection);

			virtual s32			write(xnetio::io_connection, xnetio::io_writer*);
			virtual s32			read(xnetio::io_connection, xnetio::io_reader*);

			XCORE_CLASS_PLACEMENT_NEW_DELETE

			iallocator*			allocator_;
			imessage_allocator*	message_allocator_;
			ipeer_registry*		peer_registry_;
			xnetio::ns_server*	server_;
			incoming_messages	incoming_messages_;
		};

		class peer_connection : public ipeer
		{
		public:
			// peer interface
			virtual bool		is_remote() const			{ return is_remote_; }
			virtual estatus		get_status() const			{ return status_; }
			virtual netip4		get_ip4() const				{ return endpoint_; }
			virtual peerid		get_id() const				{ return peerid_; }

			bool				is_remote_;
			estatus				status_;
			netip4				endpoint_;
			peerid				peerid_;

			outgoing_messages	outgoing_messages_;
			incoming_messages	incoming_messages_;
		};


		node::node_imp::node_imp(iallocator* _allocator, imessage_allocator* _message_allocator)
			: allocator_(_allocator)
			, message_allocator_(_message_allocator)
		{

		}

		node::node_imp::~node_imp()
		{

		}

		ipeer*	node::node_imp::start(netip4 _endpoint)
		{
			peer_registry_ = gCreatePeerRegistry(allocator_);
			//xnetio::ns_server_init()

			return NULL;
		}

		void	node::node_imp::stop()
		{

		}

		ipeer*	node::node_imp::register_peer(peerid _id, netip4 _endpoint)
		{
			return NULL;
		}

		void	node::node_imp::unregister_peer(ipeer* _peer)
		{

		}

		void	node::node_imp::connect_to(ipeer* _peer)
		{

		}

		void	node::node_imp::disconnect_from(ipeer* _peer)
		{

		}

		u32		node::node_imp::connections(ipeer** _out_peers, u32 _in_max_peers)
		{
			return 0;
		}

		void	node::node_imp::send(outgoing_messages& _msg)
		{
			message* m = _msg.dequeue();
			while (m != NULL)
			{
				peer_connection* to = (peer_connection*)(m->get_to());
				to->enqueue(m);
				m = _msg.dequeue();
			}
		}

		void	node::node_imp::event_wakeup()
		{

		}

		bool	node::node_imp::event_loop(incoming_messages*&, outgoing_messages*& _sent, u32 _ms_to_wait)
		{
			return false;
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

		ipeer*				node::register_peer(peerid _id, netip4 _endpoint)
		{
			return imp_->register_peer(_id, _endpoint);
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