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
		// P2P - Peer
		// This is represents a peer in the network.
		class IPeer
		{
		public:
			enum EStatus { CONNECTING, CONNECTED, DISCONNECTING, DISCONNECTED };

			virtual bool		IsRemote() const = 0;
			virtual EStatus		GetStatus() const = 0;
			virtual NetIP4		GetIP4() const = 0;
			virtual NetPort		GetPort() const = 0;
			virtual const char*	GetStr() const = 0;

		private:
			virtual				~IPeer() {}
		};

	}
}

#endif	///< __XPEER_2_PEER_PEER_H__
