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
		udx_alloc*				m_allocator;
		udx_alloc*				m_pkt_allocator;

		xnet::udpsocket*		m_udp_socket;

		u32						m_max_sockets;
		udx_socket*				m_all_sockets;
		u32						m_num_free_sockets;
		u32*					m_free_socket_list;

		udx_registry*			m_address_to_socket;
	};



}
