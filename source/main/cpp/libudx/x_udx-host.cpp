#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-alloc.h"
#include "xp2p\libudx\x_udx-address.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-message.h"
#include "xp2p\libudx\x_udx-registry.h"
#include "xp2p\libudx\x_udx-host.h"
#include "xp2p\libudx\x_udx-udp.h"
#include "xp2p\libudx\x_udx-time.h"

#include "xp2p\private\x_sockets.h"

namespace xcore
{
	struct udx_packet_list
	{
		u32				m_size;
		udx_packet*		m_head;
		udx_packet*		m_tail;

		inline			udx_packet_list() : m_size(0), m_head(nullptr), m_tail(nullptr) {}

		bool			is_empty() const { return m_size == 0; }

		void			push(udx_packet* packet)
		{
			if (m_head == nullptr)
			{
				m_head = packet;
				m_tail = packet;
			}
			else
			{
				udx_packet_inf* packet_info = m_tail->get_inf();
				packet_info->m_next = packet;
				m_tail = packet;
			}
			m_size += 1;
		}

		bool			pop(udx_packet*& packet)
		{
			if (is_empty())
				return false;

			packet = m_head;

			udx_packet_inf* packet_info = m_head->get_inf();
			m_head = packet_info->m_next;
			packet_info->m_next = nullptr;
			m_size -= 1;

			return true;
		}
	};

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
		udx_socket*				m_udx_socket;

		u32						m_max_peers;
		udx_peer**				m_all_peers;
		u32						m_num_active_peers;
		udx_peer**				m_active_peers;

		udx_packet_list			m_recv_packets_list;
		udx_packet_list			m_send_packets_list;

		udx_indices				m_free_peer_indices;
	};


	udx_socket_host::udx_socket_host(udx_alloc* allocator, udx_alloc* msg_allocator)
		: m_sys_alloc(allocator)
		, m_msg_alloc(msg_allocator)
		, m_addrin_to_address(nullptr)
		, m_address_to_index(nullptr)
		, m_address_factory(nullptr)
		, m_peer_factory(nullptr)
		, m_udp_socket2(nullptr)
		, m_udx_socket(nullptr)
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
		m_udx_socket = gCreateUdxSocket(m_sys_alloc, m_udp_socket2, m_address_factory, m_addrin_to_address);

		m_free_peer_indices.init(m_sys_alloc, 4096);
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
				peer->push_outgoing(packet);
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
			return true;
		}
		return false;
	}

	enum epacket_type
	{
		INCOMING = 0,
		OUTGOING = 1
	};

	static void collect_packets(epacket_type ptype, udx_peer** peers, u32 max_peers, udx_packet_list& list)
	{
		u32 index = 0;
		do
		{
			udx_peer* peer = peers[index];
			if (peer != nullptr)
			{
				bool has_packet;
				udx_packet* packet;
				switch (ptype)
				{
					case INCOMING: has_packet = peer->pop_incoming(packet); break;
					case OUTGOING: has_packet = peer->pop_incoming(packet); break;
				}

				if (has_packet)
				{
					udx_packet_inf* packet_info = packet->get_inf();
					list.push(packet);
				}
				else
				{
					index = (index + 1) % max_peers;
				}
			}
			else
			{
				do
				{
					index = (index + 1) % max_peers;
					peer = peers[index];
				} while (peer == nullptr && index != 0);
			}
		} while (index != 0);
	}

	void	udx_socket_host::process(u64 delta_time_us)
	{
		// Required:
		//  - Allocator to allocate memory for packets to receive
		//  - Allocator to allocate new udx sockets

		// Drain the UDP socket for incoming packets
		//  - For every packet add it to the associated udx socket
		//    If the udx socket doesn't exist create it and verify that
		//    the packet is a SYN packet.

		// NOTE:
		// This is currently a basic implementation with some missing
		// safety features:
		// - 'maximum peers' is not used yet
		// - Connecting and Disconnecting many 'new' peers will result
		//   in issues related to 'maximum peers'

		udx_addrin addrin;
		udx_packet* packet = (udx_packet*)m_msg_alloc->alloc(m_MTU);
		while (m_udx_socket->recv(packet))
		{
			udx_packet_inf* packet_inf = packet->get_inf();
			udx_packet_hdr* packet_hdr = packet->get_hdr();

			m_msg_alloc->commit(packet, packet_inf->m_size_in_bytes);

			udx_address* address = packet_inf->m_remote_endpoint;

			udx_peer* peer = nullptr;
			u32 peer_idx;
			if (!m_address_to_index->get_assoc(address, peer_idx))
			{
				// This association doesn't exist, so get a new free peer index
				// and initialize this association between the address and the
				// peer index.
				m_free_peer_indices.pop(peer_idx);
				m_address_to_index->set_assoc(address, peer_idx);
			}

			peer = m_all_peers[peer_idx];
			if (peer == nullptr)
			{	// Create the peer
				peer = m_peer_factory->create_peer(address);
				m_all_peers[peer_idx] = peer;
				m_active_peers[peer_idx] = peer;
				m_num_active_peers += 1;
			}

			udx_packet* packet = udx_packet::from_udp(udp_pkt_data, udp_pkt_size, address);
			udx_packet_inf* packet_info = packet->get_inf();
			packet_info->m_timestamp_rcvd_us = udp_pkt_rcvd_time;
			peer->push_incoming(packet);

			if (m_active_peers[peer_idx] == nullptr)
			{
				m_num_active_peers += 1;
				m_active_peers[peer_idx] = peer;
			}

			udp_pkt_data = nullptr;
			udp_pkt_size = 0;
		}

		m_msg_alloc->dealloc(udp_pkt_data);

		u64 delta_time = 0;

		// For every 'active' udx socket
		//  - update RTT / RTO
		//  - update CC
		//  - construct ACK data ?
		for (u32 i = 0; i < m_num_active_peers; ++i)
		{
			udx_peer* peer = m_active_peers[i];
			if (peer != nullptr)
			{
				peer->process(delta_time);
			}
		}

		// Iterate over all 'active' udx sockets and send their scheduled packets
		collect_packets(OUTGOING, m_active_peers, m_max_peers, m_send_packets_list);

		udx_packet* packet_to_send;
		while (m_send_packets_list.pop(packet_to_send))
		{
			udx_packet_inf* packet_info = packet_to_send->get_inf();
			udx_addrin const & addrin = packet_info->m_remote_endpoint->get_addrin();

			u32 udp_packet_size;
			void* udp_packet_data = packet_to_send->to_udp(udp_packet_size);
			packet_info->m_timestamp_send_us = udx_time::get_time_us();
			if (!m_udp_socket->send(udp_packet_data, udp_packet_size, addrin))
			{
				break;
			}
		}
		
		// Iterate over all 'active' udx sockets and collect received packets
		// Build a singly linked list of all incoming packets (in-order!)
		collect_packets(INCOMING, m_active_peers, m_max_peers, m_recv_packets_list);


	}

}
