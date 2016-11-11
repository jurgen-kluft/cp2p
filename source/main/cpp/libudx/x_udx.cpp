#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\private\x_sockets.h"

#include <chrono>

namespace xcore
{

	static void process()
	{
		// Required:
		// - Allocator to allocate memory for packets to receive

		// Drain the UDP socket for incoming packets
		//  - For every packet add it to the associated udx socket
		//    If the udx socket doesn't exist create it and verify that
		//    the packet is a SYN packet
		// For every 'active' udx socket
		//  - update RTT / RTO
		//  - update CC
		// Iterate over all 'active' udx sockets and construct ACK data
		// Iterate over all 'active' udx sockets and send their scheduled packets
		// Iterate over all 'active' udx sockets and collect received packets

	}


}
