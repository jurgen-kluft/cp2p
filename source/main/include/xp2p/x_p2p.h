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
	// ==============================================================================================================================
	namespace xp2p
	{
		class node;
		class ipeer;
		class iallocator;
		class imessage_allocator;
		class outgoing_messages;
		class incoming_messages;
		class gc_messages;

		class inode
		{
		public:
			virtual ipeer*		start(netip4 endpoint, iallocator* _allocator, imessage_allocator* _message_allocator) = 0;
			virtual void		stop() = 0;

			virtual ipeer*		register_peer(netip4 endpoint) = 0;
			virtual void		unregister_peer(ipeer*) = 0;

			virtual void		connect_to(ipeer* peer) = 0;
			virtual void		disconnect_from(ipeer* peer) = 0;

			virtual u32			connections(ipeer** _out_peers, u32 _in_max_peers) = 0;
			virtual void		send(outgoing_messages&) = 0;

			virtual void		event_wakeup() = 0;
			virtual bool		event_loop(incoming_messages*& _received, gc_messages*& _sent, u32 _ms_to_wait) = 0;
		};


		inode*	gCreateNode(iallocator* _allocator);


	}
}

#endif	///< __XPEER_2_PEER_H__
