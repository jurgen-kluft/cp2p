//==============================================================================
//  x_allocator.h
//==============================================================================
#ifndef __XPEER_2_PEER_ALLOCATOR_H__
#define __XPEER_2_PEER_ALLOCATOR_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

namespace xcore
{
	namespace xp2p
	{
		class IAllocator
		{
		public:
			virtual void*	Alloc(u32 inSize, u32 inAlignment) = 0;
			virtual void	Dealloc(void* inOldMem) = 0;
		};
	}
}

#endif	///< __XPEER_2_PEER_ALLOCATOR_H__
