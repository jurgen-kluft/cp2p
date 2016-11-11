#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-address.h"
#include "xp2p\libudx\x_udx-alloc.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-socket.h"
#include "xp2p\libudx\x_udx-udp.h"

#include <chrono>

namespace xcore
{
	void process(udp_socket* _udpsocket, udx_addresses* _addresses, udx_alloc* _allocator)
	{
		// Required:
		// - Allocator to allocate memory for packets to receive
		// - Allocator to allocate new udx sockets

		// Drain the UDP socket for incoming packets
		//  - For every packet add it to the associated udx socket
		//    If the udx socket doesn't exist create it and verify that
		//    the packet is a SYN packet
		udx_packet* pkt = NULL;
		while (_udpsocket->recv(pkt, _allocator, _addresses))
		{

		}

		// For every 'active' udx socket
		//  - update RTT / RTO
		//  - update CC

		// Iterate over all 'active' udx sockets and construct ACK data
		// Iterate over all 'active' udx sockets and send their scheduled packets
		// Iterate over all 'active' udx sockets and collect received packets

	}


	void	udx_socket::process(u64 delta_time_us)
	{

	}


}
