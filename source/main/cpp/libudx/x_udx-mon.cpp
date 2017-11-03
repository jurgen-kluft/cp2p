#include "xbase/x_target.h"
#include "xp2p/x_sha1.h"

#include "xp2p\libudx/x_udx.h"
#include "xp2p\libudx/x_udx-mon.h"
#include "xp2p\libudx/x_udx-rtt.h"
#include "xp2p\libudx/x_udx-ack.h"
#include "xp2p\libudx/x_udx-seqnr.h"
#include "xp2p\libudx/x_udx-packetqueue.h"

#include <chrono>


namespace xcore
{
	/*
	PCC Monitor

		Compute total througput as well as loss during a monitor interval (MI).

	*/

	class udx_monitor_pcc : public udx_monitor
	{
	public:
		virtual void	reset();

		virtual bool	on_send(double _time_s, udx_seqnr pkt_segnr);
		virtual bool	on_recv(double _time_s, udx_seqnr ack_segnr);
		virtual bool	on_loss(double _time_s, udx_seqnr pkk_segnr);

		virtual s64		get_loss_rate() const;
		virtual s64		get_total_rate() const;
		
	protected:

	};

	/*
	PCC Performance Monitor

		Compute rate at which the sender should operate by executing PCC logic.

		We are executing monitors sequentially to find the best rate at which we
		should send packets. Every monitor keeps track of throughput and loss which
		result in a utility value (0.0 - 1.0).

		The performance monitor is using this information to compute the best most
		efficient rate for this connection.

	*/

	class udx_perf_monitor_pcc : public udx_perf_monitor
	{
	public:
		virtual bool	on_send(double _time_s, udx_seqnr pkt_segnr);
		virtual bool	on_recv(double _time_s, udx_seqnr ack_segnr);
		virtual bool	on_loss(double _time_s, udx_seqnr pkk_segnr);

		virtual s64		get_rate() const;

		udx_monitor*	m_monitor[32];
		double			m_monitor_rate[32];

		s32				m_current_monitor;

		s32				m_state;

	};


	udx_perf_monitor*	gCreateDefaultPerformanceMonitor(udx_alloc* _allocator)
	{
		return NULL;
	}
}
