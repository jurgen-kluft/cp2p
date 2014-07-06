//==============================================================================
//  x_peer.h
//==============================================================================
#ifndef __XPEER_2_PEER_PEER_H__
#define __XPEER_2_PEER_PEER_H__
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
		// P2P - Remote Peer
		class IPeer
		{
		public:	

			virtual PeerID		GetID() const = 0;

		protected:
			virtual				~IPeer() {}
		};

	}
}

#endif	///< __XPEER_2_PEER_PEER_H__