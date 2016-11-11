#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-packet.h"
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


	1 Gbps ~= 93622 packets per second (average size of packet == 1400 bytes)

	UDX Goal:

	- udx::update(packets_to_send, packets_received) to be able to send ~100.000
	  packets/s as well as receive ~100.000 packets/s. This amount of packets
	  equals ~100 MiB/s.

	- udx:update should be called 

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
		double time = (double)time_period_us / 1000000.0;
		double L = (double)lost_packets / time;
		double T = (double)transferred_bytes / time;

		double utility = ((T - L) / time * (1.0 - 1.0 / (1.0 + exp(-100.0 * (L / T - 0.05)))) - 1.0 * L / time);

		out_utility = utility;
	}


	void	PoCC::process()
	{

	}

	class PoCC_Monitor_Controller
	{
		enum eccsettings
		{
			MAX_MONITOR_NUMBER = 100,
			MAX_COUNTINOUS_SEND = 1,
			MAX_COUNTINOUS_GUESS = 5,
			NUMBER_OF_PROBE = 4,
		};

		enum eccstate
		{
			CC_STATE_STARTING_PHASE,
			CC_STATE_GUESSING,
			CC_STATE_CONTINUES_SEND,
			CC_STATE_RECORDING_GUESS_RESULT,
		};

		eccstate		m_state;

		double			m_start_rate_array[MAX_MONITOR_NUMBER];
		s32				m_current_monitor;
		s32				m_monitor_bucket[NUMBER_OF_PROBE];


		double			m_previous_rate;

		double			m_current_rate;
		double			m_GRANULARITY;
		double			m_rate_bucket[NUMBER_OF_PROBE];

		void			setRate(double rate)
		{
			m_current_rate = rate;
		}

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

		void	on_monitor_start(u64 current_time_us)
		{
			if (m_state == CC_STATE_STARTING_PHASE)
			{
				m_start_rate_array[m_current_monitor] = m_previous_rate * 2.0;
				m_previous_rate = m_start_rate_array[m_current_monitor];
				setRate(m_previous_rate);
			}

			u32 guess_time = 0;		// Seems to be a counter tracking the number of PROBES/GUESSES
			u32 continous_guess_count = 0;

			if (m_state == CC_STATE_GUESSING)
			{
				if (guess_time == 0 && continous_guess_count == MAX_COUNTINOUS_GUESS)
				{
					continous_guess_count = 0;
				}

				if (guess_time == 0)
				{
					m_state = CC_STATE_RECORDING_GUESS_RESULT;
					continous_guess_count++;

					int rand_dir;

					for (int i = 0; i < NUMBER_OF_PROBE; i++)
					{
						rand_dir = (rand() % 2 * 2 - 1);
						m_rate_bucket[i] = m_current_rate + rand_dir*continous_guess_count*m_GRANULARITY*m_current_rate;
						m_rate_bucket[++i] = m_current_rate - rand_dir*continous_guess_count*m_GRANULARITY*m_current_rate;
					}

					for (int i = 0; i < NUMBER_OF_PROBE; i++)
					{
						m_monitor_bucket[i] = (m_current_monitor + i) % MAX_MONITOR_NUMBER;
					}

				}

				setRate(m_rate_bucket[guess_time]);
				guess_time++;

				//TODO:Here the sender stopped at a particular rate
				if (guess_time == NUMBER_OF_PROBE)
				{
					m_make_guess = 0;
					guess_time = 0;
				}
			}

			s32 m_continous_send_count = 0;
			if (m_state == CC_STATE_CONTINUES_SEND)
			{
				if (m_continous_send_count == 1)
				{
					setRate(m_current_rate);
				}

				if (m_continous_send_count < MAX_COUNTINOUS_SEND)
				{
					m_continous_send_count++;
				}
				else
				{
					m_continous_send = 0;
					m_continous_send_count = 0;
					continous_guess_count = 0;

					m_state = CC_STATE_GUESSING;
				}
			}
		}

		void	on_monitor_end(u64 current_time_us)
		{
			double utility;
			double t = total;
			double l = loss;
			int random_direciton;

			if (l < 0)
				l = 0;

			if (rtt == 0)
			{
				// "RTT cannot be 0!!!"
			}

			if (previous_rtt == 0)
				previous_rtt = rtt;

			utility = ((t - l) / time*(1 - 1 / (1 + exp(-100 * (l / t - 0.05)))) - 1 * l / time);

			previous_rtt = rtt;
			if (endMonitor == 0 && starting_phase)
				utility /= 2;

			if (starting_phase)
			{
				if (endMonitor - 1 > start_previous_monitor)
				{
					if (start_previous_monitor == -1)
					{
						// "fall back to guess mode"
						starting_phase = 0;
						make_guess = 1;
						setRate(start_rate_array[0]);
						current_rate = start_rate_array[0];
						return;
					}
					else
					{
						// "exit because of loss"
						// "in monitor" << start_previous_monitor
						// "fall back to due to loss" << start_rate_array[start_previous_monitor]
						starting_phase = 0;
						make_guess = 1;
						setRate(start_rate_array[start_previous_monitor]);
						current_rate = start_rate_array[start_previous_monitor];
						return;
					}
				}
				if (start_previous_utility < utility)
				{
					// "moving forward"
					// do nothing
					start_previous_utility = utility;
					start_previous_monitor = endMonitor;
					return;
				}
				else
				{
					starting_phase = 0;
					make_guess = 1;
					setRate(start_rate_array[start_previous_monitor]);
					current_rate = start_rate_array[start_previous_monitor];
					// "fall back to " << start_rate_array[start_previous_monitor]
					previous_rate = current_rate;
					return;
				}

			}

			if (recording_guess_result)
			{
				for (int i = 0; i < NUMBER_OF_PROBE; i++)
				{
					if (endMonitor == monitor_bucket[i])
					{
						recorded_number++;
						utility_bucket[i] = utility;
					}
				}

				// TODO:
				//   to let the sender go back to the current sending rate,
				//   one way is to let the decision maker stop for another
				//   monitor period, which might not be a good option,
				//   let's try this first
				if (recorded_number == NUMBER_OF_PROBE)
				{
					recorded_number = 0;
					double decision = 0;
					for (int i = 0; i < NUMBER_OF_PROBE; i++)
					{
						if (((utility_bucket[i] > utility_bucket[i + 1]) && (rate_bucket[i] > rate_bucket[i + 1])) || ((utility_bucket[i] < utility_bucket[i + 1]) && (rate_bucket[i] < rate_bucket[i + 1])))
							decision += 1;
						else
							decision -= 1;
						i++;
					}

					if (decision == 0)
					{
						make_guess = 1;
						recording_guess_result = 0;
						// "no decision"
					}
					else
					{
						change_direction = decision > 0 ? 1 : -1;
						// "change to the direction of" << change_direction
						recording_guess_result = 0;
						target_monitor = (current + 1) % MAX_MONITOR_NUMBER;
						moving_phase_initial = 1;
						change_intense = 1;
						change_amount = (continous_guess_count / 2 + 1)*change_intense*change_direction * GRANULARITY * current_rate;
						previous_utility = 0;
						continous_guess_count--; continous_guess_count = 0;

						if (continous_guess_count < 0)
							continous_guess_count = 0;

						previous_rate = current_rate;
						current_rate = current_rate + change_amount;
						target_monitor = (current + 1) % MAX_MONITOR_NUMBER;
						setRate(current_rate);
					}
				}



			}

			if (moving_phase_initial && endMonitor == target_monitor)
			{
				if (current_rate > (t * 12 / time / 1000 + 30) && current_rate > 200)
				{
					current_rate = t * 12 / time / 1000;
					make_guess = 1;
					moving_phase = 0;
					moving_phase_initial = 0;
					change_direction = 0;
					change_intense = 1;
					guess_time = 0;
					continous_guess_count = 0;
					continous_send = 0;
					continous_send_count = 0;
					recording_guess_result = 0;
					recorded_number = 0;
					setRate(current_rate);
					// "system udp call speed limiting, resyncing rate"
					return;
				}

				// "first time moving"

				target_monitor = (current + 1) % MAX_MONITOR_NUMBER;
				previous_rate = current_rate;
				previous_utility = utility;
				change_intense += 1;
				change_amount = change_intense * m_GRANULARITY * current_rate * change_direction;
				current_rate = current_rate + change_amount;
				setRate(current_rate);
				moving_phase_initial = 0;
				moving_phase = 1;
			}

			if (moving_phase && endMonitor == target_monitor)
			{
				if (current_rate > (t * 12 / time / 1000 + 30) && current_rate > 200)
				{
					current_rate = t * 12 / time / 1000;
					make_guess = 1;
					moving_phase = 0;
					moving_phase_initial = 0;
					change_direction = 0;
					change_intense = 1;
					guess_time = 0;
					continous_guess_count = 0;
					continous_send = 0;
					continous_send_count = 0;
					recording_guess_result = 0;
					recorded_number = 0;
					setRate(current_rate);
					// "system udp call speed limiting, resyncing rate"
					return;
				}

				// "moving faster"
				current_utility = utility;
				if (current_utility > previous_utility)
				{
					target_monitor = (current + 1) % MAX_MONITOR_NUMBER;
					change_intense += 1;
					previous_utility = current_utility;
					previous_rate = current_rate;
					change_amount = change_intense * m_GRANULARITY * current_rate * change_direction;
					current_rate = current_rate + change_amount;
					setRate(current_rate);
				}
				else
				{
					moving_phase = 0;
					make_guess = 1;
					//change_intense+=1;
					previous_utility = current_utility;
					previous_rate = current_rate;
					change_amount = change_intense * GRANULARITY * current_rate * change_direction;
					current_rate = current_rate - change_amount;
					setRate(current_rate);
				}
			}
		}
	};


}
