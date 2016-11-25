//==============================================================================
//  x_udx-socket.h
//==============================================================================
#ifndef __XP2P_UDX_SOCKET_H__
#define __XP2P_UDX_SOCKET_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-packetqueue.h"

namespace xcore
{
	class udx_address;
	struct udx_msg;

	class udx_msg_handler
	{
	public:
		virtual void			handle_msg(udx_msg& msg) = 0;
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

		virtual void			process(u64 delta_time_us) = 0;
	};

	udx_peer*	gCreateUdxPeer(udx_address* address, udx_alloc* allocator);

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_host
	{
	public:
		virtual udx_address*	get_address() const = 0;

		virtual bool			alloc_msg(u32 size, udx_msg& msg) = 0;
		virtual void			free_msg(udx_msg& msg) = 0;

		virtual udx_address*	connect(const char* address) = 0;
		virtual bool			disconnect(udx_address*) = 0;
		virtual bool 			is_connected(udx_address*) const = 0;

		virtual bool			send(udx_msg& msg, udx_address* to) = 0;
		virtual bool			recv(udx_msg& msg, udx_address*& from) = 0;

		virtual void			process(u64 delta_time_us) = 0;
	};

	udx_host*			gCreateUdxHost(const char* local_address, udx_alloc* allocator);
}

#endif
