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
	namespace xnetio
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

		class io_protocol
		{
		public:
			typedef	void*			connection_t;

			virtual connection_t	open(void*) = 0;
			virtual void			close(connection_t) = 0;

			virtual bool			needs_write(connection_t) = 0;
			virtual bool			needs_read(connection_t) = 0;
			
			virtual s32				write(connection_t, io_writer*) = 0;
			virtual s32				read(connection_t, io_reader*) = 0;
		};
	}
}

#endif	///< __XPEER_2_PEER_NETWORK_PROTOCOL_PRIVATE_H__
