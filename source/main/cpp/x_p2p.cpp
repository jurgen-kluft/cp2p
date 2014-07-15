#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "xp2p\x_p2p.h"
#include "xp2p\x_peer.h"
#include "xp2p\private\x_allocator.h"
#include "xp2p\private\x_peer_registry.h"

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


		class node::node_imp
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

			void				send(outgoing_message&);

			void				event_wakeup();
			bool				event_loop(incoming_messages*&, u32 _ms_to_wait);

			XCORE_CLASS_PLACEMENT_NEW_DELETE

			iallocator*			allocator_;
			imessage_allocator*	message_allocator_;
			ipeer_registry*		peer_registry_;
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

		void	node::node_imp::send(outgoing_message& _msg)
		{

		}

		void	node::node_imp::event_wakeup()
		{

		}

		bool	node::node_imp::event_loop(incoming_messages*&, u32 _ms_to_wait)
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

		void				node::send(outgoing_message& _msg)
		{
			imp_->send(_msg);
		}

		void				node::event_wakeup()
		{
			imp_->event_wakeup();
		}

		bool				node::event_loop(incoming_messages*& _msgs, outgoing_messages*& _sent, u32 _wait_in_ms)
		{
			return imp_->event_loop(_msgs, _wait_in_ms);
		}
	}
}