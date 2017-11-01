//==============================================================================
//  x_udx-rto_timer.h
//==============================================================================
#ifndef __XP2P_UDX_RTO_TIMER_H__
#define __XP2P_UDX_RTO_TIMER_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p/libudx/x_udx-rtt.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_rto_timer
	{
	public:
		virtual void	start() = 0;
		virtual void	stop() = 0;

		virtual bool	is_timeout(u64 current_time_us, u64 time_send_us) = 0;
		virtual bool	on_timeout() = 0;
	};

	udx_rto_timer*	gCreateDefaultRTOTimer(udx_alloc* _allocator, udx_rtt* _rtt_computer);
}

#endif	// __XP2P_UDX_RTO_TIMER_H__