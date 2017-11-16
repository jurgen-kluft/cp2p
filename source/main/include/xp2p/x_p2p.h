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
	// Peer to Peer node
	// Goals:
	// - Zer0 memory copy messaging
	// - Peers are objects
	// - Event loop
	// - Message size <= 1400 bytes
	// ==============================================================================================================================
	namespace xp2p
	{
		class node;
		class peer;
		class allocator;
		class message_allocator;
		class outgoing_messages;
		class incoming_messages;
		class garbagec_messages;

		class node
		{
		public:
			virtual peer*		start(netip endpoint, allocator* _allocator, message_allocator* _message_allocator) = 0;
			virtual void		stop() = 0;

			virtual peer*		register_peer(netip endpoint) = 0;
			virtual void		unregister_peer(peer*) = 0;

			virtual void		connect_to(peer* peer) = 0;
			virtual void		disconnect_from(peer* peer) = 0;

			virtual u32			connections(peer** _out_peers, u32 _in_max_peers) = 0;
			virtual void		send(outgoing_messages&) = 0;

			virtual void		event_wakeup() = 0;
			virtual bool		event_loop(incoming_messages*& _received, garbagec_messages*& _sent, u32 _ms_to_wait) = 0;
		};


		node*	gCreateNode(allocator* _allocator);


	}
}

#endif	///< __XPEER_2_PEER_H__
