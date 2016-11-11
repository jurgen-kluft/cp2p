#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-alloc.h"
#include "xp2p\libudx\x_udx-address.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-message.h"
#include "xp2p\libudx\x_udx-registry.h"
#include "xp2p\libudx\x_udx-socket.h"
#include "xp2p\libudx\x_udx-udp.h"

#include "xp2p\private\x_sockets.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PRIVATE] IMPLEMENTATION OF HOST
	class udx_socket_peer : public udx_socket
	{
	public:
		udx_socket_peer(udx_alloc* _allocator, udx_alloc* _msg_allocator, udx_addresses* _addresses);

		virtual udx_address*	get_address() const;

		virtual void			push_incoming(udx_packet* packet);
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
	class udx_socket_host : public udx_socket
	{
	public:
		udx_socket_host(udx_alloc* _allocator, udx_alloc* _msg_allocator);

		virtual udx_address*	get_address() const;

		virtual udx_msg			alloc_msg(u32 size);
		virtual void			free_msg(udx_msg& msg);

		virtual udx_address*	connect(const char* address);
		virtual bool			disconnect(udx_address*);

		virtual void			send(udx_msg& msg, udx_address* to);
		virtual bool			recv(udx_msg& msg, udx_address*& from);

		// Process time-outs and deal with re-transmitting, disconnecting etc..
		virtual void			process(u64 delta_time_us);

	protected:
		udx_alloc*				m_sys_alloc;
		udx_alloc*				m_msg_alloc;

		udx_address*			m_address;
		u32						m_max_sockets;
		udx_socket*				m_all_sockets;
		u32						m_num_free_sockets;
		u32*					m_free_socket_list;

		udp_socket*				m_udp_socket;
		udx_addresses*			m_addresses;
	};


	udx_socket_host::udx_socket_host(udx_alloc* allocator, udx_alloc* msg_allocator)
		: m_sys_alloc(allocator)
		, m_msg_alloc(msg_allocator)
		, m_max_sockets(1024)
		, m_all_sockets(NULL)
		, m_num_free_sockets(0)
		, m_free_socket_list(NULL)
		, m_addresses(NULL)
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
		void* ipstructdata = NULL;
		u32 ipstructsize = 0;
		udx_address* address = m_addresses->find(ipstructdata, ipstructsize);
		if (address == NULL)
		{
			// Create new udx-socket
			// Create new udx-address
			address = m_addresses->add(ipstructdata, ipstructsize);
		}
		return address;
	}

	bool			udx_socket_host::disconnect(udx_address*)
	{

		return false;
	}

}
