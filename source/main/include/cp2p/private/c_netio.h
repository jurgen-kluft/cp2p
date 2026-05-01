#ifndef __CP2P_NETWORK_IO_H__
#define __CP2P_NETWORK_IO_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "cp2p/c_types.h"

namespace ncore
{
    namespace np2p
    {
        class ns_allocator
        {
        public:
            virtual void* ns_allocate(u32 _size, u32 _alignment) = 0;
            virtual void  ns_deallocate(void* _old)              = 0;
        };

    }  // namespace np2p
}  // namespace ncore

#endif  // __CP2P_NETWORK_IO_H__
