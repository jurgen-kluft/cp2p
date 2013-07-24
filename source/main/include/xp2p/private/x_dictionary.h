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

	namespace xp2p
	{
		class IAllocator;

		class Dictionary;
		extern Dictionary*			gCreateDictionary(NetPort port, IAllocator* allocator);

		// "Peer ID - Address" Dictionary
		class Dictionary
		{
		public:
			virtual NetAddress		Register(const char* inEndPoint) = 0;
			virtual void			Unregister(NetAddress) = 0;	

		protected:
			virtual					~Dictionary() {}
		};
	}
}

#endif	///< __XPEER_2_PEER_DICTIONARY_H__
