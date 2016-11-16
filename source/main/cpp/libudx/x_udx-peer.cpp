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
		udx_socket_peer(udx_alloc* _allocator, udx_alloc* _msg_allocator, udx_addresses* _addresses);

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
		udx_addresses*			m_addresses;
	};

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] IMPLEMENTATION OF HOST
	class udx_socket_host : public udx_host
	{
	public:
		udx_socket_host(udx_alloc* _allocator, udx_alloc* _msg_allocator);

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

		udx_address*			m_address;

		udx_factory*			m_factory;
		udx_addresses*			m_addresses;
		udp_socket*				m_udp_socket;

		u32						m_max_peers;
		u32						m_num_peers;
		udx_peer**				m_all_peers;
	};


	udx_socket_host::udx_socket_host(udx_alloc* allocator, udx_alloc* msg_allocator)
		: m_sys_alloc(allocator)
		, m_msg_alloc(msg_allocator)
		, m_address(NULL)
		, m_factory(NULL)
		, m_addresses(NULL)
		, m_udp_socket(NULL)
		, m_max_peers(1024)
		, m_num_peers(0)
		, m_all_peers(NULL)
	{

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
		msg.data_ptr = NULL;
		msg.data_size = 0;
	}

	udx_address*	udx_socket_host::connect(const char* addressstr)
	{
		udx_address* address = m_addresses->add(addressstr);
		udx_peer* peer = address->get_peer();
		if (peer == NULL)
		{
			peer = m_factory->create_peer(address);
			address->set_peer(peer);
		}
		if (peer->is_connected() == false)
		{
			peer->connect();
		}
		return address;
	}

	bool			udx_socket_host::disconnect(udx_address* address)
	{
		if (address->get_peer() != NULL)
		{
			udx_peer* peer = address->get_peer();
			if (peer->is_connected())
			{
				peer->disconnect();
				return true;
			}
		}
		return false;
	}

	bool			udx_socket_host::is_connected(udx_address* address) const
	{
		if (address->get_peer() != NULL)
		{
			udx_peer* peer = address->get_peer();
			return (peer->is_connected());
		}
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
