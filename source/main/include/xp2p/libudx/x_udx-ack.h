//==============================================================================
//  x_udx-ack.h
//==============================================================================
#ifndef __XP2P_UDX_ACK_H__
#define __XP2P_UDX_ACK_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-packetqueue.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_ack_iterator
	{
	public:
		virtual bool		pop(u32& seq_nr) = 0;
	};

	class udx_ack_builder
	{
	public:
		void	build(udx_packet* p, udx_packetqueue* rcvd_packets)
		{
			udx_seqnr ackseqnr;
		}
	};

	class udx_ack_reader : public udx_ack_iterator
	{
	public:
		inline		udx_ack_reader(udx_seqnr ack_seq_nr, udx_bitstream& ackstream)
			: m_ack_seq_nr(ack_seq_nr)
			, m_ack_bit(0)
			, m_ack_stream(ackstream)
		{}

		virtual bool		pop(udx_seqnr& seqnr, bool& acked)
		{
			seqnr = m_ack_seq_nr;
			acked = m_ack_stream.is_true(m_ack_bit);
			m_ack_seq_nr.inc();
			m_ack_bit++;
			return m_ack_bit < m_ack_stream.get_maxbits();
		}

	protected:
		udx_seqnr		m_ack_seq_nr;
		u32				m_ack_bit;
		udx_bitstream	m_ack_stream;
	};


}

#endif