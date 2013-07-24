//==============================================================================
//  x_msg.h
//==============================================================================
#ifndef __XPEER_2_PEER_MESSAGE_H__
#define __XPEER_2_PEER_MESSAGE_H__
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
		class IChannel;
		class IPeer;
		class Message;
		class System;

		// P2P - Message Sending
		class SendMessage
		{
		public:
								SendMessage(System* inSystem, IChannel* inChannel, IPeer* inToPeer);

			void				Write(s16 inValue);
			void				Write(s32 inValue);
			void				Write(u16 inValue);
			void				Write(u32 inValue);
			void				WriteStr(const char* inStr);
			void				WriteStr(const char* inStr, s32 inStrLen);

		private:
			void				WriteData(xbyte const* inData, u32 inDataSize);

			System*				mSystem;
			Message*			mMessage;
			IPeer*				mTo;
			u32					mWriteOffset;
		};

		// P2P - Message Receiving
		class ReceiveMessage
		{
		public:
								ReceiveMessage(System* inSystem, IChannel* inChannel);

			enum EEvent
			{
				EVENT_CONNECTING_FAILED		= -100, 
				EVENT_CONNECTED				= 100, 
				EVENT_DISCONNECTED			= -200 
			};

			IPeer*				FromPeer() const;
			bool				IsEvent() const;
			EEvent				GetEvent() const;

			bool				HasData() const;
			void				Read(s16& outValue);
			void				Read(s32& outValue);
			void				Read(u16& outValue);
			void				Read(u32& outValue);
			u32					ReadStr(char* outStr, u32 inOutStrMaxLen);

		private:
			void				ReadData(xbyte* inData, u32 inDataSize);

			System*				mSystem;
			Message*			mMessage;
			IPeer*				mFrom;
			u32					mReadOffset;
		};

	}
}

#endif	///< __XPEER_2_PEER_MESSAGE_H__
