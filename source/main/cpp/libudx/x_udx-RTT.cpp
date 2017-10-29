#include "xbase/x_target.h"
#include "xp2p/x_sha1.h"
#include "xp2p\libudx/x_udx.h"
#include "xp2p\libudx/x_udx-rtt.h"
#include "xp2p\libudx/x_udx-ack.h"
#include "xp2p\libudx/x_udx-seqnr.h"
#include "xp2p\libudx/x_udx-packetqueue.h"

#include <chrono>

namespace xcore
{
	/*
	RTT - Round Trip Time

		For UDX with PCC we are only interested in an average RTT and derived RTO.
		Since RTT is not used to modify 'cwnd' (since we only have rate) it is not
		necessary to measure at a high frequency.

	Note: This is an exponential moving average accumulator. Add samples to it 
	      and it keeps track of a moving mean value and an average deviation.
	*/

	class udx_rtt_pcc : public udx_rtt
	{
	public:
		udx_rtt_pcc()
			: m_inverted_gain(10)
			, m_num_samples(0)
			, m_mean(0)
			, m_average_deviation(0)
		{

		}

		virtual s64		get_rtt_us() const		{ return m_rtt; }
		virtual s64		get_rto_us() const		{ return m_rto; }

		virtual void	update(udx_packet* pkt);
		
	protected:
		s64			m_inverted_gain;
		s64			m_num_samples;
		s64			m_mean;
		s64			m_average_deviation;

		s64			mean() const
		{
			s64 rtt_1sec = (1000 * 1000);
			return m_num_samples > 0 ? (m_mean + 32) / 64 : rtt_1sec;
		}

		s64			avg_deviation() const
		{
			return m_num_samples > 1 ? (m_average_deviation + 32) / 64 : 0;
		}

		s64			m_rtt;
		s64			m_rto;
	};

	void	udx_rtt_pcc::update(udx_packet* pkt)
	{
		udx_packet_inf* pkt_inf = pkt->get_inf();
		udx_packet_hdr* pkt_hdr = pkt->get_hdr();

		s64 rtt_sample = (pkt_inf->m_timestamp_rcvd_us - pkt_inf->m_timestamp_send_us) - pkt_hdr->m_hdr_ack_delay_us;
		rtt_sample *= 64;

		s64 deviation = 0;
		if (m_num_samples > 0)
		{
			deviation = abs(m_mean - rtt_sample);
			if (m_num_samples > 1)
			{
				m_average_deviation += (deviation - m_average_deviation) / m_num_samples;
			}
		}

		if (m_num_samples < m_inverted_gain)
		{
			++m_num_samples;
		}

		m_mean += (rtt_sample - m_mean) / m_num_samples;
	}

}
