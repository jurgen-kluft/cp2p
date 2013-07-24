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
	
		// P2P - Host
		// This is the local Peer called Host. Using the Host you can connect to and 
		// disconnect from remote Peers. Using the created IChannel you can send and 
		// receive messages.
		// Note: You will work with remote peers using IPeer*.
		class IHost
		{
		public:
			virtual void		AddChannel(IChannel* channel) = 0;
			
			virtual void		Start() = 0;
			virtual void		Stop() = 0;

			virtual IPeer*		ConnectTo(NetIP4 ip, NetPort port) = 0;
			virtual u32			NumConnections() const = 0;
			virtual void		GetConnections(IPeer** outPeerList, u32 sizePeerList, u32& outPeerCnt) = 0;
			virtual void		DisconnectFrom(IPeer* peer) = 0;

		protected:
			virtual				~IHost() {}
		};

	}
}

#endif	///< __XPEER_2_PEER_HOST_H__
