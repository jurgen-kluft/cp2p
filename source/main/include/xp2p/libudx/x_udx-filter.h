//==============================================================================
//  x_udx-filter.h
//==============================================================================
#ifndef __XP2P_UDX_FILTER_H__
#define __XP2P_UDX_FILTER_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif


namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_filter
	{
	public:
		virtual void	init(u64* window, u32 size);

		virtual u64		add(u64 value) = 0;
		virtual u64		get() const = 0;
	};



}

#endif