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
	struct udx_addrin;

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] API
	class udp_socket
	{
	public:
		virtual bool	send(void* pkt, u32 pkt_size, udx_addrin const& addrin) = 0;
		virtual bool	recv(void* pkt, u32& pkt_size, udx_addrin& addrin) = 0;
	};

}

#endif