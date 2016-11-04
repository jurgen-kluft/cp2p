#include "xbase\x_target.h"
#include "xp2p\libudx\x_udx.h"

#include <chrono>

namespace xcore
{
	u64 udx_time::get_time_us()
	{
		std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
		std::chrono::nanoseconds ns = time.time_since_epoch;
		return (u64)ns.count();
	}

}
