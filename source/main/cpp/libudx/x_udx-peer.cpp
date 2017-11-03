#include "xbase\x_target.h"
#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-ack.h"
#include "xp2p\libudx\x_udx-address.h"
#include "xp2p\libudx\x_udx-alloc.h"
#include "xp2p\libudx\x_udx-message.h"
#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-peer.h"
#include "xp2p\libudx\x_udx-registry.h"
#include "xp2p\libudx\x_udx-seqnr.h"
#include "xp2p\libudx\x_udx-time.h"
#include "xp2p\libudx\x_udx-rtt.h"
#include "xp2p\libudx\x_udx-rto_timer.h"
#include "xp2p\libudx\x_udx-udp.h"

namespace xcore
{
	struct udx_peer_queues
	{
		udx_packets_lqueue* out;		// Outgoing queue, user wants these packets to be send
		udx_packets_squeue* in;			// Incoming queue, packets that have been read from socket
		udx_packets_lqueue* snd;		// Send queue, to be written to socket
		udx_packets_squeue* ack;		// In-flight queue, waiting to be ACK-ed
		udx_packets_lqueue* gc;			// Garbage queue, packets that have been succesfully send
	};

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] IMPLEMENTATION OF PEER (P2P CONNECTION)
	// 
	// Functionality of a peer:
	//   - Receive and Send packets according to the Congestion-Control module
	//   - Process ACK data and apply it to our in-flight queue
	//   - Make and send ACK data back to remote peer for received packets
	//   - Release incoming (in-order) messages to user
	//   - Deallocate in-flight packets that have been ACK-ed
	//   - Compute RTT and RTO in real-time
	//   - Re-send any in-flight packets that are not ACK-ed and older than RTO 
	// 
	// When DISCONNECTED:
	//   - When receiving a SYN packet change state to CONNECTED
	//     Note: After verifying the credentials (public key, private key)
	// 
	// When CONNECTED and asked to disconnect or receiving a FIN packet:
	//   - Change state to DISCONNECTING
	//   - Do not accept any more outgoing packets from user 
	//   - Create FIN packet and add to outgoing queue
	//   - Keep sending remaining packets in outgoing queue
	//   - Accept incoming packets
	//   - Wait for in-flight queue to be empty (ACK-ed)
	//   - When outgoing and in-flight queues are empty change state to DISCONNECTED
	//
	// Future Features:
	//   - Encryption of the header
	//   - 
	 

	class udx_socket_peer : public udx_peer
	{
	public:
		udx_socket_peer(udx_alloc* _allocator, udx_alloc* _msg_allocator, udx_address* _address);

		virtual udx_address*	get_address() const;

		virtual bool 			connect();
		virtual bool 			disconnect();
		virtual bool 			is_connected() const;

		virtual void 			send(udx_packet*) = 0;
		virtual void			received(udx_packet*) = 0;

		virtual void			process_transmit(udx_packet_writer* writer) = 0;
		virtual void			collect_garbage(udx_packet_writer* writer) = 0;

	protected:
		void					push_incoming(udx_packet * packet);
		bool					pop_incoming(udx_packet *& packet);

		void					push_outgoing(udx_packet * packet);
		bool					peek_outgoing(udx_packet *& packet);
		bool					pop_outgoing(udx_packet *& packet);

		void					push_tosend(udx_packet * packet);
		void					push_tosend_now(udx_packet * packet);
		bool					peek_tosend(udx_packet *& packet);
		bool					pop_tosend(udx_packet *& packet);

		void					push_inflight(udx_packet * packet);
		bool					peek_inflight(udx_packet *& packet);
		bool					pop_inflight(udx_packet *& packet);

		void					push_gc(udx_packet * packet);
		bool					pop_gc(udx_packet *& packet);

		void					process_ACK(udx_ack_reader& ack_reader);

		udx_alloc*				m_sys_alloc;
		udx_alloc*				m_msg_alloc;

		udx_rtt*				m_rtt;			// RTT computer
		udx_rto_timer*			m_rto;			// RTO timer

		udx_address*			m_address;
		udx_seqnrs_in			m_seqnrs_in;
		udx_seqnrs_out			m_seqnrs_out;

		udx_peer_queues			m_queues;
	};

	udx_socket_peer::udx_socket_peer(udx_alloc* _allocator, udx_alloc* _msg_allocator, udx_address* _address)
		: m_sys_alloc(_allocator)
		, m_msg_alloc(_msg_allocator)
		, m_address(_address)
	{

	}

	udx_address*	udx_socket_peer::get_address() const
	{
		return m_address;
	}

	bool 	udx_socket_peer::connect()
	{
		return false;
	}

	bool	udx_socket_peer::disconnect()
	{
		return false;
	}

	bool	udx_socket_peer::is_connected() const
	{
		return false;
	}
	
	void	udx_socket_peer::push_incoming(udx_packet * packet)
	{
		udx_packet_hdr* packet_hdr = packet->get_hdr();
		udx_seqnr const packet_seqnr = m_seqnrs_in.get(packet_hdr->m_pkt_seqnr);
		m_queues.in->insert(packet_seqnr, packet);
	}

	bool	udx_socket_peer::pop_incoming(udx_packet *& packet)
	{
		// Pop out the packets that are in-order.
		udx_seqnr seqnr;
		m_queues.in->peek(seqnr, packet);
		if (packet != NULL)
		{
			m_queues.in->dequeue(seqnr, packet);
			return true;
		}
		return false;
	}

	void	udx_socket_peer::push_outgoing(udx_packet * packet)
	{
		m_queues.out->push(packet);
	}

	bool	udx_socket_peer::peek_outgoing(udx_packet *& packet)
	{
		packet = m_queues.out->peek();
		return (packet != NULL);
	}

	bool	udx_socket_peer::pop_outgoing(udx_packet *& packet)
	{
		return m_queues.out->pop(packet);
	}

	void	udx_socket_peer::push_tosend(udx_packet * packet)
	{
		m_queues.snd->push(packet);
	}

	void	udx_socket_peer::push_tosend_now(udx_packet * packet)
	{
		m_queues.snd->push_front(packet);
	}

	bool	udx_socket_peer::peek_tosend(udx_packet *& packet)
	{
		packet = m_queues.snd->peek();
		return (packet != NULL);
	}

	bool	udx_socket_peer::pop_tosend(udx_packet *& packet)
	{
		return m_queues.snd->pop(packet);
	}

	void	udx_socket_peer::push_inflight(udx_packet * packet)
	{
		m_queues.ack->insert(packet->get_seqnr(), packet);
	}

	bool	udx_socket_peer::peek_inflight(udx_packet *& packet)
	{
		udx_seqnr seqnr;
		return m_queues.ack->peek(seqnr, packet);
	}

	bool	udx_socket_peer::pop_inflight(udx_packet *& packet)
	{
		udx_seqnr seqnr;
		m_queues.ack->peek(seqnr, packet);
		if (packet != NULL && packet->get_inf()->m_is_acked)
		{
			m_queues.ack->dequeue(seqnr, packet);

			// Move to next sequence number that has a packet
			while (m_queues.ack->peek(seqnr, packet) == false)
			{
				udx_packet* null_packet;
				m_queues.ack->dequeue(seqnr, null_packet);
			}
			return true;
		}
		return false;
	}

	void	udx_socket_peer::process_ACK(udx_ack_reader& ack_reader)
	{
		// Mark ACK-ed packets in the in-flight queue
		bool acked;
		udx_seqnr seqnr;
		while (ack_reader.pop(seqnr, acked))
		{
			if (acked)
			{
				udx_packet* p;
				if (m_queues.ack->get(seqnr, p) && p != NULL)
				{

				}
			}
		}
	}

	void	udx_socket_peer::received(udx_packet* packet)
	{
		// Does this packet have ACK information ?
		// If so process the 'in-flight' queue with the ACK info
		// and also inform the RTT computer of the ACK so as to
		// update RTT and RTO.

		push_incoming(packet);
	}

	void	udx_socket_peer::process_transmit(udx_packet_writer* writer)
	{
		// For our inflight queue check for any packets that
		// have not received an ACK and are older than RTO.
		// For those we need to mark them as resend
		udx_packet* unacked_timedout_packet;
		while (peek_inflight(unacked_timedout_packet))
		{
			udx_packet_inf* inf = unacked_timedout_packet->get_inf();
			if (m_rto->is_timeout(udx_time::get_time_us(), inf->m_timestamp_send_us))
			{
				// RTO event on this packet, schedule for resend
				inf->m_need_resend = true;
				inf->m_retransmissions += 1;

				// Insert this packet at the head of the 'to-send' queue

			}
		}

		// Do we have new incoming packets, if so prepare an
		// ack packet to be send back.

		// Congestion control: move outgoing queued packets to the 'tosend' queue.
		// Sending should only process the 'tosend' queue.

		udx_packet* packet_to_send;
		while (peek_tosend(packet_to_send))
		{
			//@TODO: Process ACKS into this packet

			// Give it an outgoing sequence number
			udx_packet_hdr* packet_hdr = packet_to_send->get_hdr();
			packet_hdr->m_pkt_seqnr = m_seqnrs_out.get().to_pktseqnr();

			// Write out packet, at anytime the writer can decide to
			// stop writing packets (socket full?)
			if (!writer->write(packet_to_send))
				break;

			// Packet has been written succesfully, pop this packet from
			// the 'tosend' queue and move it to the 'in-flight' queue.
			// The 'in-flight' queue is a sequenced queue and there the
			// packet will wait for an ACK and be subject to RTO.
			pop_tosend(packet_to_send);
			push_inflight(packet_to_send);
		}
	}

}
