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
		class allocator;
		
		class peer;
		class peer_registry;

		extern peer_registry*		gCreatePeerRegistry(allocator* allocator);

		// "Peer" registry
		class peer_registry
		{
		public:
			virtual void			register_peer(peer*) = 0;
			virtual bool			unregister_peer(peer*) = 0;

			virtual peer*			find_peer_by_ip(netip*) const = 0;

			virtual void			release() = 0;

		protected:
			virtual					~peer_registry() {}
		};		

	}
}

#endif	///< __XPEER_2_PEER_DICTIONARY_H__
