#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-alloc.h"
#include "xp2p\libudx\x_udx-address.h"
#include "xp2p\libudx\x_udx-host.h"
#include "xp2p\libudx\x_udx-message.h"
#include "xp2p\libudx\x_udx-peer.h"
#include "xp2p\libudx\x_udx-registry.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-packetqueue.h"
#include "xp2p\libudx\x_udx-udp.h"
#include "xp2p\libudx\x_udx-time.h"

#include "xp2p\private\x_sockets.h"

namespace xcore
{
	struct udx_indices
	{
		u32*		m_indices;
		u32			m_size;
		u32			m_max;

		inline		udx_indices()
			: m_indices(nullptr)
			, m_size(0)
			, m_max(0)
		{

		}

		void		init(udx_alloc* _allocator, u32 size)
		{
			m_indices = (u32*)_allocator->alloc(size * sizeof(u32));
			m_size = size;
			m_max = size;
		}

		bool		pop(u32& index)
		{
			if (m_size > 0)
			{
				m_size -= 1;
				index = m_indices[m_size];
				return true;
			}
			return false;
		}

		void		push(u32 index)
		{
			m_indices[m_size++] = index;
		}
	};

	class udx_packet_garbage_collector : public udx_packet_writer
	{
		udx_alloc*		m_packet_allocator;
	public:
		inline			udx_packet_garbage_collector(udx_alloc* _packet_allocator) : m_packet_allocator(_packet_allocator) {}

		virtual bool	write(udx_packet* p)
		{
			m_packet_allocator->dealloc(p);
		}
	};

	class udx_packet_to_lqueue_collector : public udx_packet_writer
	{
		udx_packets_lqueue*	m_packet_queue;
	public:
		inline			udx_packet_to_lqueue_collector(udx_packets_lqueue*	packet_queue) : m_packet_queue(packet_queue) {}

		virtual bool	write(udx_packet* p)
		{
			m_packet_queue->push(p);
		}

	};


	// --------------------------------------------------------------------------------------------
	// [PRIVATE] IMPLEMENTATION OF HOST
	class udx_socket_host : public udx_host
	{
	public:
		udx_socket_host(udx_alloc* _allocator, udx_alloc* _msg_allocator);

		void					initialize();

		virtual udx_address*	get_address() const;

		virtual udx_msg			alloc_msg(u32 size);
		virtual void			free_msg(udx_msg& msg);

		virtual udx_address*	connect(const char* address);
		virtual bool			disconnect(udx_address*);
		virtual bool			is_connected(udx_address*) const;

		virtual bool			send(udx_msg& msg, udx_address* to);
		virtual bool			recv(udx_msg& msg, udx_address*& from);

		// Process time-outs and deal with re-transmitting, disconnecting etc..
		virtual void			process(u64 delta_time_us);

	protected:
		udx_alloc*				m_sys_alloc;
		udx_alloc*				m_msg_alloc;

		u32						m_MTU;		// Maximum Transmission Unit

		udx_address*			m_address;

		udx_iaddrin2address*	m_addrin_to_address;
		udx_iaddress2idx*		m_address_to_index;
		udx_address_factory*	m_address_factory;

		udx_peer_factory*		m_peer_factory;
		
		udp_socket*				m_udp_socket2;
		udx_packet_reader*		m_packet_from_socket_reader;
		udx_packet_writer*		m_packet_to_socket_writer;

		u32						m_max_peers;
		udx_peer**				m_all_peers;
		u32						m_num_active_peers;
		udx_peer**				m_active_peers;

		udx_indices				m_free_peer_indices;

		udx_packets_lqueue		m_recv_packets_list;
	};


	udx_socket_host::udx_socket_host(udx_alloc* allocator, udx_alloc* msg_allocator)
		: m_sys_alloc(allocator)
		, m_msg_alloc(msg_allocator)
		, m_addrin_to_address(nullptr)
		, m_address_to_index(nullptr)
		, m_address_factory(nullptr)
		, m_peer_factory(nullptr)
		, m_udp_socket2(nullptr)
		, m_packet_from_socket_reader(nullptr)
		, m_packet_to_socket_writer(nullptr)
		, m_max_peers(1024)
		, m_all_peers(nullptr)
		, m_num_active_peers(0)
		, m_active_peers(nullptr)
	{

	}

	void			udx_socket_host::initialize()
	{
		void* factory_mem = m_sys_alloc->alloc(sizeof(udx_address_factory));
		udx_address_factory* factory = new (factory_mem) udx_address_factory();
		m_address_factory = factory;
		m_addrin_to_address = m_address_factory;
		m_address_to_index = m_address_factory;

		m_udp_socket2 = gCreateUdpSocket(m_sys_alloc);

		udx_packet_rw_config config;
		config.m_address_factory = m_address_factory;
		config.m_addrin_2_address = m_address_factory;
		config.m_udp_socket = m_udp_socket2;
		gCreateUdxPacketReaderWriter(m_sys_alloc, config, m_packet_from_socket_reader, m_packet_to_socket_writer);

		m_free_peer_indices.init(m_sys_alloc, m_max_peers);
	}

	udx_address*	udx_socket_host::get_address() const
	{
		return m_address;
	}

	udx_msg			udx_socket_host::alloc_msg(u32 size)
	{
		void* msg = m_msg_alloc->alloc(size);
		return udx_msg(msg, size);
	}

	void			udx_socket_host::free_msg(udx_msg& msg)
	{
		m_msg_alloc->dealloc(msg.data_ptr);
		msg.data_ptr = nullptr;
		msg.data_size = 0;
	}

	udx_address*	udx_socket_host::connect(const char* addressstr)
	{
		udx_address* address = nullptr;

		udx_addrin addrin;
		if (addrin.from_string(addressstr) == 0)
		{
			if (m_addrin_to_address->get_assoc(addrin.m_data, addrin.m_len, address))
			{
				address = m_address_factory->create(addrin.m_data, addrin.m_len);
			}

			udx_peer* peer = nullptr;
			
			u32 peer_idx;
			if (m_address_to_index->get_assoc(address, peer_idx))
			{
				peer = m_all_peers[peer_idx];
			}
			
			if (peer == nullptr)
			{
				peer = m_peer_factory->create_peer(address);
				m_all_peers[peer_idx] = peer;
				m_address_to_index->set_assoc(address, peer_idx);
			}

			if (peer->is_connected() == false)
			{
				m_active_peers[peer_idx] = peer;
				m_num_active_peers += 1;
				peer->connect();
			}
		}
		return address;
	}

	bool			udx_socket_host::disconnect(udx_address* address)
	{
		u32 peer_idx;
		if (m_address_to_index->get_assoc(address, peer_idx))
		{
			udx_peer* peer = m_active_peers[peer_idx];
			if (peer != NULL)
			{
				if (peer->is_connected())
				{
					peer->disconnect();
					return true;
				}
			}
		}
		return false;
	}

	bool			udx_socket_host::is_connected(udx_address* address) const
	{
		u32 peer_idx;
		if (m_address_to_index->get_assoc(address, peer_idx))
		{
			udx_peer* peer = m_active_peers[peer_idx];
			if (peer != NULL)
			{
				return (peer->is_connected());
			}
		}
		return false;
	}

	bool	udx_socket_host::send(udx_msg& msg, udx_address* to)
	{
		u32 peer_index;
		if (m_address_to_index->get_assoc(to, peer_index))
		{
			udx_peer* peer = m_active_peers[peer_index];
			if (peer != nullptr)
			{
				udx_packet* packet = udx_packet::from_user(msg.data_ptr, msg.data_size);
				peer->send(packet);
				return true;
			}
		}
		return false;
	}

	bool	udx_socket_host::recv(udx_msg& msg, udx_address*& from)
	{
		udx_packet* packet;
		if (m_recv_packets_list.pop(packet))
		{
			msg.data_ptr = packet->to_user(msg.data_size);
			from = packet->get_address();
			return true;
		}
		from = NULL;
		return false;
	}

	void	udx_socket_host::process(u64 delta_time_us)
	{
		// Required:
		//  - Allocator to allocate memory for packets to send
		//    and receive.
		//  - Allocator to allocate udx objects like queues etc..

		// Drain the UDP socket for incoming packets
		//  - For every received packet give it to the associated udx peer
		//    If the udx peer doesn't exist create it and verify that
		//    the packet is a SYN packet.

		// NOTE:
		// This is currently a basic implementation with some missing
		// safety features:
		// - 'maximum peers' is not used yet
		// - Connecting and Disconnecting many 'new' peers will result
		//   in issues related to 'maximum peers'
		// - RST and FIN packets are not processed yet
		// - Destruction of a peer should garbage collect all packets
		//   that the peer is holding on to.

		udx_packet* packet = (udx_packet*)m_msg_alloc->alloc(m_MTU);
		while (m_packet_from_socket_reader->read(packet))
		{
			udx_packet_inf* packet_inf = packet->get_inf();
			m_msg_alloc->commit(packet, packet_inf->m_size_in_bytes);
			udx_address* address = packet_inf->m_remote_endpoint;

			u32 peer_idx;
			if (!m_address_to_index->get_assoc(address, peer_idx))
			{
				// This association doesn't exist, so get a new free peer index
				// and initialize this association between the address and the
				// peer index.
				m_free_peer_indices.pop(peer_idx);
				m_address_to_index->set_assoc(address, peer_idx);
			}

			udx_peer* peer = m_all_peers[peer_idx];
			if (peer == nullptr)
			{	// Create the peer
				peer = m_peer_factory->create_peer(address);
				m_all_peers[peer_idx] = peer;
			}
			
			// Give the received packet to the peer for further handling.
			peer->received(packet);

			// Register this peer in the array of active peers
			if (m_active_peers[peer_idx] == nullptr)
			{
				m_num_active_peers += 1;
				m_active_peers[peer_idx] = peer;
			}
		}

		// For every 'active' udx peer
		//  - process received packets
		//  - update RTT / RTO, CC
		//  - construct ACK data
		//  - determine any packets to resend
		//  - push packets on the socket
		//  - collect garbage
		udx_packet_garbage_collector packet_deallocator(m_msg_alloc);
		udx_packet_writer* garbage_collector = &packet_deallocator;
		udx_packet_to_lqueue_collector packets_to_queue_collector(&m_recv_packets_list);
		udx_packet_writer* incoming_packet_collector = &packets_to_queue_collector;
		for (u32 i = 0; i < m_num_active_peers; ++i)
		{
			udx_peer* peer = m_active_peers[i];
			if (peer != nullptr)
			{
				peer->process();

				peer->writeto_socket(m_packet_to_socket_writer);
				peer->collect_garbage(garbage_collector);
				peer->collect_incoming(incoming_packet_collector);
			}
		}

	}

}
