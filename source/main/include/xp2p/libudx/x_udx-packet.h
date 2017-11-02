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
#include "xp2p\libudx\x_udx-list.h"

namespace xcore
{
	class udx_address;

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API

	// The current layout of an allocated packet in memory is as follows:
	//
	// [  INF(64)  ][  HDR(64)  ][  BODY  ]
	// 
	// A packet might not use all of the ACK bytes in the header, to solve this
	// without data copying we do the following:
	// N number of tail bytes from the message body will be written to the tail
	// of the header and this in turn will shorten the udp message size.
	// The good thing is that the position of the message body in the udp packet
	// is always "u8* udp_packet + sizeof(udx_packet_hdr)", the only thing we
	// need to do after receiving the udp packet is to fix the tail of the body.

	// 64 bytes
	struct udx_packet_inf
	{
		u32						m_flags : 8;
		u32						m_size_in_bytes : 12;			// Full size [packet_inf + packet_hdr + packet_hdr + body]
		u32						m_body_in_bytes : 12;			// Actual body size for sending/receiving

		u32						m_retransmissions : 7;			// Retransmission count
		u32						m_need_resend : 1;
		u32						m_is_acked : 1;
		u32						m_used_for_rtt : 1;
		u32						m_dummy_x1 : 20;

		u64						m_timestamp_send_us;
		u64						m_timestamp_rcvd_us;			// (rcv - send) - rcvdelay = RTT
		udx_address*			m_remote_endpoint;

		udx_list_node			m_list_node;

		u32						m_dummy_xn[4];

		void					encode();
		void					decode();
	};

	// 64 bytes
	struct udx_packet_hdr
	{
		// Packet-header (64)
		enum EPktType
		{
			PKT_TYPE_SYN = 1,
			PKT_TYPE_FIN = 2,
			PKT_TYPE_RST = 3,
			PKT_TYPE_ACK = 4,
			PKT_TYPE_DATA = 8
		};

		u32						m_pkt_type : 4;				// SYN, FIN, RST, ACK, ACK|DATA
		u32						m_xxx1 : 4;					// Unused
		u32						m_pkt_seqnr : 24;			// Packet sequence number

		// --- ACK ------ 
		enum EAckType 
		{ 
			ACK_TYPE_ACK = 1, 
			ACK_TYPE_BITS = 2, 
			ACK_TYPE_RLE = 4
		};

		u32						m_ack_type : 3;				// ACK type (ACK=1, BITS=2, RLE=4)
		u32						m_ack_size : 9;				// ACK size in bits
		u32						m_ack_delay_us : 20;		// ACK delay time in micro-seconds (the time it took at this end-point to reply to ack-seqnr)

		u32						m_ack_seqnr : 24;			// ACK sequence number
		u32						m_xxx2 : 8;					// Unused

		u8						m_acks[52];					// ACK bit stream (52 * 8 = max 416 bits)

		// 52 bytes - RLE examples:
		// A:
		//    4 separate places where we have a lost packet
		//    4 * 4 bits = 16 bits = 2 bytes
		//    50 bytes left means = 50 * 128 = 6400 ACK bits
		// B:
		//    16 separate places where we have one lost packet
		//    16 * 4 bits = 64 bits = 8 bytes
		//    44 bytes left means = 44 * 128 = 5632 ACK bits
		// C:
		//    32 separate places where we have one lost packet
		//    32 * 4 bits = 128 bits = 16 bytes
		//    36 bytes left means = 36 * 128 = 4608 ACK bits

		udx_packet*				get_packet() const				{ return (udx_packet*)((u8*)this - sizeof(udx_packet_inf)); }
	};

	struct udx_packet
	{
		udx_packet_inf*			get_inf()						{ return (udx_packet_inf*)this; }
		udx_packet_hdr*			get_hdr()						{ return (udx_packet_hdr*)((u8*)this + sizeof(udx_packet_inf)); }
		udx_packet_inf const*	get_inf() const					{ return (udx_packet_inf const*)this; }
		udx_packet_hdr const*	get_hdr() const					{ return (udx_packet_hdr const*)((u8 const*)this + sizeof(udx_packet_inf)); }

		udx_seqnr				get_seqnr() const				{ udx_packet_hdr const * hdr = get_hdr(); return hdr->m_pkt_seqnr; }
		udx_list_node*			get_list_node()					{ udx_packet_inf * inf = get_inf(); return &inf->m_list_node; }
		udx_address*			get_address() const				{ udx_packet_inf const* inf = get_inf(); return inf->m_remote_endpoint; }

		void					set_acked()						{ udx_packet_inf * inf = get_inf(); inf->m_is_acked = 1; }

		void*					to_user(u32& size) const;		// Pointer and size of the user msg transfered or received
		void*					to_udp(u32& size) const;		// Pointer and size of the udp msg transfered or received

		static udx_packet*		from_user(void* user, u32 size);
		static udx_packet*		from_udp(void* udp, u32 size, udx_address* address);
	};
}

#endif
