//==============================================================================
//  x_host.h
//==============================================================================
#ifndef __XPEER_2_PEER_HOST_H__
#define __XPEER_2_PEER_HOST_H__
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
		class IPeer;
		class IChannel;

		// API for Host
		class IHost
		{
		public:
			virtual PeerID		GetId() const = 0;

		protected:
			virtual				~IHost() {}
		};


	}
}

#endif	///< __XPEER_2_PEER_HOST_H__