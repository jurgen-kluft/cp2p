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
		// This represents a peer in the network.
		class ipeer
		{
		public:
			enum estatus { CONNECTING, CONNECTED, DISCONNECTING, DISCONNECTED };

			virtual bool		is_remote() const = 0;
			virtual estatus		get_status() const = 0;
			virtual netip4		get_ip4() const = 0;
			virtual peerid		get_id() const = 0;

		private:
			virtual				~ipeer() {}
		};

	}
}

#endif	///< __XPEER_2_PEER_PEER_H__
