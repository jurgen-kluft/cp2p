//==============================================================================
//  x_udx-packetqueue.h
//==============================================================================
#ifndef __XP2P_UDX_PACKET_QUEUE_H__
#define __XP2P_UDX_PACKET_QUEUE_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-seqnr.h"
#include "xp2p\libudx\x_udx-list.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	// Sequenced packets queue, mostly used for processing 'incoming' packets to manage in-order
	// handling as well as detecting incoming packet time-outs where we could send a NACK back
	// to sender.
	// Also the 'in-flight' queue which is waiting for ACK data to free acknowledged packets back
	// to the user.
	class udx_packetsqueue
	{
	public:
		typedef		udx_packet*	ITEM;

		void		set(ITEM* queue_data, u32 queue_size)
		{
			m_queue.set(queue_data, queue_size);
		}

		u32			get_size() const
		{
			return m_queue.m_qsize;
		}

		u32			get_count() const
		{
			return m_queue.m_qcount;
		}

		u32			get_wnd_size() const
		{
			u32 spanning = m_queue.m_qtail.get() - m_queue.m_qhead.get();
			return spanning;
		}

		udx_seqnr	get_head() const { return m_queue.m_qhead; }
		udx_seqnr	get_tail() const { return m_queue.m_qtail; }

		void		insert(udx_seqnr seq_nr, ITEM p)
		{
			m_queue.insert(seq_nr, p);
		}

		ITEM		get(udx_seqnr seq_nr)
		{
			return m_queue.get(seq_nr);
		}

		ITEM		peek(udx_seqnr& seq_nr)
		{
			return m_queue.peek(seq_nr);
		}

		ITEM		dequeue(udx_seqnr& seq_nr)
		{
			ITEM p = m_queue.dequeue(seq_nr);
			return p;
		}

		// 
		// Iteration
		bool		begin(u32& len, udx_seqnr& seqnr, ITEM& item)
		{
			len = 0;
			seqnr = m_queue.m_qhead;
			item = m_queue.get(seqnr);
			return len < get_count();
		}
		bool		next(u32& len, udx_seqnr& seqnr, ITEM& item)
		{
			seqnr.inc();
			item = m_queue.get(seqnr);
			if (item != NULL)
				len += 1;
			return len < get_count();
		}

	protected:
		struct dqueue
		{
			udx_seqnr		m_qhead;
			udx_seqnr		m_qtail;
			u32				m_qcount;
			u32				m_qsize;
			ITEM*			m_items;

			inline			dqueue() : m_qcount(0), m_qsize(0), m_items(NULL) {}

			void			set(ITEM* queue_data, u32 queue_size)
			{
				m_qsize = queue_size;
				m_items = queue_data;
				reset();
			}

			void			reset()
			{
				m_qhead = udx_seqnr::max();
				m_qtail = udx_seqnr::min();
				m_qcount = 0;
			}

			ITEM			get(udx_seqnr seq_nr)
			{
				u32 index = seq_nr.get() % m_qsize;
				return m_items[index];
			}

			ITEM			peek(udx_seqnr& seq_nr)
			{
				if (m_qcount == 0)
				{
					seq_nr = udx_seqnr::nil();
					return NULL;
				}
				u32 index = m_qhead.get() % m_qsize;
				seq_nr = m_qhead;
				ITEM p = m_items[index];
				return p;
			}

			ITEM			dequeue(udx_seqnr& seq_nr)
			{
				if (m_qcount == 0)
				{
					seq_nr = udx_seqnr::nil();
					return NULL;
				}
				u32 const index = m_qhead.get() % m_qsize;
				ITEM p = m_items[index];

				seq_nr = m_qhead;
				m_qhead.inc();
				if (p != NULL)
				{
					m_items[index] = NULL;
					m_qcount -= 1;
				}
				return p;
			}

			bool			insert(udx_seqnr seq_nr, ITEM p)
			{
				u32 index = seq_nr.get() % m_qsize;
				if ((p != NULL) && m_items[index] == NULL)
				{
					if (seq_nr < m_qhead)
						m_qhead = seq_nr;
					if (seq_nr > m_qtail)
						m_qtail = seq_nr;
					m_qcount++;
					m_items[index] = p;
					return true;
				}
			}
		};
		dqueue			m_queue;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	// Basic packets queue using a list, mostly used for processing 'outgoing', 'garbage' and 
	// 'received' packets.
	class udx_packetlqueue
	{
		udx_list	m_list;

	public:
		typedef		udx_packet*	ITEM;

		u32			get_count() const
		{
			return m_list.m_count;
		}

		void		enqueue(ITEM p)
		{
			m_list.enqueue(p->get_list_node());
		}

		ITEM		peek()
		{
			udx_list_node* node;
			if (m_list.head(node))
				return (ITEM)node->m_item;
			return NULL;
		}

		ITEM		dequeue()
		{
			udx_list_node* node;
			if (m_list.dequeue(node))
				return (ITEM)node->m_item;
			return NULL;
		}

		// 
		// Iteration
		bool		begin(u32& len, ITEM& item)
		{
			len = 0;
			item = (ITEM)m_list.m_root.m_prev->m_item;
			return len < m_list.m_count;
		}

		bool		next(u32& len, ITEM& item)
		{
			len += 1;
			udx_list_node* node = item->get_list_node()->m_prev;
			item = (ITEM)node->m_item;
			return len < m_list.m_count;
		}
	};

}

#endif
