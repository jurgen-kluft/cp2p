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
#include "xp2p\libudx\x_udx-bitstream.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_ack_iterator
	{
	public:
		virtual bool		pop(udx_seqnr& seq_nr, bool& acked) = 0;
	};

	class udx_ack_builder
	{
	public:
		s32		build(udx_packet_hdr* pkt_hdr, udx_packetsqueue* packet_queue)
		{
			s32 num_acks = 0;

			u32 c;
			udx_seqnr s;
			udx_packet* p;

			if (packet_queue->begin(c, s, p))
			{
				u8* ackdata = pkt_hdr->m_hdr_acks;
				u32 const ackmaxbits = sizeof(pkt_hdr->m_hdr_acks) * 8;
				udx_bitstream ackbits(ackmaxbits, ackdata);
				ackbits.set_range_false(0, ackmaxbits);

				udx_seqnr const base_seqnr = s;
				do
				{
					u32 const bitnr = (u32)((s - base_seqnr).get());
					if (bitnr > ackbits.get_maxbits())
						break;
					num_acks = bitnr;
					bool const bitval = (p != NULL);
					ackbits.set_bit(bitnr, bitval);
				} while (packet_queue->next(c, s, p));

				pkt_hdr->m_hdr_ack_seqnr = base_seqnr.to_pktseqnr();
				pkt_hdr->m_hdr_ack_type = udx_packet_hdr::ACK_TYPE_BITS;
				pkt_hdr->m_hdr_ack_size = num_acks;
			}
			return (s32)num_acks;
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