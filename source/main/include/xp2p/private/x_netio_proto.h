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
		class io_reader
		{
		public:
			/// returns:
			///   >= 0, number of bytes read
			///   <  0, error
			virtual s32				read(xbyte* _buffer, u32 _size) = 0;
		};

		class io_writer
		{
		public:
			/// returns:
			///   >= 0, number of bytes written
			///   <  0, error
			virtual s32				write(xbyte const* _buffer, u32 _size) = 0;
		};

		typedef void*				io_connection;

		class io_protocol
		{
		public:
			virtual io_connection	io_open(xp2p::netip4) = 0;
			virtual void			io_close(io_connection) = 0;

			virtual bool			io_needs_write(io_connection) = 0;
			virtual bool			io_needs_read(io_connection) = 0;
			
			virtual s32				io_write(io_connection, io_writer*) = 0;
			virtual s32				io_read(io_connection, io_reader*) = 0;

			// Events. Meaning of event parameter (evp) is given in the comment.
			enum event 
			{
				EVENT_POLL,     // Sent to each connection on each call to ns_server_poll()
				EVENT_ACCEPT,   // New connection accept()-ed. socket_address * remote_addr
				EVENT_CONNECT,  // Connect() succeeded or failed. int *success_status
				EVENT_CLOSE     // Connection is closed. NULL
			};

			virtual void			io_callback(io_connection, event, void *evp) = 0;
		};
	}
}

#endif	///< __XPEER_2_PEER_NETWORK_PROTOCOL_PRIVATE_H__
