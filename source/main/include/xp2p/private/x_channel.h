//==============================================================================
//  x_channel.h
//==============================================================================
#ifndef __XPEER_2_PEER_CHANNEL_H__
#define __XPEER_2_PEER_CHANNEL_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

namespace xcore
{
	// Forward declarations
	class x_iallocator;

	namespace xp2p
	{
		class IPeer;

		typedef void*				WriteHandle;
		typedef void*				ReadHandle;

		class Channels;
		extern Channels*			gCreateChannels(u32 max_num_channels, x_iallocator* allocator);

		//
		class IDelegate
		{
		public:
			virtual void			Handle() = 0;
		};

		// API for Channels
		class Channels
		{
		public:
			virtual IChannel*		Create(const char* inChannelName, IDelegate* inOnReceiveDelegate) = 0;
			virtual void			Destroy(IChannel*) = 0;

		private:
			virtual 				~Channels() {}
		};

		// P2P - Message Channel (single threaded access)
		class IChannel
		{
		public:
			virtual const char*		Name() const = 0;

			// Send
			virtual WriteHandle		CreateMsg(IPeer* to, u32 size) = 0;
			virtual void			QueueMsg(WriteHandle hnd) = 0;

			// Receive
			virtual ReadHandle		ReadMsg() = 0;
			virtual void			CloseMsg(ReadHandle hnd) = 0;

		protected:
			virtual					~IChannel() {}
		};

		// 
		// We can create different kind of channels:
		// 
		// 1. A channel where it is allocating directly from the system using malloc() 
		//    and closing a message will directly call free().
		//
		// 2. A channel where it uses a specialized allocator that uses a pre-allocated 
		//    chunk of memory.
		//
		// 3. A channel where it holds a lockless queue with a finite amount of pointers 
		//    to fixed size chunks (512 bytes or so). Here a message can never use more 
		//    than 512 bytes, but even a 8 byte message will occupy 512 bytes in memory.
		//
		// Blocking IO:
		//    The channel could block the calling thread when there are no messages.
		//    The channel could block when the channel has no memory left for inserting
		//    another message, the calling thread has to wait until messages are being closed
		//    by the network system sending and closing the messages.
		// 

	}
}

#endif	///< __XPEER_2_PEER_CHANNEL_H__
