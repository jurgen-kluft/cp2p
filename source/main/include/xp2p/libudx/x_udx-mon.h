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
	class udx_bitstream;

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API

	class udx_monitor
	{
	public:
		virtual void	reset() = 0;

		virtual bool	on_send(double _time_s, udx_seqnr pkt_segnr) = 0;
		virtual bool	on_recv(double _time_s, udx_seqnr ack_segnr) = 0;
		virtual bool	on_loss(double _time_s, udx_seqnr ack_segnr) = 0;

		virtual s64		get_loss_rate() const = 0;
		virtual s64		get_total_rate() const = 0;
	};

	class udx_perf_monitor
	{
	public:
		virtual bool	on_send(double _time_s, udx_seqnr pkt_segnr) = 0;
		virtual bool	on_recv(double _time_s, udx_seqnr ack_segnr) = 0;
		virtual bool	on_loss(double _time_s, udx_seqnr ack_segnr) = 0;

		virtual s64		get_rate() const = 0;
	};

	udx_perf_monitor*	gCreateDefaultPerformanceMonitor(udx_alloc* _allocator);
}

#endif