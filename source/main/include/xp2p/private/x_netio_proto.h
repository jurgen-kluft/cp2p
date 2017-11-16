//==============================================================================
//  x_netio_proto.h (private)
//==============================================================================
#ifndef __XPEER_2_PEER_NETWORK_PROTOCOL_PRIVATE_H__
#define __XPEER_2_PEER_NETWORK_PROTOCOL_PRIVATE_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

namespace xcore
{
	namespace xp2p
	{
		typedef		void*		io_connection;

		class io_writer
		{
		public:
			virtual s32				write(xbyte const* _buffer, u32 _size) = 0;
		};

		class io_reader
		{
		public:
			virtual s32				read(xbyte* _buffer, u32 _size) = 0;
		};

		class io_protocol
		{
		public:
			virtual void*	io_open(netip) = 0;
			virtual void	io_close(void* io_connection) = 0;

			enum EEvent
			{
				EVENT_POLL = 1,
				EVENT_CLOSE = 2,
				EVENT_ACCEPT = 3,
				EVENT_CONNECT = 4,
			};

			
			virtual void	io_callback(io_connection connection, EEvent, void* param);

			virtual bool	io_needs_read(io_connection c) = 0;
			virtual bool	io_needs_write(io_connection c) = 0;

			virtual s32		io_read(io_connection c, io_reader* reader) = 0;
			virtual s32		io_write(io_connection c, io_writer* writer) = 0;
		};
	}
}

#endif	///< __XPEER_2_PEER_NETWORK_PROTOCOL_PRIVATE_H__
