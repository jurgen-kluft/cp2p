#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-address.h"
#include "xp2p\libudx\x_udx-alloc.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-peer.h"
#include "xp2p\libudx\x_udx-udp.h"

#include <chrono>

namespace xcore
{

	void	udx_socket_host::process(u64 delta_time_us)
	{
		// Required:
		//  - Allocator to allocate memory for packets to receive
		//  - Allocator to allocate new udx sockets

		// Drain the UDP socket for incoming packets
		//  - For every packet add it to the associated udx socket
		//    If the udx socket doesn't exist create it and verify that
		//    the packet is a SYN packet
		udx_packet* packet = NULL;
		while (_udpsocket->recv(packet, m_allocator, m_addresses))
		{
			udx_address* address = packet->get_address();
			udx_peer* peer = address->get_peer();
			if (peer == NULL)
			{
				// Create the peer
				peer = m_factory->create_peer(address);
				address->set_peer(peer);
			}
			peer->handle_incoming(packet);
		}

		// For every 'active' udx socket
		//  - update RTT / RTO
		//  - update CC

		// Iterate over all 'active' udx sockets and construct ACK data
		// Iterate over all 'active' udx sockets and send their scheduled packets
		// Iterate over all 'active' udx sockets and collect received packets

	}


}
