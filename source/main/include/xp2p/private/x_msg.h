//==============================================================================
//  x_msg.h (private)
//==============================================================================
#ifndef __XPEER_2_PEER_MSG_PRIVATE_H__
#define __XPEER_2_PEER_MSG_PRIVATE_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

namespace xcore
{
	namespace xp2p
	{
		// P2P - Message (private)
		// This represents a handle to a to-sent or received message.
		class Message
		{
		public:
			virtual u32			Read(u32 inOffset, xbyte* inData, u32 inDataSize);
			virtual u32			Write(u32 inOffset, xbyte const* inData, u32 inDataSize);

		private:
			virtual				~Message() {}
		};

	}
}

#endif	///< __XPEER_2_PEER_MSG_PRIVATE_H__
