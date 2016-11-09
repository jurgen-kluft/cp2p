#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-registry.h"
#include "xp2p\libudx\x_udx-socket.h"

#include "xp2p\private\x_sockets.h"

namespace xcore
{
	

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] IMPLEMENTATION
	class udx_socket_imp : public udx_socket
	{
	public:
		udx_socket_imp(udx_alloc* allocator, udx_alloc* msg_allocator);

		virtual udx_message		alloc_msg(u32 size);
		virtual void			free_msg(udx_message& msg);

		virtual udx_address*	connect(const char* address);
		virtual bool			disconnect(udx_address*);

		virtual void			send(udx_message& msg, udx_address* to);
		virtual bool			recv(udx_message& msg, udx_address*& from);

		// Process time-outs and deal with re-transmitting, disconnecting etc..
		virtual void			process(u64 delta_time_us);

	protected:
		udx_alloc*				m_sys_alloc;
		udx_alloc*				m_msg_alloc;

		xnet::udpsocket*		m_udp_socket;

		u32						m_max_sockets;
		udx_socket*				m_all_sockets;
		u32						m_num_free_sockets;
		u32*					m_free_socket_list;

		udx_registry*			m_address_to_socket;
	};


	udx_socket_imp::udx_socket_imp(udx_alloc* allocator, udx_alloc* msg_allocator)
		: m_sys_alloc(allocator)
		, m_msg_alloc(msg_allocator)
		, m_udp_socket(NULL)
		, m_max_sockets(1024)
		, m_all_sockets(NULL)
		, m_num_free_sockets(0)
		, m_free_socket_list(NULL)
		, m_address_to_socket(NULL)
	{

	}
	
	udx_message		udx_socket_imp::alloc_msg(u32 size)
	{
		void* msg = m_msg_alloc->alloc(size);
		return udx_message(msg, size);
	}

	void			udx_socket_imp::free_msg(udx_message& msg)
	{
		m_msg_alloc->dealloc(msg.data_ptr);
		msg.data_ptr = NULL;
		msg.data_size = 0;
	}

	udx_address*	udx_socket_imp::connect(const char* addressstr)
	{
		void* ipstructdata = NULL;
		u32 ipstructsize = 0;
		udx_address* address = m_address_to_socket->find(ipstructdata, ipstructsize);
		if (address == NULL)
		{
			address = m_address_to_socket->add(ipstructdata, ipstructsize);
		}
		return address;
	}

	bool			udx_socket_imp::disconnect(udx_address*)
	{

		return false;
	}

}
