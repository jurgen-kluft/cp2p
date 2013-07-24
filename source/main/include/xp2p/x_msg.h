//==============================================================================
//  x_msg.h
//==============================================================================
#ifndef __XPEER_2_PEER_MSG_READER_WRITER_H__
#define __XPEER_2_PEER_MSG_READER_WRITER_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

namespace xcore
{
	// ==============================================================================================================================
	// ==============================================================================================================================
	// ==============================================================================================================================
	namespace xp2p
	{
		class Message
		{
		public:
			enum EType
			{
				DATA  = 1,
				EVENT = 2,
			};

			enum EEvent
			{
				CONNECTED				= 100, 
				CONNECTION_FAILED		= -100, 
				DISCONNECTED			= -200 
			};
		};

		class TxMessage
		{
		public:
			u32			CurrentSize() const;
			u32			MaximumSize() const;
			bool		CanWrite(u32 numberOfBytes) const;		// Check if we still can write N number of bytes

			void		Write(bool);
			void		Write(u8 );
			void		Write(s8 );
			void		Write(u16);
			void		Write(s16);
			void		Write(u32);
			void		Write(s32);
			void		Write(u64);
			void		Write(s64);
			void		Write(f32);
			void		Write(f64);
			void		Write(const char*);

		protected:
			inline		TxMessage() : mData(0), mWrittenSize(0), mMaximumSize(0) {}
			inline		TxMessage(const TxMessage&) : mData(0), mWrittenSize(0), mMaximumSize(0) {}
			inline		~TxMessage() {}

			void*		mData;
			u32			mWrittenSize;
			u32			mMaximumSize;
		};

		class RxMessage
		{
		public:
			bool		IsType(Message::EType) const;
			bool		IsEvent(Message::EEvent) const;

			PeerID		FromPeerID() const;

			u32			ReadSize() const;
			u32			TotalSize() const;
			bool		CanRead(u32 numberOfBytes) const;		// Check if we still can read N number of bytes

			bool		Read(bool&) const;
			bool		Read(u8 &) const;
			bool		Read(s8 &) const;
			bool		Read(u16&) const;
			bool		Read(s16&) const;
			bool		Read(u32&) const;
			bool		Read(s32&) const;
			bool		Read(u64&) const;
			bool		Read(s64&) const;
			bool		Read(f32&) const;
			bool		Read(f64&) const;

			bool		ReadStr(const char* str, u32 maxstrlen, u32& strlen);

		protected:
			inline		RxMessage() : mData(0), mReadSize(0), mTotalSize(0) {}
			inline		RxMessage(const RxMessage&) : mData(0), mReadSize(0), mTotalSize(0) {}
			inline		~RxMessage() {}

			void*		mData;
			u32			mReadSize;
			u32			mTotalSize;
		};
	}
}

#endif	///< __XPEER_2_PEER_MSG_READER_WRITER_H__
