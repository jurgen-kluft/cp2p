//==============================================================================
//  x_udx-time.h
//==============================================================================
#ifndef __XP2P_UDX_TIME_H__
#define __XP2P_UDX_TIME_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_time
	{
	public:
		static u64		get_time_us();
	};

}

#endif