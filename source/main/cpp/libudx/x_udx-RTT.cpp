#include "xbase/x_target.h"
#include "xp2p/x_sha1.h"
#include "xp2p/libudx/x_udx.h"
#include "xp2p/libudx/x_udx-rtt.h"
#include "xp2p/libudx/x_udx-ack.h"
#include "xp2p/libudx/x_udx-seqnr.h"
#include "xp2p/libudx/x_udx-packetqueue.h"

#include <chrono>

namespace xcore
{
	/*
	RTT - Round Trip Time and Retransmission Time Out

		For UDX with PCC we are only interested in an average RTT and derived RTO.
		Since RTT is not used to modify 'cwnd' (since we only have rate) it is not
		necessary to measure at a high frequency.


	Note: This is an exponential moving average accumulator. Add samples to it
		  and it keeps track of a moving mean value and an average deviation.


	Note: Karn�s Algorithm

		One mandatory algorithm that prevents the RTT measurement from giving false 
		results is Karn�s Algorithm. It simply states that the RTT samples should 
		not be taken for retransmitted packets.

		In other words, the TCP sender keeps track whether the segment it sent was 
		a retransmission and skips the RTT routine for those acknowledgments. 
		This makes sense, since otherwise the sender could not distinguish acknowledgements 
		between the original and retransmitted segment.
	*/

	class udx_rtt_pcc : public udx_rtt
	{
	public:
		udx_rtt_pcc()
		{
			reset();
		}

		virtual void	reset();

		virtual void	on_ack(udx_packet const* p);

		virtual s64		get_rtt_us() const { return (s64)(m_rtt_ms * 1000); }
		virtual s64		get_rto_us() const { return (s64)(m_rto_ms * 1000); }

		XCORE_CLASS_PLACEMENT_NEW_DELETE

	protected:
		double			m_rtt_ms;
		double			m_rto_ms;

		s32				m_measurements;
		double			m_srtt3_last;		// srtt(k), srtt(k+1) - Jacobson
		double			m_serr_last;		// serr(k), serr(k+1) - Jacobson
		double			m_sdev_last;		// sdev(k), sdev(k+1) - Jacobson
	};

	//----- Defines -------------------------------------------------------------- 
	const double	PARA_F = 4;			// The new TCP RTO constant factor "f"
	const double	PARA_G = 0.125;		// "g" value in Jacobson's algorithm
	const double	PARA_H = 0.25;		// "h" value in Jacobson's algorithm
	const double	SRTT_0 = 1500;		// Initial value of smoothed RTT (ms)
	const double	SDEV_0 = 0;			// Initial value of smoothed deviation
	const double	MIN_RTO = 200.0;	// Minimum value of RTO (ms)
	const double	MAX_RTO = 60000.0;	// Maximum value of RTO (ms)

	void	udx_rtt_pcc::reset()
	{
		m_rtt_ms = 0.0;
		m_rto_ms = 1000.0;
		m_measurements = 0;
	}

	void	udx_rtt_pcc::on_ack(udx_packet const* p)
	{
		udx_packet_inf const * info = p->get_inf();
		u64 const time_send_us = info->m_timestamp_send_us;
		u64 const time_recv_us = info->m_timestamp_rcvd_us;
		udx_packet_hdr const * header = p->get_hdr();
		u64 remote_delay_us = header->m_ack_delay_us;
		udx_seqnr const segnr = header->m_pkt_seqnr;

		s64 const rtt_us = (time_recv_us - time_send_us) - remote_delay_us;
		double rtt_ms = (double)rtt_us / 1000.0;

		double srtt3_now;		// srtt(k), srtt(k+1) - Jacobson
		double serr_now;		// serr(k), serr(k+1) - Jacobson
		double sdev_now;		// sdev(k), sdev(k+1) - Jacobson

		// Calculate RTO using the Jacobson algorithm
		if (m_measurements == 1)
		{
			srtt3_now = (1 - PARA_G) * SRTT_0 + PARA_G * rtt_ms;
			serr_now = rtt_ms - SRTT_0;
			sdev_now = (1 - PARA_H) * SDEV_0 + PARA_H * fabs(serr_now);
			m_rto_ms = srtt3_now + PARA_F * sdev_now;
		}
		else
		{
			srtt3_now = (1 - PARA_G) * m_srtt3_last + PARA_G * rtt_ms;
			serr_now = m_rtt_ms - m_srtt3_last;
			sdev_now = (1 - PARA_H) * m_sdev_last + PARA_H * fabs(serr_now);
			m_rto_ms = srtt3_now + PARA_F * sdev_now;
		}

		// Clamp RTO between MIN_RTO and MAX_RTO
		if (m_rto_ms < MIN_RTO)
			m_rto_ms = MIN_RTO;
		else if (m_rto_ms > MAX_RTO)
			m_rto_ms = MAX_RTO;

		m_srtt3_last = srtt3_now;
		m_serr_last = serr_now;
		m_sdev_last = sdev_now;

		m_rtt_ms = srtt3_now;

		m_measurements += 1;
	}

	udx_rtt*	gCreateDefaultRTT(udx_alloc* _allocator)
	{
		x_cdtor_placement_new<udx_rtt_pcc> creator;
		u32 const size = sizeof(udx_rtt_pcc);
		void* mem = _allocator->alloc(size);
		_allocator->commit(mem, size);
		return creator.construct(mem);
	}
}
