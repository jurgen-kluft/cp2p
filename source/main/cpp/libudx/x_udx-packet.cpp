#include "xbase\x_target.h"
#include "xp2p\libudx\x_udx-packet.h"

namespace xcore
{
	void		udx_packet_inf::encode()
	{

	}

	void		udx_packet_inf::decode()
	{

	}

	// User message data block
	void*		udx_packet::to_user(u32& size) const
	{
		// The message data block exists after the header
		udx_packet_inf const* inf = get_inf();
		size = inf->m_size_in_bytes - sizeof(udx_packet_inf) - sizeof(udx_packet_hdr);
		u8* msgblock = (u8*)((u8*)this + sizeof(udx_packet_inf) + sizeof(udx_packet_hdr));

		// Figure out if this is a receiving or sending packet, a sending packet is
		// easy since the msg starts after a fixed 'compressed' udx_packet_hdr struct.

		// A receiving packet has the message body start at sizeof(udx_packet_hdr) so in 
		// this case we do not need to compute anything, the only thing to do is to fix
		// the tail of the message body.

		return msgblock;
	}

	// Pointer and size of the packet to transfer
	void*	udx_packet::to_udp(u32& size) const
	{
		// Figure out the size of the header
		udx_packet_hdr const* hdr = get_hdr();
		u32 const real_ack_bytes = (hdr->m_ack_size + 7) / 8;
		u32 const max_ack_bytes = sizeof(hdr->m_acks);

		// Compute the size of the udp packet
		u32 const sizeof_hdr = sizeof(udx_packet_hdr);
		u32 const offset_hdr = max_ack_bytes - real_ack_bytes;
		u32 const sizeof_com = sizeof_hdr - offset_hdr;

		udx_packet_inf const* inf = get_inf();
		size = inf->m_body_in_bytes + sizeof_com;

		return (void*)hdr;
	}

	udx_packet*		udx_packet::from_user(void* user, u32 size)
	{
		udx_packet* packet = (udx_packet*)((u8*)user - sizeof(udx_packet_inf) - sizeof(udx_packet_hdr));
		return packet;
	}

	udx_packet*		udx_packet::from_udp(void* udpdata, u32 udp_pkt_size, udx_address* address)
	{
		udx_packet_hdr* packet_info = (udx_packet_hdr*)udpdata;
		return packet_info->get_packet();
	}
}
