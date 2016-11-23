#include "xbase\x_target.h"
#include "xp2p\libudx\x_udx-packet.h"

namespace xcore
{
	// User message data block
	void*		udx_packet::get_msg(u32& size)
	{
		// The message data block exists after the header
		udx_packet_inf*	inf = get_inf();
		size = inf->m_size_in_bytes - sizeof(udx_packet_inf) - sizeof(udx_packet_hdr);
		u8* msgblock = (u8*)((u8*)this + sizeof(udx_packet_inf) + sizeof(udx_packet_hdr));

		// Figure out if this is a receiving or sending packet, a sending packet is
		// easy since the msg starts after a fixed 'compressed' udx_packet_hdr struct.

		// A receiving packet has the data start anywhere in or right after the 'compressed'
		// udx_packet_hdr so in this case we need to find out the size of the compressed header.

		return msgblock;
	}

	// Pointer and size of the packet to transfer
	void*	udx_packet::to_udp(u32& size) const
	{
		// Figure out the size of the header, this is dynamic since the ACK data is not
		// constant size
		udx_packet_hdr const* hdr = get_hdr();
		u32 const real_ack_bytes = (hdr->m_hdr_ack_size + 7) / 8;
		u32 const max_ack_bytes = sizeof(hdr->m_hdr_acks);

		// We still should compact the udx_packet_hdr 
		u32 const sizeof_hdr = sizeof(udx_packet_hdr);
		u32 const offset_hdr = max_ack_bytes - real_ack_bytes;
		u32 const sizeof_com = sizeof_hdr - offset_hdr;

		u8* compacthdr = (u8*)((u8*)this + sizeof(udx_packet_inf) + sizeof(udx_packet_hdr) + offset_hdr);

		udx_packet_inf const* inf = get_inf();
		size = inf->m_body_in_bytes + sizeof_com;

		return compacthdr;
	}

	// Copy the 'accessible' packet header into the 'compact' header
	void		udx_packet::a2c_hdr()
	{
		// Figure out the size of the header, this is dynamic since the ACK data is not
		// constant size
		udx_packet_hdr const* hdr = get_hdr();
		u32 const real_ack_bytes = (hdr->m_hdr_ack_size + 7) / 8;
		u32 const max_ack_bytes = sizeof(hdr->m_hdr_acks);

		// We still should compact the udx_packet_hdr 
		u32 const sizeof_hdr = sizeof(udx_packet_hdr);
		u32 const offset_hdr = max_ack_bytes - real_ack_bytes;
		u32 const sizeof_com = sizeof_hdr - offset_hdr;

		u8 const* generichdr = (u8*)((u8*)this + sizeof(udx_packet_inf));
		u8* compacthdr = (u8*)((u8*)this + sizeof(udx_packet_inf) + sizeof(udx_packet_hdr));
		for (s32 i = 0; i < sizeof_com; ++i)
		{
			compacthdr[offset_hdr + i] = generichdr[i];
		}
	}

	udx_packet*		resolve_packet(void* udpdata, u32 udp_pkt_size, udx_address* address)
	{
		// Figure out what the header is and re-arrange the header-data from 'compressed' to 'accessible'
	}
}
