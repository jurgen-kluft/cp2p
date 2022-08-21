//==============================================================================
//  x_udx-rtt.h
//==============================================================================
#ifndef __XP2P_UDX_RTT_H__
#define __XP2P_UDX_RTT_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p/libudx/x_udx-alloc.h"
#include "xp2p/libudx/x_udx-packet.h"
#include "xp2p/libudx/x_udx-packetqueue.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API

	class udx_rtt
	{
	public:
		virtual void	reset() = 0;

		virtual void	on_ack(udx_packet const* p) = 0;

		virtual s64		get_rtt_us() const = 0;
		virtual s64		get_rto_us() const = 0;
	};

	udx_rtt*	gCreateDefaultRTT(udx_alloc* _allocator);
}

#endif