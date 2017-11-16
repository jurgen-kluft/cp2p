//=======================================================================
//  x_udx-perfmon.h
//=======================================================================
#ifndef __XP2P_UDX_PERFMON_H__
#define __XP2P_UDX_PERFMON_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p/libudx/x_udx-seqnr.h"

namespace xcore
{
	class udx_alloc;

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API

	class udx_cc
	{
	public:
		virtual void	reset() = 0;

		virtual bool	on_send(double _time_s, u32 pkt_size, udx_seqnr pkt_segnr) = 0;
		virtual bool	on_recv(double _time_s, u64 pkt_rtt_us, udx_seqnr pkt_segnr) = 0;
		virtual bool	on_loss(double _time_s, u32 pkt_size, udx_seqnr pkt_segnr) = 0;

		virtual s64		get_loss_rate() const = 0;
		virtual s64		get_total_rate() const = 0;

		virtual bool	can_send(u32 num_bytes) = 0;
	};

	udx_cc*		gCreateDefaultCC(udx_alloc* _allocator);
}

#endif