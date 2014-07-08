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
		class IPeer;

		struct MessageType
		{
			inline			MessageType(bool _is_event) : is_event_(_is_event) {}

			inline bool		IsEvent() const						{ return is_event_; }
			inline bool		HasData() const						{ return !is_event_; }

		private:
			bool			is_event_;
		};

		struct MessageEvent
		{
			inline			MessageEvent(bool _is_connected, bool _cannot_connect) : is_connected_(_is_connected), cannot_connect_(_cannot_connect) {}

			inline bool		IsConnected() const					{ return is_connected_; }
			inline bool		IsDisconnected() const				{ return !is_connected_; }

			inline bool		CannotConnect() const				{ return cannot_connect_; }

		private:
			bool			is_connected_;
			bool			cannot_connect_;
		};

		class OutgoingMessage
		{
		public:
			inline		 OutgoingMessage() : mData(0), mCursor(0) {}
			inline		~OutgoingMessage() {}

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

			void		Release();

		protected:
			inline		 OutgoingMessage(const OutgoingMessage&) : mData(0), mCursor(0) {}
			
			void*		mData;
			u32			mCursor;
		};

		class IncomingMessage
		{
		public:
			inline		 IncomingMessage() : mData(0), mCursor(0) {}
			inline		~IncomingMessage() {}

			MessageType	Type() const;
			MessageEvent Event() const;

			bool		IsFrom(IPeer*) const;
			PeerID		From() const;

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

			void		Release();

		protected:
			inline		 IncomingMessage(const IncomingMessage&) : mData(0), mCursor(0) {}

			void*		mData;
			u32			mCursor;
		};
	}
}

#endif	///< __XPEER_2_PEER_MSG_READER_WRITER_H__
