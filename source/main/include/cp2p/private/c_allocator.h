#ifndef __CPEER_2_PEER_ALLOCATOR_H__
#define __CPEER_2_PEER_ALLOCATOR_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "cp2p/c_types.h"

namespace ncore
{
	namespace np2p
	{
		class allocator
		{
		public:
			virtual void*	allocate(u32 _size, u32 _alignment) = 0;
			virtual void	deallocate(void* _in_oldmem) = 0;
		};
	}
}

#endif	// __CPEER_2_PEER_ALLOCATOR_H__
