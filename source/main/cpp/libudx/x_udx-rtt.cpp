#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\private\x_sockets.h"


namespace xcore
{
	/*

	*/

	class udx_rtt_imp
	{
	public:
		virtual void 	on_send(udx_packet* pkt)
		{
			m_queue.m_packets[m_queue.m_qindex++] = pkt;
			pkt->m_timestamp_send_us = get_current_time_us();
			if (m_queue.m_qindex == m_queue.m_size)
			{	// Wrap around
				m_queue.m_qindex = 0;
			}
		}

		virtual void 	on_receive(udx_packet* pkt)
		{
			pkt->m_timestamp_rcvd_us = get_current_time_us();

			// Dequeue any ack'ed packets and add their RTT into our RTT filter

			m_rtt_us = m_rtt_filter->get();
		}

		virtual s64 	get_rtt_us() const
		{
			return m_rtt_us;
		}

		virtual s64 	get_rto_us() const
		{
			return m_rto_us;
		}

	protected:
		u64				get_current_time_us() const
		{
			return 0;
		}
		udx_filter*		m_rtt_filter;

		s64				m_rtt_us;
		s64				m_rto_us;

		struct queue
		{
			u32				m_qindex;	// The 'queue-ing' index
			u32				m_dindex;	// The 'dequeue-ing' index
			u32				m_size;		// Maximum number of items
			udx_packet*		m_packets;	// The 'array' that is our queue
		};
		queue			m_queue;
	};

	/*
	MSS: is the maximum segment size

	TCP: ACK - SACK

		When sending ACK data to acknowledge the receipt of packets to the sender
		we advance the receive queue to thepoint where there is a gap in the seqnr,
		we then send ack_seqnr = seqnr-1.

		An ACK packet should be send every time we drain the UDP socket of data (recv)

	RTT: Smooth RTT, using 'moving' average computation

	TCP: given a new RTT measurement `RTT'
	http://www.erg.abdn.ac.uk/users/gerrit/dccp/notes/ccid2/rto_estimator/

		RTT : = max(RTT, 1)		// 1 jiffy sampling granularity

		if (this is the first RTT measurement)
		{
			SRTT: = RTT
			mdev : = RTT / 2
			mdev_max : = max(RTT / 2, 200msec / 4)
			RTTVAR : = mdev_max
			rtt_seq : = SND.NXT
		}
		else
		{
			SRTT'	 := SRTT + 1/8 * (RTT - SRTT)

				if (RTT < SRTT - mdev)
					mdev'	:= 31/32 * mdev + 1/32 * |RTT - SRTT|
				else
					mdev'	:= 3/4   * mdev + 1/4  * |RTT - SRTT|

				if (mdev' > mdev_max)
				{
					mdev_max : = mdev'
					if (mdev_max > RTTVAR)
						RTTVAR' := mdev_max
				}

				if (SND.UNA is `after' rtt_seq)
				{
					if (mdev_max < RTTVAR)
						RTTVAR' := 3/4 * RTTVAR + 1/4 * mdev_max
					rtt_seq  : = SND.NXT
					mdev_max : = 200msec / 4
				}
		}

		RTO' := SRTT + 4 * RTTVAR
*/

}
