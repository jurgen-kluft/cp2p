//=======================================================================
//  x_udx-perfmon.h
//=======================================================================
#ifndef __XP2P_UDX_PERFMON_H__
#define __XP2P_UDX_PERFMON_H__
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

	class udx_perfmon
	{
	public:
		virtual void	on_snd(u64 time, udx_seqnr segnr_base, udx_bitstream const& pkts) = 0;
		virtual void	on_rcv(u64 time, udx_seqnr segnr_base, udx_bitstream const& acks) = 0;

		virtual s64		get_loss() const = 0;
		virtual s64		get_total() const = 0;
	};

	udx_perfmon*	gCreateDefaultPerfMon(udx_alloc* _allocator);
}

#endif