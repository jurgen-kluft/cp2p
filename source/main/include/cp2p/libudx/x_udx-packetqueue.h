//==============================================================================
//  x_udx-packetqueue.h
//==============================================================================
#ifndef __XP2P_UDX_PACKET_QUEUE_H__
#define __XP2P_UDX_PACKET_QUEUE_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xp2p/libudx/x_udx.h"
#include "xp2p/libudx/x_udx-seqnr.h"
#include "xp2p/libudx/x_udx-list.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	// Sequenced packets queue, mostly used for processing 'incoming' packets to manage in-order
	// handling as well as detecting incoming packet time-outs where we could send a NACK back
	// to sender.
	// Also the 'in-flight' queue which is waiting for ACK data to free acknowledged packets back
	// to the user.
	class udx_packets_squeue
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

		bool		get(udx_seqnr seq_nr, ITEM& item)
		{
			return m_queue.get(seq_nr, item);
		}

		bool		peek(udx_seqnr& seq_nr, ITEM& item)
		{
			return m_queue.peek(seq_nr, item);
		}

		bool		dequeue(udx_seqnr& seq_nr, ITEM& item)
		{
			return m_queue.dequeue(seq_nr, item);
		}

		bool		dequeue_ff(udx_seqnr& seq_nr, ITEM& item)
		{
			while (m_queue.peek(seq_nr, item) && item == NULL)
			{
				m_queue.dequeue(seq_nr, item);
			}
		}

		// 
		// Iteration
		bool		begin(u32& len, udx_seqnr& seqnr, ITEM& item)
		{
			len = 0;
			seqnr = m_queue.m_qhead;
			m_queue.get(seqnr, item);
			return len < get_count();
		}

		bool		next(u32& len, udx_seqnr& seqnr, ITEM& item)
		{
			seqnr.inc();
			if (m_queue.get(seqnr, item) == false)
				return false;

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

			bool			get(udx_seqnr seq_nr, ITEM& item)
			{
				if (seq_nr >= m_qhead && seq_nr <= m_qtail)
				{
					u32 index = seq_nr.get() % m_qsize;
					item = m_items[index];
					return true;
				}
				return false;
			}

			bool			peek(udx_seqnr& seq_nr, ITEM& item)
			{
				if (m_qcount == 0)
				{
					seq_nr = udx_seqnr::nil();
					item = NULL;
					return false;
				}
				u32 index = m_qhead.get() % m_qsize;
				seq_nr = m_qhead;
				ITEM p = m_items[index];
				return true;
			}

			bool			dequeue(udx_seqnr& seq_nr, ITEM& item)
			{
				if (m_qcount == 0)
				{
					seq_nr = udx_seqnr::nil();
					item = NULL;
					return false;
				}
				u32 const index = m_qhead.get() % m_qsize;
				item = m_items[index];
				seq_nr = m_qhead;

				m_qhead.inc();
				if (item != NULL)
				{
					m_items[index] = NULL;
					m_qcount -= 1;
				}
				return true;
			}

			void			insert(udx_seqnr seq_nr, ITEM p)
			{
				u32 index = seq_nr.get() % m_qsize;
				if (seq_nr < m_qhead)
					m_qhead = seq_nr;
				if (seq_nr > m_qtail)
					m_qtail = seq_nr;
				m_qcount++;
				m_items[index] = p;
			}
		};
		dqueue			m_queue;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	// Basic packets queue using a list, mostly used for processing 'outgoing', 'garbage' and 
	// 'received' packets.
	class udx_packets_lqueue
	{
		udx_list	m_list;

	public:
		typedef		udx_packet*	ITEM;

		u32			get_count() const
		{
			return m_list.m_count;
		}

		void		push(ITEM p)
		{
			m_list.push(p->get_list_node());
		}

		void		push_front(ITEM p)
		{
			m_list.push_front(p->get_list_node());
		}

		ITEM		peek()
		{
			udx_list_node* node;
			if (m_list.head(node))
				return (ITEM)node->m_item;
			return NULL;
		}

		bool		pop(ITEM& item)
		{
			udx_list_node* node;
			if (m_list.pop(node))
			{
				item = (ITEM)node->m_item;
				return true;
			}
			return false;
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
