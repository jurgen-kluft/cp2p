//==============================================================================
//  x_udx.h
//==============================================================================
#ifndef __XP2P_UDX_PACKET_H__
#define __XP2P_UDX_PACKET_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xp2p\libudx\x_udx-seqnr.h"


namespace xcore
{
	class udx_address;

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API

	// The current layout of an allocated packet in memory is as follows:
	//
	// [  INF(32)  ][ QUEUE-NODE  ][  HDR(64)  ][  COMPACT HDR(64)  ][  BODY  ]

	// 32 bytes
	struct udx_packet_inf
	{
		u32						m_flags : 8;
		u32						m_size_in_bytes : 12;			// Full size [packet_inf + packet_hdr + packet_hdr + body]
		u32						m_body_in_bytes : 12;			// Actual body size that needs to be send

		u32						m_transmissions : 7;
		u32						m_need_resend : 1;
		u32						m_is_acked : 1;
		u32						m_used_for_rtt : 1;
		u32						m_dummy1 : 20;

		u64						m_timestamp_send_us;
		u64						m_timestamp_rcvd_us;			// (rcv - send) - rcvdelay = RTT
		udx_address*			m_remote_endpoint;
	};

	struct udx_interval
	{
		udx_seqnr				m_interval[2];
		u32						m_refcount;
	};

	struct udx_packet_qnode
	{
		udx_interval*			m_interval;

		udx_packet*				get_pkt()						{ return (udx_packet*)((u8*)this - sizeof(udx_packet_inf)); }
		udx_packet const*		get_pkt() const					{ return (udx_packet const*)((u8 const*)this - sizeof(udx_packet_inf)); }
	};

	// 64 bytes
	struct udx_packet_hdr
	{
		// Packet-header (64)
		u32						m_hdr_pkt_type : 4;				// SYN, FIN, RST, ACK
		u32						m_hdr_xxx1 : 4;					// Unused
		u32						m_hdr_pkt_seqnr : 24;			// Packet sequence number

		u32						m_hdr_ack_type : 3;				// ACK type (ACK=1, BITS=2, RLE=4)
		u32						m_hdr_ack_size : 9;				// ACK size in bits
		u32						m_hdr_ack_delay_us : 20;		// ACK delay time in micro-seconds (the time it took at this end-point to reply to ack-seqnr)

		u32						m_hdr_ack_seqnr : 24;			// ACK sequence number
		u32						m_hdr_xxx2 : 8;					// Unused

		u32						m_hdr_acks[13];					// ACK bit stream (13 * 4 * 8 = max 416 bits)
	};

	struct udx_packet
	{
		udx_packet_inf*			get_inf()						{ return (udx_packet_inf*)this; }
		udx_packet_hdr*			get_hdr()						{ return (udx_packet_hdr*)((u8*)this + sizeof(udx_packet_inf)); }
		udx_packet_inf const*	get_inf() const					{ return (udx_packet_inf const*)this; }
		udx_packet_hdr const*	get_hdr() const					{ return (udx_packet_hdr const*)((u8 const*)this + sizeof(udx_packet_inf)); }

		udx_address*			get_address() const				{ return udx_packet_inf* inf = get_inf(); return inf->m_remote_endpoint; }

		void*					get_msg(u32& size);				// User message data block

		void					a2c_hdr();						// Copy the 'accessible' packet header into the 'compact' header
		void const*				get_pkt(u32& size) const;		// Pointer and size of the packet to transfer
	};
}

#endif
