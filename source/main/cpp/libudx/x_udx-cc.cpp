#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-bitstream.h"
#include "xp2p\private\x_sockets.h"

#include <chrono>

namespace xcore
{
	/*
	PCC:
		States:
			- Starting Phase
				- Ends when Utility(t) < Utility(t-1)
			- Moving Phase
				- Guessing

		Current predicament is what we should do when the request of packets to
		send is not requiring full throughput but just for example 1 mbps?
		Both the 'Starting Phase' and 'Moving Phase' can be constraint with a
		target throughput value where the 'Starting Phase' also ends when the
		target throughput is reached. The 'Moving Phase' can take the target
		throughput into account by not deviating from it by more than X %.

		When the target throughput changes we get 2 situations:
		- Decrease; we can do this immediately and we stay in the 'Moving Phase'
		- Increase; we stay in the 'Moving Phase' and we let PCC increase the
		            send-rate until it reaches the target rate.

		The 'Target Throughput' computation should be filtered with something
		like a 'Moving Average Smoothing Filter'.


	1 Gbps ~= 93622 packets per second (average size of packet == 1280 bytes)
	0
	UDX Goal:

	- udx::update(packets_to_send, packets_received) to be able to send ~100.000
	  packets/s as well as receive ~100.000 packets/s. This amount of packets
	  equals ~100 MiB/s.

	- udx:update should be called 

	*/


	/*
	Performance Monitoring

		The timeline is sliced into chunks of duration of T[m] called 
		the Monitor Interval (MI). When the Sending Module sends 
		packets (new or retransmission) at a certain sending rate 
		instructed by the Performance-oriented Rate Control Module, 
		the Monitor Module will remember what packets
		are sent out during each MI. As the SACK comes back
		from receiver, the Monitor will know what happened
		(received?  lost?  RTT?) to each packet sent out during an MI. 
		The Monitor knows what packets were sent during 
		MI[1], spanning T[0] to T[0]+T[m], and at time T[1], approximately 
		one RTT after T[0] + T[m], it gets the SACKs for all packets sent 
		out in MI[1]. The Monitor aggregates these individual SACKs
		to meaningful performance metrics including through-
		put, loss rate and average RTT. The performance metrics
		are then combined by a utility function and unless oth-
		erwise stated. 
		The end result of this is that we associate a control action of
		each MI (sending rate) with its performance result (utility).
		This information will be used by the performance oriented control
		module. To ensure there are enough packets in one monitor interval,
		we set T[m] to the maximum of:
		  (a) the time to send 10 data packets and 
		  (b) a uniform-random time in the range [1.7,2.2] RTT.
		Again, we want to highlight that PCC does not pause sending
		packets to wait for performance results, and it does not
		decide on a rate and send for a long time; packet transfer and
		measurement-control cycles are truly continuous along each MI.
		In some cases, the utility result of one MI can come
		back in the middle of another MI and the control module
		can decide to change sending rate after processing this
		result. As an optimization, PCC will immediately change
		the rate and “re-align” the current MI’s starting time with
		the time of rate change without waiting for the next MI.
	*/

	/*
	User Sending Rate

		The user is giving packets to be sent which can be translated into
		a Monitor Interval. However this means that the MI will be dynamically
		changing in duration.
		For the final sending rate we need a mimimum so that when PCC is 
		in Idle State we can still send data. So 'Idle State' will get a 
		threshold value (MPR = Minimum Packet Rate (packets/s)) before it
		can start to determine its state-transition condition.
	
	So when do we activate PCC ?

		When the requested packet rate (RPR) is > N packets in a time 
		window of [1.7,2.2].RTT we should move PCC from 'Idle State' to
		'Starting State'. When the RPR falls below this threshold we should 
		transition back to 'Idle State'.
		Also we should have a good measurement of RTT before we can
		even start executing Idle State.

	PCC rate limiter !

		Currently PCC logic has no notion of rate limiting and its logic
		is aiming to increase the rate. However rate is depending on the
		amount of packets that are actually being send. If the user is 
		sending 40 packets per second (40 * 1024 = 40 Kbyte/s) then PCC
		logic for the 'Starting State' shouldn't aggressively  increase
		the sending rate.

	*/

	/*
	PCC State Diagram

	STATE = Starting State

		PCC starts at rate 2·MSS/RTT and doubles its rate at
		each consecutive monitor interval (MI), like TCP.
		Unlike TCP, PCC does not exit this 'Starting State' because
		of a packet loss. Instead, it monitors the utility result
		of each rate doubling action.
		Only when the utility decreases, PCC exits the 'Starting State',
		returns to the previous rate which had higher utility
		(i.e., half of the rate), and enters the 'Decision Making State'.
		PCC could use other more aggressive startup strategies,
		but such tweaks could be applied to TCP as well.
	*/

	struct MI_Instance
	{
		double	mT_s, mTs_s, mTe_s;

		void	begin(double _time_s, double _interval_s, double _rtt_s, udx_seqnr _seqnr)
		{
			mT_s = _time_s;
			mTs_s = _time_s;
			mTe_s = _time_s + _interval_s;

			double pkt_snd_period_s = 0.001;

			u64 mi_packet_count = 0;
			if (((_rtt_s * 1.1) / pkt_snd_period_s) > 10)
			{
				double const rand_factor = double(rand() % 3) / 10;
				mi_packet_count = (_rtt_s * (1.5 + rand_factor)) / pkt_snd_period_s;
			}
			else
			{
				double const packets_per_min_rtt = (0.005 / pkt_snd_period_s);
				mi_packet_count = (packets_per_min_rtt < 10) ? 10 : packets_per_min_rtt;
			}

			// e.g:
			// rtt_s             = 0.005 (5 ms)
			// pkt_send_period_s = 0.001

			// if (0.005 * 1.1 / 0.001 > 10)
			// mi_packet_count   = (10 > (0.005 / 0.001)) ? 10 : (0.005 / 0.001);
			// mi_packet_count   = 10;

		}

		bool	tick(double _time_s)
		{
			if (mT_s + _time_s > mTe_s)	// Is time beyond mTe + RTT ? 
				return false;		// If so we are done!

			// Can we still receive ACKs that are part of our interval?

			return true;
		}


	};

	static double	sComputeUtility(double _time, udx_rtt* rtt, udx_perf_monitor* perf) const
	{
		double const t = (double)perf->get_total();
		double const l = (double)perf->get_loss();
		double const U = ((t - l) / _time*(1 - 1 / (1 + exp(-100 * (l / t - 0.05)))) - 1 * l / _time);
		return U;
	}

	struct MI_History
	{
		MI_Instance		m[32];
		s32				mCurrent;
	};


	/*
	STATE = Decision Making State

		Assume PCC is currently at rate r. To decide which direction 
		and amount to change its rate, PCC conducts	multiple randomized 
		controlled trials (RCTs).
		PCC takes four consecutive MIs and divides them into two pairs
		(2 MIs each). For each pair, PCC attempts a slightly higher rate
		r(1+ε) and slightly lower rate r(1−ε), each for one MI, in random
		order.  After the four consecutive trials, PCC changes
		the rate back to r and keeps aggregating SACKs until
		the Monitor generates utility value for these four trials.
		For each pair i∈1,2, PCC gets two utility measure-
		ments U+i,U−i corresponding to r.(1+ε),r.(1−ε) respectively.
		If the higher rate consistently has higher utility (U+i>U−i∀i∈{1,2}),
		then PCC adjusts its sending rate to r[new]=r.(1+ε);
		and if the lower rate consistently has higher utility then PCC picks
		r[new] = r.(1−ε). However, if the results are inconclusive, e.g.
		U+1>U−1 but U+2<U−2, PCC stays at its current rate r and re-enters
		the Decision Making State with larger experiment granularity,
		ε=ε+ε[min]. The granularity starts from ε[min] when it enters the
		decision making system for the first time and will increase up to
		ε[max] if the process continues to be inconclusive.
		This increase of granularity helps PCC avoid getting stuck due to
		noise. Unless otherwise stated, we use ε[min]=0.01 and ε[max]=0.05.
	*/
	
	
	/*
	STATE = Rate Adjusting State

		Assume the new rate after 'Decision Making State' is r[0] and dir=±1 
		is the chosen moving direction. In each MI, PCC adjusts its rate in 
		that direction faster and faster, setting the new rate r[n] as: 
			 r[n] = r[n] − 1·(1+n·ε[min]·dir).
		However, if utility falls, i.e. U(r[n])<U(r[n]−1), PCC reverts its rate 
		to r[n]−1 and moves back to the 'Decision Making State'.
	*/



	
	// --------------------------------------------------------------------------------------------
	// Generic Congestion-Control Sender
	class CC_Sender
	{
	public:
		virtual bool on_send(u32 packet_size) = 0;
	};

	// --------------------------------------------------------------------------------------------
	// Generic Congestion-Control Receiver
	class CC_Receiver
	{
	public:
		virtual void on_receive(u32 packet_size, u32 packet_seqnr, u32 ack_segnr, u8* ack_data, u32 ack_data_size) = 0;
	};

	// --------------------------------------------------------------------------------------------
	// Generic Congestion-Control Controller
	class CC_Control
	{
	public:
		virtual void on_control_update() = 0;
	};

	// --------------------------------------------------------------------------------------------
	// Generic Congestion-Control Monitor
	class CC_Monitor
	{
	public:
		virtual void on_monitor_update(u64 delta_time_us) = 0;
	};


	// --------------------------------------------------------------------------------------------
	// [PRIVATE][CPP]
	// --------------------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------------------
	// Performance oriented Congestion Control (PoCC)
	// --------------------------------------------------------------------------------------------

	class PoCC_Sender : public CC_Sender
	{
	public:
		virtual void set_send_rate(u64 send_rate_bps) = 0;
	};

	class PoCC_Control : public CC_Control
	{
	public:
		virtual void on_monitor_report(u32 monitor_nr, u32 utility) = 0;
	};

	class PoCC_Utility
	{
	public:
		virtual void compute_utility(u64 send_bytes, u64 lost_bytes, u64 RTT_us, u32& out_utility) = 0;
	};

	class PoCC_Monitor_Controller : public CC_Monitor, public CC_Receiver
	{
	public:

	};

	class PoCC : public PoCC_Sender, public PoCC_Control, public PoCC_Utility, public PoCC_Monitor_Controller
	{
	public:
		virtual bool		on_send(u32 packet_size);
		virtual void		set_send_rate(u64 send_rate_bytes_per_second);

		virtual void		on_receive(u32 packet_size, u32 packet_seqnr, u32 ack_segnr, u8* ack_data, u32 ack_data_size);

		virtual void		on_control_update();
		virtual void		on_monitor_report(u32 monitor_nr, u32 utility);

		virtual void		compute_utility(u64 transferred_bytes, u64 lost_packets, u64 time_period_us, u64 RTT_us, u32& out_utility);

		virtual void		process();

	protected:
		virtual u32			on_monitor_start(u64 interval_us);
		virtual void		on_monitor_update(u64 delta_time_us);
	};

	void PoCC::compute_utility(u64 transferred_bytes, u64 lost_packets, u64 time_period_us, u64 RTT_us, u32& out_utility)
	{
		double const time = (double)time_period_us / 1000000.0;
		double const L = (double)lost_packets / time;
		double const T = (double)transferred_bytes / time;

		out_utility = ((T - L) / time * (1.0 - 1.0 / (1.0 + exp(-100.0 * (L / T - 0.05)))) - 1.0 * L / time);;
	}


	void	PoCC::process()
	{

	}

	class PoCC_Monitor_Controller
	{
		enum eccsettings
		{
			MAX_MONITOR_NUMBER = 100,
			MAX_COUNTINOUS_GUESS = 5,
			NUMBER_OF_PROBE = 4,
		};

		enum eccstate
		{
			CC_STATE_STARTING_PHASE = 1,
			CC_STATE_GUESSING = 2,
			CC_STATE_MOVING_PHASE = 4,
			CC_STATE_MOVING_PHASE_INITIAL = 8,
			CC_STATE_RECORDING_GUESS_RESULT = 16,
			CC_STATE_ALL = 0xffffffff
		};

		u32				m_state;

		double			m_start_rate_array[MAX_MONITOR_NUMBER];
		s32				m_current_monitor;
		s32				m_monitor_bucket[NUMBER_OF_PROBE];
		double			m_utility_bucket[NUMBER_OF_PROBE];

		double			m_GRANULARITY;
		double			m_rate_bucket[NUMBER_OF_PROBE];

	public:
		void	initialize()
		{
			for (int i = 0; i < MAX_MONITOR_NUMBER; i++)
				m_start_rate_array[i] = 0;

			m_state = CC_STATE_STARTING_PHASE;
			m_previous_rate = 1.0;

			m_GRANULARITY = 0.01;
		}

		void	on_monitor_update(u64 current_time_us)
		{
		}

		u32 m_guess_time = 0;		// Seems to be a counter tracking the number of PROBES/GUESSES
		u32 m_continous_guess_count = 0;

		void	on_monitor_start(u64 current_time_us)
		{
			if ((m_state & CC_STATE_STARTING_PHASE) == CC_STATE_STARTING_PHASE)
			{
				// Double the sending rate
				m_start_rate_array[m_current_monitor] = m_previous_rate * 2.0;
				m_previous_rate = m_start_rate_array[m_current_monitor];
				m_current_rate  = m_previous_rate;
			}
			else if ((m_state & CC_STATE_GUESSING) == CC_STATE_GUESSING)
			{
				if (m_guess_time == 0 && m_continous_guess_count == MAX_COUNTINOUS_GUESS)
				{
					m_continous_guess_count = 0;
				}

				if (m_guess_time == 0)
				{
					m_state = m_state | CC_STATE_RECORDING_GUESS_RESULT;
					m_continous_guess_count++;

					int rand_dir;

					for (int i = 0; i < NUMBER_OF_PROBE; i++)
					{
						rand_dir = (rand() % 2 * 2 - 1);
						m_rate_bucket[i] = m_current_rate + rand_dir*m_continous_guess_count*m_GRANULARITY*m_current_rate;
						m_rate_bucket[++i] = m_current_rate - rand_dir*m_continous_guess_count*m_GRANULARITY*m_current_rate;
					}

					for (int i = 0; i < NUMBER_OF_PROBE; i++)
					{
						m_monitor_bucket[i] = (m_current_monitor + i) % MAX_MONITOR_NUMBER;
					}

				}

				m_current_rate = m_rate_bucket[m_guess_time];
				m_guess_time++;

				//TODO:Here the sender stopped at a particular rate
				if (m_guess_time == NUMBER_OF_PROBE)
				{
					m_state = m_state & ~CC_STATE_GUESSING;
					m_guess_time = 0;
				}
			}

		}

		double m_total;
		double m_loss;
		double m_rtt;

		double m_current_rate;
		double m_previous_rate;

		double m_current_utility;
		double m_previous_utility;
		double m_start_previous_utility;

		s32 m_current;
		s32 m_target_monitor;
		s32 m_start_previous_monitor;

		s32 m_endMonitor;
		s32 m_change_direction;
		s32 m_recorded_number;

		s32 m_change_intense;
		s32 m_guess_time;

		void	on_monitor_end(u64 interval_time_us)
		{
			double utility;
			double t = m_total;
			double l = m_loss;
			double time = interval_time_us;

			if (l < 0)
				l = 0;

			if (m_rtt == 0)
			{
				// "RTT cannot be 0!!!"
			}

			utility = ((t - l) / time*(1 - 1 / (1 + exp(-100 * (l / t - 0.05)))) - 1 * l / time);

			if (m_endMonitor == 0 && ((m_state & CC_STATE_STARTING_PHASE) == CC_STATE_STARTING_PHASE))
				utility /= 2;

			if ((m_state & CC_STATE_STARTING_PHASE) == CC_STATE_STARTING_PHASE)
			{
				if (m_endMonitor - 1 > m_start_previous_monitor)
				{
					if (m_start_previous_monitor == -1)
					{
						// "fall back to guess mode"
						m_state = m_state & ~CC_STATE_STARTING_PHASE;
						m_state = m_state | CC_STATE_GUESSING;
						m_current_rate = m_start_rate_array[0];
						return;
					}
					else
					{
						// "exit because of loss"
						// "in monitor" << m_start_previous_monitor
						// "fall back to due to loss" << m_start_rate_array[m_start_previous_monitor]
						m_state = m_state & ~CC_STATE_STARTING_PHASE;
						m_state = m_state | CC_STATE_GUESSING;
						m_current_rate = m_start_rate_array[m_start_previous_monitor];
						return;
					}
				}
				if (m_start_previous_utility < utility)
				{
					// "moving forward"
					// do nothing
					m_start_previous_utility = utility;
					m_start_previous_monitor = m_endMonitor;
					return;
				}
				else
				{
					m_state = m_state & ~CC_STATE_STARTING_PHASE;
					m_state = m_state | CC_STATE_GUESSING;

					m_current_rate = m_start_rate_array[m_start_previous_monitor];
					// "fall back to " << m_start_rate_array[m_start_previous_monitor]
					m_previous_rate = m_current_rate;
					return;
				}

			}

			if ((m_state & CC_STATE_RECORDING_GUESS_RESULT) == CC_STATE_RECORDING_GUESS_RESULT)
			{
				for (int i = 0; i < NUMBER_OF_PROBE; i++)
				{
					if (m_endMonitor == m_monitor_bucket[i])
					{
						m_recorded_number++;
						m_utility_bucket[i] = utility;
					}
				}

				// TODO:
				//  to let the sender go back to the current sending rate,
				//   one way is to let the decision maker stop for another
				//   monitor period, which might not be a good option,
				//   let's try this first
				if (m_recorded_number == NUMBER_OF_PROBE)
				{
					m_recorded_number = 0;
					double decision = 0;
					for (int i = 0; i < NUMBER_OF_PROBE; i++)
					{
						if (((m_utility_bucket[i] > m_utility_bucket[i + 1]) && (m_rate_bucket[i] > m_rate_bucket[i + 1])) || ((m_utility_bucket[i] < m_utility_bucket[i + 1]) && (m_rate_bucket[i] < m_rate_bucket[i + 1])))
							decision += 1;
						else
							decision -= 1;
						i++;
					}

					if (decision == 0)
					{
						m_state = m_state | CC_STATE_GUESSING;
						m_state = m_state & ~CC_STATE_RECORDING_GUESS_RESULT;
						// "no decision"
					}
					else
					{
						m_state = m_state & ~CC_STATE_RECORDING_GUESS_RESULT;
						m_state = m_state | CC_STATE_MOVING_PHASE_INITIAL;

						m_change_direction = decision > 0 ? 1 : -1;
						// "change to the direction of" << m_change_direction
						m_target_monitor = (m_current + 1) % MAX_MONITOR_NUMBER;
						m_change_intense = 1;
						s32 change_amount = (m_continous_guess_count / 2 + 1)*m_change_intense*m_change_direction * m_GRANULARITY * m_current_rate;
						m_previous_utility = 0;
						m_continous_guess_count--;
						m_continous_guess_count = 0;

						if (m_continous_guess_count < 0)
							m_continous_guess_count = 0;

						m_previous_rate = m_current_rate;
						m_current_rate = m_current_rate + change_amount;
					}
				}
			}

			if (((m_state & CC_STATE_MOVING_PHASE_INITIAL) == CC_STATE_MOVING_PHASE_INITIAL) && m_endMonitor == m_target_monitor)
			{
				if (m_current_rate > (t * 12 / time / 1000 + 30) && m_current_rate > 200)
				{
					m_state = m_state | CC_STATE_GUESSING;
					m_state = m_state & ~CC_STATE_RECORDING_GUESS_RESULT;
					m_state = m_state & ~CC_STATE_MOVING_PHASE;
					m_state = m_state & ~CC_STATE_MOVING_PHASE_INITIAL;

					m_current_rate = t * 12 / time / 1000;
					m_change_direction = 0;
					m_change_intense = 1;
					m_guess_time = 0;
					m_continous_guess_count = 0;
					m_recorded_number = 0;
					// "system udp call speed limiting, resyncing rate"
					return;
				}

				// "first time moving"

				m_target_monitor = (m_current + 1) % MAX_MONITOR_NUMBER;
				m_previous_rate = m_current_rate;
				m_previous_utility = utility;
				m_change_intense += 1;
				s32 change_amount = m_change_intense * m_GRANULARITY * m_current_rate * m_change_direction;
				m_current_rate = m_current_rate + change_amount;

				m_state = m_state & ~CC_STATE_MOVING_PHASE_INITIAL;
				m_state = m_state | CC_STATE_MOVING_PHASE;
			}

			if (((m_state & CC_STATE_MOVING_PHASE) == CC_STATE_MOVING_PHASE) && m_endMonitor == m_target_monitor)
			{
				if (m_current_rate > (t * 12 / time / 1000 + 30) && m_current_rate > 200)
				{
					m_state = m_state | CC_STATE_GUESSING;
					m_state = m_state & ~CC_STATE_RECORDING_GUESS_RESULT;
					m_state = m_state & ~CC_STATE_MOVING_PHASE;
					m_state = m_state & ~CC_STATE_MOVING_PHASE_INITIAL;


					m_current_rate = t * 12 / time / 1000;

					m_change_direction = 0;
					m_change_intense = 1;
					m_guess_time = 0;
					m_continous_guess_count = 0;
					m_recorded_number = 0;

					// "system udp call speed limiting, resyncing rate"
					return;
				}

				// "moving faster"
				m_current_utility = utility;
				if (m_current_utility > m_previous_utility)
				{
					m_target_monitor = (m_current + 1) % MAX_MONITOR_NUMBER;
					m_change_intense += 1;
					m_previous_utility = m_current_utility;
					m_previous_rate = m_current_rate;
					s32 change_amount = m_change_intense * m_GRANULARITY * m_current_rate * m_change_direction;
					m_current_rate = m_current_rate + change_amount;

				}
				else
				{
					m_state = m_state | CC_STATE_GUESSING;
					m_state = m_state & ~CC_STATE_MOVING_PHASE;

					m_previous_utility = m_current_utility;
					m_previous_rate = m_current_rate;
					s32 change_amount = m_change_intense * m_GRANULARITY * m_current_rate * m_change_direction;
					m_current_rate = m_current_rate - change_amount;

				}
			}
		}
	};


}
