//==============================================================================
//  x_udx-udp.h
//==============================================================================
#ifndef __XP2P_UDX_UDP_H__
#define __XP2P_UDX_UDP_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p/libudx/x_udx-alloc.h"

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
	class udx_packet_writer
	{
	public:
		virtual bool	write(udx_packet*) = 0;
	};

	class udx_packet_reader
	{
	public:
		virtual bool	read(udx_packet*) = 0;
	};

	struct udx_packet_rw_config
	{
		udp_socket*				m_udp_socket;
		udx_iaddress_factory*	m_address_factory;
		udx_iaddrin2address*	m_addrin_2_address;
	};

	void	gCreateUdxPacketReaderWriter(udx_alloc* allocator, udx_packet_rw_config& _config, udx_packet_reader*& _out_reader, udx_packet_writer*& _out_writer);
}

#endif