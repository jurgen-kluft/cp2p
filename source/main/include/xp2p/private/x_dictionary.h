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
	class MemoryAllocator;

	namespace xp2p
	{
		class Dictionary;
		extern Dictionary*			gCreateDictionary(MemoryAllocator* allocator);


		// "Peer ID - Address" Dictionary
		class Dictionary
		{
		public:
			virtual NetAddress		RegisterAddress(const char* address_str) = 0;
			virtual void			RegisterPeerID(PeerID, NetAddress) = 0;

			virtual NetAddress		FindAddressByPeerID(PeerID) const = 0;
			virtual PeerID			FindPeerIDByAddress(NetAddress) const = 0;

			virtual void			UnregisterPeer(PeerID) = 0;			// Will also unregister the address

		protected:
			virtual					~Dictionary() {}
		};		

	}
}

#endif	///< __XPEER_2_PEER_DICTIONARY_H__
