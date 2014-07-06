//==============================================================================
//  x_p2p.h
//==============================================================================
#ifndef __XPEER_2_PEER_H__
#define __XPEER_2_PEER_H__
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
		class IAllocator;
		class IPeer;
		class OutgoingMessage;
		class IncomingMessage;

		class P2P
		{
		public:
								P2P(IAllocator* memory_allocator);
								~P2P();

			IPeer*				Start(NetIP4 localhost);
			void				Stop();

			IPeer*				RegisterPeer(PeerID id, NetIP4 endpoint);
			void				UnregisterPeer(IPeer*);

			void				ConnectTo(IPeer* peer);
			void				DisconnectFrom(IPeer* peer);

			u32					Connections(IPeer** outPeerList, u32 sizePeerList);

			bool				CreateMsg(OutgoingMessage&, IPeer* to, u32 size);
			void				SendMsg(OutgoingMessage&);

			bool				ReceiveMsg(IncomingMessage&, u32 _ms_to_wait=0);

		protected:
			IAllocator*			mAllocator;
			class Implementation;
			Implementation*		mImplementation;
		};

	}
}

#endif	///< __XPEER_2_PEER_H__
