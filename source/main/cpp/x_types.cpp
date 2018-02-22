#include "xbase/x_target.h"
#include "xbase/x_allocator.h"
#include "xbase/x_string_ascii.h"

#include "xp2p/x_p2p.h"
#include "xp2p/x_types.h"

namespace xcore
{
	namespace xp2p
	{
		void		netip4::to_string(char* s, u32 l) const
		{
			SPrintf(s, l, "%u.%u.%u.%u:%u", x_va(ip_.aip_[0]), x_va(ip_.aip_[1]), x_va(ip_.aip_[2]), x_va(ip_.aip_[3]), x_va(port_));
		}

	}
}