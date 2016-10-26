#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\private\x_sockets.h"

#include <chrono>

namespace xcore
{
	static u64 get_time_us()
	{
		std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
		std::chrono::nanoseconds ns = time.time_since_epoch;
		return (u64)ns.count();
	}

	class udx_address
	{
	public:
		u32						m_index;
		u32						m_hash[4];
		u32						m_data[16];
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_socket
	{
	public:
		virtual udx_address*	get_key() const = 0;

		virtual udx_message		alloc_msg(u32 size) = 0;
		virtual void			free_msg(udx_message& msg) = 0;

		virtual udx_address*	connect(const char* address) = 0;
		virtual bool			disconnect(udx_address*) = 0;

		virtual void			send(udx_message& msg, udx_address* to) = 0;
		virtual bool			recv(udx_message& msg, udx_address*& from) = 0;

		// Process time-outs and deal with re-transmitting, disconnecting etc..
		virtual void			process(u64 delta_time_us) = 0;
	};


	struct udx_message
	{
		void*			data_ptr;
		u32				data_size;
	};

	struct udx_packet	// align(8)
	{
		// Book-keeping
		u32				m_magic_marker;
		u32				m_flags : 8;
		u32				m_transmissions : 23;
		u32				m_need_resend : 1;
		u32				m_size_in_bytes : 12;
		u32				m_ref_counter : 20;
		u64				m_timestamp_us;
		udx_address*	m_to_or_from;

		// Packet-header (12) followed by payload in memory
		u64				m_hdr_pkt_type : 4;
		u64				m_hdr_pkt_seqnr : 24;
		u64				m_hdr_ack_seqnr : 24;
		u64				m_hdr_ack_info : 12;
	};

	class udx_packet_send_queue
	{
	public:
		u32				size() const;

		void			add(u32 seqnr, udx_packet* pkt);
		udx_packet*		get(u32 seqnr) const;

		u32				remove_ackd();

	private:
		void			shift(u32 n);
	};

	class udx_packet_recv_queue
	{
	public:
		void			insert(u32 seqnr, udx_packet* pkt);
		u32				size() const;
		udx_packet*		remove();

		void			collect_acks(udx_packet*);	// Insert ACK data into a packet

	private:
		void			shift(u32 n);
		udx_packet*		get(u32 seqnr) const;
	};


	// --------------------------------------------------------------------------------------------
	// [PUBLIC] allocator interface
	class udx_allocator
	{
	public:
		virtual void*		alloc(u32 _size) = 0;
		virtual void		dealloc(void*) = 0;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] udx registry of 'address' to 'socket'
	class udx_registry
	{
	public:
		virtual udx_address*	find(void const* data, u32 size) const = 0;
		virtual udx_address*	add(void const* data, u32 size) = 0;
		virtual udx_socket*		find(udx_address* key) = 0;
		virtual void			add(udx_address* k, udx_socket* v) = 0;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] udx registry of 'address' to 'socket'
	class udx_registry_imp : public udx_registry
	{
	public:
		virtual void			init(udx_allocator* allocator)
		{
			s32 num_buckets = 1024;
			m_buckets = (bucket*)allocator->alloc(num_buckets * sizeof(bucket));
			for (s32 i = 0; i < num_buckets; ++i)
			{
				m_buckets[i].init(allocator, 2);
			}
		}

		virtual udx_address*	find(void const* data, u32 size) const
		{
			u32 hash[5];
			data_to_hash(data, size, hash);
			
			udx_socket* s = find_by_hash(hash);
			if (s == NULL)
				return NULL;
			return s->get_key();
		}

		virtual udx_address*	add(void const* data, u32 size) 
		{
			u32 hash[5];
			data_to_hash(data, size, hash);

			udx_address* a = (udx_address*)m_allocator->alloc(sizeof(udx_address));
			memcpy(a->m_data, data, size);
			memcpy(a->m_hash, hash, sizeof(hash));
			a->m_index = 0;
			return a;
		}

		virtual udx_socket*		find(udx_address* k)
		{
			udx_socket* s = find_by_key(k);
			return s;
		}

		virtual void			add(udx_address* k, udx_socket* v)
		{
			u32 hash = k->m_hash[0];
			u32 bidx = hash_to_bucket_index(hash);
			m_buckets[bidx].add(m_allocator, k, v);
		}

	protected:
		static inline void		data_to_hash(void const* data, u32 size, u32* out_hash)
		{
			SHA1_CTX ctx;
			sha1_init(&ctx);
			sha1_update(&ctx, (const u8*)data, size);
			sha1_final(&ctx, (u8*)out_hash);
		}

		static inline u32		hash_to_bucket_index(u32 hash)
		{	// Take 10 bits to be our index in the bucket array
			return (hash & 0x3FF0) >> 4;
		}
		
		udx_socket*				find_by_key(udx_address* a) const
		{
			u32 bidx = hash_to_bucket_index(a->m_hash[0]);
			udx_socket* s = m_buckets[bidx].find(a);
			return s;
		}

		udx_socket*				find_by_hash(u32 hash[5]) const
		{
			u32 bidx = hash_to_bucket_index(hash[0]);
			udx_socket* s = m_buckets[bidx].find_by_hash(hash);
			return s;
		}

	protected:
		udx_allocator*			m_allocator;

		struct bucket
		{
			u32				m_size;
			u32				m_max;
			udx_socket**	m_values;

			void			init(udx_allocator* a, u32 size)
			{
				m_size = 0;
				m_max = size;
				m_values = (udx_socket**)a->alloc(size * sizeof(void*));
				for (u32 i = 0; i < size; ++i)
					m_values[i] = NULL;
			}

			void			add(udx_allocator* a, udx_address* k, udx_socket* v)
			{
				for (u32 i = 0; i < m_size; i++)
				{
					if (m_values[i] == v)
						return;
				}
				if (m_size == m_max)
				{
					m_max = m_max * 2;
					udx_socket** values = (udx_socket**)a->alloc(m_max * sizeof(void*));
					memcpy(values, m_values, m_size * sizeof(void*));
					a->dealloc(m_values);
					m_values = values;
				}
				m_values[m_size++] = v;
			}

			udx_socket*		find(udx_address* k)
			{
				for (u32 i = 0; i < m_size; i++)
				{
					if (m_values[i]->get_key() == k)
						return m_values[i];
				}
				return NULL;
			}

			udx_socket*		find_by_hash(u32 hash[5])
			{
				for (u32 i = 0; i < m_size; i++)
				{
					udx_address* a = m_values[i]->get_key();
					if (memcmp(a->m_hash, hash, sizeof(hash)) == 0)
						return m_values[i];
				}
				return NULL;
			}
		};
		bucket*					m_buckets;
	};


	// --------------------------------------------------------------------------------------------
	// [PRIVATE] API
	class udp_socket
	{
	public:
		virtual void		send(udx_packet* pkt) = 0;
		virtual void		recv(udx_packet*& pkt) = 0;
	};


	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class CC_Filter
	{
	public:
		virtual void	init(u64* window, u32 size);
		virtual u64		add(u64 value) = 0;
		virtual u64		get() const = 0;
	};

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] IMP
	class CC_Filter_SMA : public CC_Filter
	{
	public:
		virtual void	init(u64* window, u32 size)
		{
			m_window = window;
			m_average = 0;
			m_count = 0;
			m_size = size;
		}

		virtual u64		add(u64 value)
		{
			m_average -= m_window[m_index];
			m_window[m_index++] = value;
			m_average += value;
			m_index += 1;
			m_count += 1;
			if (m_count > m_size)
				m_count = m_size;

			return get();
		}

		u32				get() const
		{
			return m_average / m_count;
		}

	protected:
		u64				m_average;
		u64				m_index;
		u64				m_count;

		u32				m_size;
		u64				*m_window;
	};


	// --------------------------------------------------------------------------------------------
	// [PRIVATE] IMPLEMENTATION
	class udx_socket_imp : public udx_socket
	{
	public:
		udx_socket_imp(udx_allocator* allocator, udx_allocator* msg_allocator);

		virtual udx_message		alloc_msg(u32 size);
		virtual void			free_msg(udx_message& msg);

		virtual udx_address*	connect(const char* address);
		virtual bool			disconnect(udx_address*);

		virtual void			send(udx_message& msg, udx_address* to);
		virtual bool			recv(udx_message& msg, udx_address*& from);

		// Process time-outs and deal with re-transmitting, disconnecting etc..
		virtual void			process(u64 delta_time_us);

	protected:
		udx_allocator*			m_allocator;
		udx_allocator*			m_pkt_allocator;

		xnet::udpsocket*		m_udp_socket;

		u32						m_max_sockets;
		udx_socket*				m_all_sockets;
		u32						m_num_free_sockets;
		u32*					m_free_socket_list;

		udx_registry*			m_address_to_socket;
	};



	// --------------------------------------------------------------------------------------------
	// Generic Congestion-Control Sender
	class CC_Sender
	{
	public:
		virtual bool		on_send(u32 packet_size) = 0;
	};

	// --------------------------------------------------------------------------------------------
	// Generic Congestion-Control Receiver
	class CC_Receiver
	{
	public:
		virtual void		on_receive(u32 packet_size, u32 packet_seqnr, u32 ack_segnr, u8* ack_data, u32 ack_data_size) = 0;
	};

	// --------------------------------------------------------------------------------------------
	// Generic Congestion-Control Controller
	class CC_Control
	{
	public:
		virtual void		on_control_update() = 0;
	};

	// --------------------------------------------------------------------------------------------
	// Generic Congestion-Control Monitor
	class CC_Monitor
	{
	public:
		virtual void		on_monitor_update(u64 delta_time_us) = 0;
	};

	
	// --------------------------------------------------------------------------------------------
	// Generic RTO controller
	class CC_RTT
	{
	public:
		virtual void		on_send(u32 packet_seqnr) = 0;
		virtual void		on_receive(u32 ack_segnr, u8* ack_data, u32 ack_data_size) = 0;

		virtual s64			get_rtt_us() const = 0;
		virtual s64			get_rto_us() const = 0;
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
		virtual void		set_send_rate(u64 send_rate_bps) = 0;
	};

	class PoCC_Control : public CC_Control
	{
	public:
		virtual void		on_monitor_report(u32 monitor_nr, u32 utility) = 0;
	};

	class PoCC_Utility
	{
	public:
		virtual void		compute_utility(u64 send_bytes, u64 lost_bytes, u64 RTT_us, u32& out_utility) = 0;
	};

	class PoCC_Monitor_Controller : public CC_Monitor, public CC_Receiver
	{
	public:

	};

	class PoCC : public PoCC_Sender, public PoCC_Control, public PoCC_Utility, public PoCC_Monitor_Controller
	{
	public:
		virtual bool		on_send(u32 packet_size) = 0;
		virtual void		set_send_rate(u64 send_rate_bytes_per_second);

		virtual void		on_receive(u32 packet_size, u32 packet_seqnr, u32 ack_segnr, u8* ack_data, u32 ack_data_size) = 0;

		virtual void		on_control_update();
		virtual void		on_monitor_report(u32 monitor_nr, u32 utility) = 0;

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
		// Required:
		// - Allocator to allocate memory for packets to receive

		// Drain the UDP socket for incoming packets
		//  - For every packet add it to the associated udx socket
		//    If the udx socket doesn't exist create it and verify that
		//    the packet is a SYN packet
		// For every 'active' udx socket 
		//  - check time-outs and react to them (e.g. ACK, RTO)
		//  - update CC
		// Iterate over all 'active' udx sockets and send their queued packets
		// For every 'active' udx socket see if it has received packets to pass on to
		//  the user layer.
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


























































