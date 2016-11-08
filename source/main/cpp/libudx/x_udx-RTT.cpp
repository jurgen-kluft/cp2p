#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-rtt.h"
#include "xp2p\libudx\x_udx-ack.h"
#include "xp2p\libudx\x_udx-seqnr.h"
#include "xp2p\libudx\x_udx-packetqueue.h"

#include <chrono>

namespace xcore
{
	/*
	MSS: is the maximum segment size

	TCP: ACK - SACK

		When sending ACK data to acknowledge the receipt of packets to the sender we advance the receive queue to the
		point where there is a gap in the seqnr, we then send ack_seqnr = seqnr-1.

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

	class udx_rtt_imp : public udx_rtt
	{
	public:
		virtual void	update(udx_packet* pkt)
		{

		}

		virtual s64		get_rtt_us() const
		{
			return m_rtt;
		}

		virtual s64		get_rto_us() const
		{
			return m_rto;
		}

	protected:
		s64				m_rtt;
		s64				m_rto;

	};

}
