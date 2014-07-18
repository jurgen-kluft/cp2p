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
		struct io_buffer
		{
			xbyte*		data;
			u32			size;
		};

		class io_reader
		{
		public:
			virtual s32				read(io_buffer& buf) = 0;
		};

		class io_writer
		{
		public:
			virtual s32				write(io_buffer& buf) = 0;
		};

		typedef void*				io_connection;

		class io_protocol
		{
		public:
			virtual io_connection	io_open(void*, xp2p::netip4) = 0;
			virtual void			io_close(io_connection) = 0;

			virtual bool			io_needs_write(io_connection) = 0;
			virtual bool			io_needs_read(io_connection) = 0;
			
			virtual s32				io_write(io_connection, io_writer*) = 0;
			virtual s32				io_read(io_connection, io_reader*) = 0;
		};
	}
}

#endif	///< __XPEER_2_PEER_NETWORK_PROTOCOL_PRIVATE_H__
