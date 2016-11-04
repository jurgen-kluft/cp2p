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

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API

	// Inserting and removing items could also be tracking intervals, the best would be to have a
	// seperate interval object which can be allocated/deallocated.

	// A udx_packet_qnode would then have a pointer to the interval that it is currently part of.
	// When adding a node you check on the left/right, if there is an existing neighbour you take
	// the pointer to the interval object and you update the interval.
	// Same with removing, if you are at the extend of the interval you update the interval. If
	// you are the last of the interval you dealloc the interval object.
	// This interval object can then be used when the ACK data needs to be constructed, it is fast
	// because you immediately have the begin and end of the interval. Empty items would not have
	// an interval although we could make a 'fake' node that we only know internally which points
	// to an interval that keeps track of non-existing continues item intervals.

	class udx_packetqueue
	{
	public:
		typedef		void*	ITEM;

		u32			get_size() const
		{
			return m_queue.m_qsize;
		}

		u32			get_count() const
		{
			return m_queue.m_qcount;
		}

		u32			get_spanning() const
		{
			u32 spanning = m_queue.m_qtail.get() - m_queue.m_qhead.get();
			return spanning;
		}

		udx_seqnr	get_head() const { return m_queue.m_qhead; }
		udx_seqnr	get_tail() const { return m_queue.m_qtail; }

		void		enqueue(udx_seqnr seq_nr, ITEM p)
		{
			m_queue.enqueue(seq_nr, p);
		}

		ITEM		dequeue()
		{
			return m_queue.dequeue();
		}

		ITEM		get(udx_seqnr seq_nr)
		{
			return m_queue.get(seq_nr);
		}

		bool		iter(u32& len, udx_seqnr& seqnr)
		{
			seqnr = m_queue.m_qhead;
			len = 0;
			for (s32 i = 0; i < m_queue.m_qcount; ++i)
			{ 
				if (m_queue.get(seqnr) == NULL)
					break;
				len += 1;
				seqnr.inc();
			}
			return len > 0;
		}

	protected:
		struct queue
		{
			udx_seqnr		m_qhead;
			udx_seqnr		m_qtail;
			u32				m_qcount;
			u32				m_qsize;
			ITEM*			m_items;
			udx_alloc*		m_allocator;

			void			reset()
			{
				m_qhead = udx_seqnr::max();
				m_qtail = udx_seqnr::min();
				m_qcount = 0;
				m_qsize = 4096;
				m_items = (ITEM*)m_allocator->alloc(sizeof(ITEM) * m_qsize);
			}

			ITEM			get(udx_seqnr seq_nr)
			{
				u32 index = seq_nr.get() % m_qsize;
				return m_items[index];
			}

			ITEM			dequeue()
			{
				if (m_qcount == 0)
					return NULL;

				u32 index = m_qhead.get() % m_qsize;
				ITEM p = m_items[index];
				if (p != NULL)
				{
					m_items[index] = NULL;
					m_qhead.inc();
					m_qcount--;
				}
				return p;
			}

			void			enqueue(udx_seqnr seq_nr, ITEM p)
			{
				u32 index = seq_nr.get() % m_qsize;
				if (m_items[index] == NULL)
				{
					if (seq_nr < m_qhead)
						m_qhead = seq_nr;
					if (seq_nr > m_qtail)
						m_qtail = seq_nr;

					m_qcount++;
				}
				m_items[index] = p;
			}

			void			interval_item_insert(udx_seqnr seq_nr, ITEM p)
			{

			}

			void			interval_item_remove(udx_seqnr seqnr)

		};
		queue			m_queue;
	};


}

#endif