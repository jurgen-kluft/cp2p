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
		class ipeer;
		class outgoing_message;
		class incoming_messages;

		class node
		{
		public:
								node(iallocator* memory_allocator);
								~node();

			ipeer*				start(netip4 localhost);
			void				stop();

			ipeer*				register_peer(peerid id, netip4 endpoint);
			void				unregister_peer(ipeer*);

			void				connect_to(ipeer* peer);
			void				disconnect_from(ipeer* peer);

			u32					connections(ipeer** _out_peers, u32 _in_max_peers);

			bool				send(outgoing_message*);

			void				event_wakeup();
			bool				event_loop(incoming_messages&, u32 _ms_to_wait = 0);

		protected:
			iallocator*			allocator_;
			class node_imp*		imp_;
		};

	}
}

#endif	///< __XPEER_2_PEER_H__
