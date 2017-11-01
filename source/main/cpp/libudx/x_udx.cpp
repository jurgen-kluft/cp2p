#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-address.h"
#include "xp2p\libudx\x_udx-alloc.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-peer.h"
#include "xp2p\libudx\x_udx-registry.h"
#include "xp2p\libudx\x_udx-udp.h"

#include <chrono>

namespace xcore
{
	class udx_peer_db
	{
	public:
		virtual bool		find(udx_address*, udx_peer*& peer, u32& peer_idx) = 0;
		virtual bool		create(udx_address*, udx_peer*& peer, u32& peer_idx) = 0;
	};

	static void ProcessLoop()
	{
		udx_alloc*			allocator = NULL;
		udx_packet_reader*	pkt_reader = NULL;
		udx_packet_writer*	pkt_writer = NULL;
		udx_registry*		peer_registry = NULL;
		udx_iaddress2idx*	address2idx = NULL;

		u32 const			max_num_peers = 1024;
		udx_peer**			active_peers = (udx_peer**)allocator->alloc(sizeof(udx_peer*) * max_num_peers);
		for (s32 i = 0; i < max_num_peers; ++i) 
			active_peers[i] = NULL;

		while (true)
		{
			// Drain the UDP socket:
			// - Create udx_packet from packet allocator
			// - Read UDP message into udx_packet
			// - Get peer from address
			//   - If no peer found then this means that this message should be a connection attempt
			//   - Create and initialize new peer
			// - Push incoming packet into peer
			//   This will initialize the 
			// - Activate peer into list of peers that need processing
			udx_packet* incoming_packet = NULL;
			while (pkt_reader->read(incoming_packet))
			{
				u32 remote_peer_idx;
				udx_address* remote_address = incoming_packet->get_address();
				address2idx->get_assoc(remote_address, remote_peer_idx);
				udx_peer* remote_peer = peer_registry->get(remote_address);
				remote_peer->push_incoming(incoming_packet);
				active_peers[remote_peer_idx] = remote_peer;
			}

			// Process all peers that need processing:
			// - Peers with outgoing messages
			// - Peers with incoming messages (from above)
			// - Peers with inflight messages (RTO check -> retransmit)
			
			// 
			// Any succesfully transmitted packets should be deallocated with the packet allocator
			//

			for (s32 i = 0; i < max_num_peers; ++i)
			{
				udx_peer* peer = active_peers[i];
				if (peer != NULL)
				{
					peer->
				}
			}
		}


	}

}
