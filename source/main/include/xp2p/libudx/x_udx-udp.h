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
	struct udx_packet;

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] API
	class udp_socket
	{
	public:
		virtual void	send(udx_packet* pkt) = 0;
		virtual void	recv(udx_packet*& pkt) = 0;
	};

}

#endif