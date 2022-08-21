//==============================================================================
//  x_udx-alloc.h
//==============================================================================
#ifndef __XP2P_UDX_ALLOC_H__
#define __XP2P_UDX_ALLOC_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xbase/x_allocator.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_alloc
	{
	public:
		virtual void*		alloc(u32 _size) = 0;
		virtual void		commit(void*, u32 _size) = 0;
		virtual void		dealloc(void*) = 0;
	};

}

#endif