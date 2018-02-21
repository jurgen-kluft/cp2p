#include "xbase/x_target.h"
#include "xp2p/x_sha1.h"
#include "xp2p/libudx/x_udx.h"
#include "xp2p/libudx/x_udx-packet.h"
#include "xp2p/libudx/x_udx-packetqueue.h"
#include "xp2p/libudx/x_udx-bitstream.h"
#include "xp2p/private/x_sockets.h"

#include <chrono>

namespace xcore
{
	/*
	uTP:
		States:
			- Starting Phase
				- Ends when Utility(t) < Utility(t-1)
			- Moving Phase
				- Guessing

	*/





}
