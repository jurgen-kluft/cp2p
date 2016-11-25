//==============================================================================
//  x_udx-rtt.h
//==============================================================================
#ifndef __XP2P_UDX_RTT_H__
#define __XP2P_UDX_RTT_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-packetqueue.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API

	class udx_rtt
	{
	public:
		virtual void on_send(u32 packet_seqnr) = 0;
		virtual void on_receive(u32 ack_segnr, u8* ack_data, u32 ack_data_size) = 0;

		virtual s64 get_rtt_us() const = 0;
		virtual s64 get_rto_us() const = 0;
	};

	udx_rtt*	gCreateDefaultRTT(udx_alloc* _allocator);
}

#endif