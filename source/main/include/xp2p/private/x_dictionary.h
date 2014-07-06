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
		class IAllocator;
		class Dictionary;
		extern Dictionary*			gCreateDictionary(IAllocator* allocator);

		// "Peer <-> PeerID" Dictionary
		class Dictionary
		{
		public:
			virtual IPeer*			RegisterPeer(PeerID, NetIP4) = 0;
			virtual IPeer*			FindPeerByID(PeerID) const = 0;

			virtual bool			UnregisterPeer(PeerID) = 0;

		protected:
			virtual					~Dictionary() {}
		};		

	}
}

#endif	///< __XPEER_2_PEER_DICTIONARY_H__
