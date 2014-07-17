//==============================================================================
//  x_p2p.h
//==============================================================================
#ifndef __XPEER_2_PEER_H__
#define __XPEER_2_PEER_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

namespace xcore
{
	// ==============================================================================================================================
	// ==============================================================================================================================
	// ==============================================================================================================================
	namespace xp2p
	{
		class iallocator;
		class imessage_allocator;
		class ipeer;
		class outgoing_messages;
		class incoming_messages;

		class node
		{
		public:
								node(iallocator* _allocator, imessage_allocator* _message_allocator);
								~node();

			ipeer*				start(netip4 endpoint);
			void				stop();

			ipeer*				register_peer(peerid id, netip4 endpoint);
			void				unregister_peer(ipeer*);

			void				connect_to(ipeer* peer);
			void				disconnect_from(ipeer* peer);

			u32					connections(ipeer** _out_peers, u32 _in_max_peers);
			void				send(outgoing_messages&);

			void				event_wakeup();
			bool				event_loop(incoming_messages*& _received, outgoing_messages*& _sent, u32 _ms_to_wait = 0);

		protected:
			iallocator*			allocator_;
			imessage_allocator*	message_allocator_;

			class node_imp;
			node_imp*			imp_;
		};

	}
}

#endif	///< __XPEER_2_PEER_H__
