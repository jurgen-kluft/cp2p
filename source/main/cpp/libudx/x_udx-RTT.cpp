#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\private\x_sockets.h"

#include <chrono>

namespace xcore
{

	class udx_ack_iterator
	{
	public:
		inline		udx_ack_iterator(u32 ack_seq_nr, u8 const* ack_data, u32 ack_data_size)
			: m_ack_seq_nr(ack_seq_nr)
			, m_ack_bit(1)
			, m_ack_data(ack_data)
			, m_ack_data_end(ack_data + ack_data_size)
		{}

		bool		pop(u32& seq_nr)
		{
			while ((m_ack_data != m_ack_data_end) && (*m_ack_data & m_ack_bit) == 0)
			{
				m_ack_seq_nr++;
				m_ack_bit <<= 1;
				if (m_ack_bit == 0x100)
				{
					m_ack_bit = 1;
					m_ack_data++;
				}
			}
			seq_nr = m_ack_seq_nr;
			return (m_ack_data != m_ack_data_end);
		}

	protected:
		u32			m_ack_seq_nr;
		u32			m_ack_bit;
		u8 const*	m_ack_data;
		u8 const*	m_ack_data_end;
	};

	class udx_rtt_imp : public udx_rtt
	{
	public:
		virtual void	on_send(udx_packet* pkt)
		{

		}

		virtual void	on_receive(u32 ack_segnr, u8* ack_data, u32 ack_data_size)
		{

		}

		virtual s64		get_rtt_us() const
		{

		}

		virtual s64		get_rto_us() const
		{
		}

	protected:
		s64				m_rtt;
		s64				m_rto;
		udx_alloc*		m_allocator;

		struct packet
		{
			u32				m_seq_nr;
			u64				m_ctime_us;
		};

		struct queue
		{
			u32				m_pivot;
			u32				m_size;
			packet*			m_packets;
		};
		queue			m_queue;
	};

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
}
