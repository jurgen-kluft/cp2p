#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-alloc.h"
#include "xp2p\libudx\x_udx-address.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-message.h"
#include "xp2p\libudx\x_udx-registry.h"
#include "xp2p\libudx\x_udx-peer.h"
#include "xp2p\libudx\x_udx-udp.h"

#include "xp2p\private\x_sockets.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PRIVATE] IMPLEMENTATION OF HOST
	class udx_socket_peer : public udx_peer
	{
	public:
		udx_socket_peer(udx_alloc* _allocator, udx_alloc* _msg_allocator, udx_address* _address);

		virtual udx_address*	get_address() const;

		virtual void			push_incoming(udx_packet* packet);
		virtual bool			pop_incoming(udx_packet*& packet);

		virtual void			push_outgoing(udx_packet* packet);
		virtual bool			pop_outgoing(udx_packet*& packet);

		// Process time-outs and deal with re-transmitting, disconnecting etc..
		virtual void			process(u64 delta_time_us);

	protected:
		udx_alloc*				m_sys_alloc;
		udx_alloc*				m_msg_alloc;

		udx_address*			m_address;
		udp_socket*				m_udp_socket;
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

		virtual void			send(udx_msg& msg, udx_address* to);
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

		udx_factory*			m_factory;
		udp_socket*				m_udp_socket;

		u32						m_max_peers;
		udx_peer**				m_all_peers;
		u32						m_num_active_peers;
		udx_peer**				m_active_peers;

		u32						m_send_peer_index;
		u32						m_recv_peer_index;

		u32						m_num_free_peers;
		u32*					m_free_peers;
		u32						get_free_peer_index()
		{
			if (m_num_free_peers > 0)
			{
				m_num_free_peers -= 1;
				return m_free_peers[m_num_free_peers];
			}
			return -1;
		}
		void					add_free_peer_index(u32 peer_index)
		{
			m_free_peers[m_num_free_peers++] = peer_index;
		}
	};


	udx_socket_host::udx_socket_host(udx_alloc* allocator, udx_alloc* msg_allocator)
		: m_sys_alloc(allocator)
		, m_msg_alloc(msg_allocator)
		, m_addrin_to_address(nullptr)
		, m_address_to_index(nullptr)
		, m_address_factory(nullptr)
		, m_factory(nullptr)
		, m_udp_socket(nullptr)
		, m_max_peers(1024)
		, m_all_peers(nullptr)
		, m_num_active_peers(0)
		, m_active_peers(nullptr)
		, m_num_free_peers(0)
		, m_free_peers(nullptr)
	{

	}

	void			udx_socket_host::initialize()
	{
		void* factory_mem = m_sys_alloc->alloc(sizeof(udx_address_factory));
		udx_address_factory* factory = new (factory_mem) udx_address_factory();
		m_address_factory = factory;
		m_addrin_to_address = m_address_factory;
		m_address_to_index = m_address_factory;

		m_send_peer_index = 0;
		m_recv_peer_index = 0;

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
				// This means that there is no peer yet, we need to create it
			
			}
			
			if (peer == nullptr)
			{
				peer = m_factory->create_peer(address);
				m_address_to_index->set_assoc(address, peer_idx);
			}
			if (peer->is_connected() == false)
			{
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

	void	udx_socket_host::send(udx_msg& msg, udx_address* to)
	{
		u32 peer_index;
		if (m_address_to_index->get_assoc(to, peer_index))
		{
			udx_peer* peer = m_active_peers[peer_index];
			if (peer != nullptr)
			{
				udx_packet* packet = udx_packet::from_msg(msg);
				peer->handle_outgoing(packet);
			}
		}
	}

	bool	udx_socket_host::recv(udx_msg& msg, udx_address*& from)
	{
		if (m_num_active_peers == 0)
			return false;

		u32 index = m_send_peer_index;
		do
		{
			udx_peer* peer = m_active_peers[index];
			if (peer != nullptr)
			{
				udx_packet* packet;
				if (peer->release_incoming(packet))
				{
					m_send_peer_index = index;

					u32 msg_size;
					void* msg_body = packet->to_msg(msg_size);
					msg.data_ptr = msg_body;
					msg.data_size = msg_size;
					return true;
				}
				else
				{
					index = (index + 1) % m_max_peers;
				}
			}
			else
			{
				while (peer == nullptr && index != m_send_peer_index)
				{
					index = (index + 1) % m_max_peers;
					peer = m_active_peers[index];
				}
			}
		} while (index != m_send_peer_index);

		return false;
	}

	void	udx_socket_host::process(u64 delta_time_us)
	{
		// Required:
		//  - Allocator to allocate memory for packets to receive
		//  - Allocator to allocate new udx sockets

		// Drain the UDP socket for incoming packets
		//  - For every packet add it to the associated udx socket
		//    If the udx socket doesn't exist create it and verify that
		//    the packet is a SYN packet

		// NOTE:
		// This is currently a basic implementation with some missing
		// safety features:
		// - 'maximum peers' is not used yet
		// - Connecting and Disconnecting many 'new' peers will result
		//   in issues related to 'maximum peers'

		udx_addrin addrin;

		u32 udp_pkt_size = m_MTU;
		void* udp_pkt_data = m_msg_alloc->alloc(m_MTU);
		
		while (m_udp_socket->recv(udp_pkt_data, udp_pkt_size, addrin))
		{
			m_msg_alloc->commit(udp_pkt_data, udp_pkt_size);

			udx_address* address = nullptr;
			m_addrin_to_address->get_assoc(addrin.m_data, addrin.m_len, address);
			if (address == nullptr)
			{
				address = m_address_factory->create(addrin.m_data, addrin.m_len);
				m_addrin_to_address->set_assoc(addrin.m_data, addrin.m_len, address);
			}

			udx_peer* peer = nullptr;
			u32 peer_idx;
			if (!m_address_to_index->get_assoc(address, peer_idx))
			{
				// This association doesn't exist, so get a new free peer index
				// and initialize this association between the address and the
				// peer index.
				peer_idx = get_free_peer_index();
				m_address_to_index->set_assoc(address, peer_idx);
			}

			peer = m_all_peers[peer_idx];
			if (peer == nullptr)
			{	// Create the peer
				peer = m_factory->create_peer(address);
				m_all_peers[peer_idx] = peer;
				m_active_peers[peer_idx] = peer;
				m_num_active_peers += 1;
			}

			udx_packet* packet = udx_packet::from_udp_packet(udp_pkt_data, udp_pkt_size, address);
			peer->handle_incoming(packet);

			udp_pkt_data = nullptr;
			udp_pkt_size = 0;
		}

		m_msg_alloc->dealloc(udp_pkt_data);

		u64 delta_time = 0;

		// For every 'active' udx socket
		//  - update RTT / RTO
		//  - update CC
		for (u32 i = 0; i < m_num_active_peers; ++i)
		{
			udx_peer* peer = m_active_peers[i];
			if (peer != nullptr)
			{
				peer->process(delta_time);
			}
		}

		// Iterate over all 'active' udx sockets and construct ACK data
		// Iterate over all 'active' udx sockets and send their scheduled packets
		// Iterate over all 'active' udx sockets and collect received packets



	}

}
