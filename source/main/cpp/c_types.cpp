#include "ccore/c_target.h"
#include "ccore/c_allocator.h"
#include "ccore/c_printf.h"

#include "cp2p/c_p2p.h"
#include "cp2p/c_types.h"

namespace ncore
{
    namespace np2p
    {
        void netip::to_string(char* s, u32 l) const { snprintf(s, l, "%u.%u.%u.%u:%u", ip_[0], ip_[1], ip_[2], ip_[3], port_); }

    }  // namespace np2p
}  // namespace ncore
