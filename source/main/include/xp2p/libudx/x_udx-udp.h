//==============================================================================
//  x_udx-udp.h
//==============================================================================
#ifndef __XP2P_UDX_UDP_H__
#define __XP2P_UDX_UDP_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xbase\x_allocator.h"

namespace xcore
{
	class udx_alloc;
	class udx_addresses;
	struct udx_packet;

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] API
	class udp_socket
	{
	public:
		virtual bool	send(udx_packet* pkt, udx_addresses* addresses) = 0;
		virtual bool	recv(udx_packet*& pkt, udx_alloc* packet_allocator, udx_addresses* addresses) = 0;
	};

}

#endif