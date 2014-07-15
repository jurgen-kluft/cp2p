//==============================================================================
//  x_dictionary.h
//==============================================================================
#ifndef __XPEER_2_PEER_DICTIONARY_H__
#define __XPEER_2_PEER_DICTIONARY_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

namespace xcore
{
	namespace xp2p
	{
		class iallocator;
		
		class ipeer;
		class ipeer_registry;

		extern ipeer_registry*		gCreatePeerRegistry(iallocator* allocator);

		// "Peer <-> PeerID" Dictionary
		class ipeer_registry
		{
		public:
			virtual ipeer*			register_peer(peerid, netip4) = 0;
			virtual bool			unregister_peer(peerid) = 0;

			virtual ipeer*			find_peer_by_id(peerid) const = 0;

			virtual void			release() = 0;

		protected:
			virtual					~ipeer_registry() {}
		};		

	}
}

#endif	///< __XPEER_2_PEER_DICTIONARY_H__
