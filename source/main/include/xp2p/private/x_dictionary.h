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
	// Forward declarations
	class x_iallocator;

	namespace xp2p
	{
		class Dictionary;
		extern Dictionary*			gCreateDictionary(NetPort port, x_iallocator* allocator);

		// "Peer ID - Address" Dictionary
		class Dictionary
		{
		public:
			virtual PeerID			RegisterHost(NetPort port) = 0;
			virtual NetAddress		RegisterAddress(const char* address_str) = 0;
			virtual void			RegisterPeer(PeerID, NetAddress) = 0;

			virtual NetAddress		FindAddressOfPeer(PeerID) const = 0;
			virtual PeerID			FindPeerByAddress(NetAddress) const = 0;

			virtual void			UnregisterPeer(PeerID) = 0;	

		protected:
			virtual					~Dictionary() {}
		};
	}
}

#endif	///< __XPEER_2_PEER_DICTIONARY_H__
