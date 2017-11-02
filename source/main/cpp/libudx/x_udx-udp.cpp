#include "xbase\x_target.h"
#include "xp2p\libudx\x_udx-address.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-time.h"
#include "xp2p\libudx\x_udx-udp.h"

#ifdef PLATFORM_PC
#include <winsock2.h>         // For socket(), connect(), send(), and recv()
#include <ws2tcpip.h>
typedef int socklen_t;
typedef char raw_type;       // Type used for raw data on this platform
#else

// Other platforms ?

#endif


namespace xcore
{
	class udp_socket_win : public udp_socket
	{
		s32		mSocketDescriptor;

	public:
		virtual bool	send(void* pkt, u32 pkt_size, udx_addrin const& addrin)
		{
			sockaddr_in* destAddr = (sockaddr_in*)addrin.m_data;
			if (::sendto(mSocketDescriptor, (char const*)pkt, pkt_size, 0, (sockaddr *)destAddr, sizeof(sockaddr_in)) != pkt_size)
			{
				return false;
			}
			return true;
		}

		virtual bool	recv(void* pkt, u32& pkt_size, udx_addrin& addrin)
		{
			sockaddr_in* clntAddr = (sockaddr_in*)addrin.m_data;
			addrin.m_len = sizeof(clntAddr);

			s32 rtn;
			if ((rtn = ::recvfrom(mSocketDescriptor, (char*)pkt, pkt_size, 0, (sockaddr *)clntAddr, (socklen_t *)&addrin.m_len)) < 0)
			{
				return false;
			}

			// If no error occurs, recvfrom returns the number of bytes received 
			pkt_size = rtn;
			return true;
		}

		XCORE_CLASS_PLACEMENT_NEW_DELETE
	};


	udp_socket*		gCreateUdpSocket(udx_alloc* allocator)
	{
		void* udp_socket_win_mem = allocator->alloc(sizeof(udp_socket_win));
		udp_socket_win* imp = new (udp_socket_win_mem) udp_socket_win();
		return imp;
	}

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] API
	class udx_packet_writer_imp : public udx_packet_writer
	{
		udp_socket*				m_udp_socket;

	public:
		udx_packet_writer_imp(udp_socket* _udp_socket)
			: m_udp_socket(_udp_socket)
		{

		}

		virtual bool	write(udx_packet* pkt)
		{
			udx_packet_inf* inf = pkt->get_inf();
			udx_packet_hdr* hdr = pkt->get_hdr();

			// Before sending encode the packet
			inf->encode();

			// Push it out on the UDP socket
			m_udp_socket->send((void*)hdr, inf->m_body_in_bytes + sizeof(udx_packet_hdr), inf->m_remote_endpoint->get_addrin());

			// We have send it, timestamp the send moment
			inf->m_timestamp_send_us = udx_time::get_time_us();

			return false;
		}

		XCORE_CLASS_PLACEMENT_NEW_DELETE
	};

	static udx_packet_writer_imp*	CreatePacketWriter(udx_alloc* _allocator, udx_packet_rw_config& _config)
	{
		u32 const size = sizeof(udx_packet_writer_imp);
		void* mem = _allocator->alloc(size);
		_allocator->commit(mem, size);
		udx_packet_writer_imp* writer = new (mem) udx_packet_writer_imp(_config.m_udp_socket);
		return writer;
	}



	class udx_packet_reader_imp : public udx_packet_reader
	{
		udp_socket*				m_udp_socket;
		udx_iaddress_factory*	m_address_factory;
		udx_iaddrin2address*	m_addrin_2_address;

	public:
		udx_packet_reader_imp(udp_socket* _udp_socket, udx_iaddress_factory* _address_factory, udx_iaddrin2address* _addrin_2_address)
			: m_udp_socket(_udp_socket)
			, m_address_factory(_address_factory)
			, m_addrin_2_address(_addrin_2_address)
		{

		}
		virtual bool	read(udx_packet* pkt)
		{
			udx_packet_inf* inf = pkt->get_inf();
			udx_packet_hdr* hdr = pkt->get_hdr();

			u32 pkt_size = inf->m_body_in_bytes;
			
			udx_addrin addrin;
			if (m_udp_socket->recv((void*)hdr, pkt_size, addrin))
			{
				inf->m_timestamp_rcvd_us = udx_time::get_time_us();
				inf->m_body_in_bytes = pkt_size - sizeof(udx_packet_hdr);

				udx_address* remote_endpoint;
				if (!m_addrin_2_address->get_assoc(addrin.m_data, addrin.m_len, remote_endpoint))
				{
					remote_endpoint = m_address_factory->create(addrin.m_data, addrin.m_len);
					m_addrin_2_address->set_assoc(addrin.m_data, addrin.m_len, remote_endpoint);
				}

				inf->m_remote_endpoint = remote_endpoint;
				inf->decode();	//< Decode the packet
				return true;
			}
			return false;
		}

		XCORE_CLASS_PLACEMENT_NEW_DELETE
	};

	static udx_packet_reader_imp*	CreatePacketReader(udx_alloc* _allocator, udx_packet_rw_config& _config)
	{
		u32 const size = sizeof(udx_packet_reader_imp);
		void* mem = _allocator->alloc(size);
		_allocator->commit(mem, size);
		udx_packet_reader_imp* reader = new (mem) udx_packet_reader_imp(_config.m_udp_socket, _config.m_address_factory, _config.m_addrin_2_address);
		return reader;
	}




	void	gCreateUdxPacketReaderWriter(udx_alloc* _allocator, udx_packet_rw_config& _config, udx_packet_reader*& _out_reader, udx_packet_writer*& _out_writer)
	{
		_out_reader = CreatePacketReader(_allocator, _config);
		_out_writer = CreatePacketWriter(_allocator, _config);
	}

}
