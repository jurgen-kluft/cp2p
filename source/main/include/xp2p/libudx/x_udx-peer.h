//==============================================================================
//  x_udx-peer.h
//==============================================================================
#ifndef __XP2P_UDX_PEER_H__
#define __XP2P_UDX_PEER_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
	class udx_alloc;
	class udx_address;
	class udx_packet;
	class udx_packetlqueue;
	class udx_packetsqueue;

	struct udx_msg;

	struct udx_peer_queues
	{
		udx_packetlqueue* outgoing;
		udx_packetsqueue* incoming;
		udx_packetlqueue* send;
		udx_packetsqueue* wack;
		udx_packetlqueue* garbage;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_peer_processor
	{
	public:
		virtual void			process() = 0;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_peer
	{
	public:
		virtual udx_address*	get_address() const = 0;

		virtual bool 			connect() = 0;
		virtual bool 			disconnect() = 0;
		virtual bool 			is_connected() const = 0;

		virtual void 			push_incoming(udx_packet*) = 0;
		virtual void 			push_outgoing(udx_packet*) = 0;

		virtual bool			pop_incoming(udx_packet*&) = 0;
		virtual bool 			pop_outgoing(udx_packet*&) = 0;

		virtual void			process(u64 delta_time_us, udx_packet_writer* packet_writer) = 0;
	};

	udx_peer*	gCreateUdxPeer(udx_address* address, udx_alloc* allocator);
}

#endif
