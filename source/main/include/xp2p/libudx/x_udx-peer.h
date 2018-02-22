//==============================================================================
//  x_udx-peer.h
//==============================================================================
#ifndef __XP2P_UDX_PEER_H__
#define __XP2P_UDX_PEER_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
	class udx_alloc;
	class udx_address;
	class udx_packet;
	class udx_packet_reader;
	class udx_packet_writer;

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_peer
	{
	public:
		virtual udx_address*	get_address() const = 0;

		virtual bool 			connect() = 0;
		virtual bool 			disconnect() = 0;
		virtual bool 			is_connected() const = 0;

		virtual void 			send(udx_packet*) = 0;
		virtual void 			received(udx_packet*) = 0;

		virtual void			process() = 0;

		virtual void			writeto_socket(udx_packet_writer* packet_to_socket_writer) = 0;
		virtual void			collect_garbage(udx_packet_writer* garbage_collector) = 0;
		virtual void			collect_incoming(udx_packet_writer* incoming_collector) = 0;
	};

	udx_peer*	gCreateUdxPeer(udx_address* address, udx_alloc* allocator);
}

#endif
