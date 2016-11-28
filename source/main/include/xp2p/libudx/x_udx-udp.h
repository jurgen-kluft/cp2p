//==============================================================================
//  x_udx-udp.h
//==============================================================================
#ifndef __XP2P_UDX_UDP_H__
#define __XP2P_UDX_UDP_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\libudx\x_udx-alloc.h"

namespace xcore
{
	struct udx_packet;
	struct udx_addrin;

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] API
	class udp_socket
	{
	public:
		virtual bool	send(void* pkt, u32 pkt_size, udx_addrin const& addrin) = 0;
		virtual bool	recv(void* pkt, u32& pkt_size, udx_addrin& addrin) = 0;
	};

	udp_socket*		gCreateUdpSocket(udx_alloc* allocator);

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] API
	class udx_socket
	{
	public:
		virtual bool	send(udx_packet*) = 0;
		virtual bool	recv(udx_packet*) = 0;
	};

	udx_socket*		gCreateUdxSocket(udx_alloc* allocator, udp_socket* _udp_socket, udx_iaddress_factory* _address_factory, udx_iaddrin2address* _addrin_2_address);
}

#endif