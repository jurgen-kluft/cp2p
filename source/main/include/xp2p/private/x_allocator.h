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
		class iallocator
		{
		public:
			virtual void*	allocate(u32 _size, u32 _alignment) = 0;
			virtual void	deallocate(void* _in_oldmem) = 0;
		};
	}
}

#endif	///< __XPEER_2_PEER_ALLOCATOR_H__
