#ifndef __XP2P_NETWORK_IO_H__
#define __XP2P_NETWORK_IO_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p/x_types.h"

namespace xcore
{
	namespace xp2p
	{
		class ns_allocator
		{
		public:
			virtual void*	ns_allocate(u32 _size, u32 _alignment) = 0;
			virtual void	ns_deallocate(void* _old) = 0;
		};

	}
}

#endif // __XP2P_NETWORK_IO_H__
