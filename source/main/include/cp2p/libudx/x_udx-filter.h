//==============================================================================
//  x_udx-filter.h
//==============================================================================
#ifndef __XP2P_UDX_FILTER_H__
#define __XP2P_UDX_FILTER_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p/libudx/x_udx-alloc.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_filter
	{
	public:
		virtual u64		add(u64 value) = 0;
		virtual u64		get() const = 0;

		virtual void	release() = 0;
	};

	udx_filter*		gCreateSmaFilter(u32 wnd_size, udx_alloc* _allocator);
}

#endif