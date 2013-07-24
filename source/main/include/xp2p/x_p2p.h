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
		class IPeer;
		class IChannel;
		class IDelegate;
		class IMessage;
		class IAllocator;
	
		/**
		\brief The P2P System
		**/
		class System
		{
		public:
								System();

			IPeer*				Start(NetPort inHostPort, IAllocator* inSystemAllocator);
			void				Stop();

			// Peers
			IPeer*				ConnectTo(const char* inEndpoint);
			void				DisconnectFrom(IPeer*);

			// Channels
			IChannel*			RegisterChannel(const char* inChannelName, IAllocator* inMsgAllocator, IDelegate* inChannelReceiveListener); 

			// Messages
			IMessage*			CreateMessage(IChannel* inChannel, IPeer* inTo, u32 inMaxMsgSizeInBytes);
			void				SendMessage(IMessage* inMsg);
			IMessage*			ReceiveMessage(IChannel* inChannel);
			void				DestroyMessage(IMessage* inMsg);

			// Notifications

		protected:
			class Implementation;
			Implementation*		mImplementation;
		};


		/**
		\brief Example on how to use P2P

		

		\code
		\endcode
		**/

	}
}

#endif	///< __XPEER_2_PEER_H__
