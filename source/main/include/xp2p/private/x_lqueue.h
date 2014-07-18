//==============================================================================
//  x_llist.h
//==============================================================================
#ifndef __XPEER_2_PEER_LINKED_LIST_H__
#define __XPEER_2_PEER_LINKED_LIST_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

namespace xcore
{
	namespace xp2p
	{
		// queue using a circular doubly linked list

		template <typename T>
		class lqueue
		{
		public:
			inline			lqueue(T* base) : next_(base), prev_(base) { }

			inline T*		get_next() const						{ return next_; }
			inline T*		get_prev() const						{ return prev_; }

			inline T*		get() const								{ return next_->get_prev(); }

			inline void		enqueue(T* _item)
			{
				T* tail = get_prev();
				T* head = tail->get_next();
				head->set_prev(_item);
				tail->set_next(_item);
				_item->set_next(head);
				_item->set_prev(tail);
			}

			inline T*		dequeue(T** current_head = NULL)
			{
				T* head = this->get_next();
				T* item = head->get_prev();
				T* tail = this->get_prev();
				tail->set_next(head);
				head->set_prev(tail);
				item->set_next(NULL);
				item->set_prev(NULL);
				if (current_head != NULL && item == *current_head)
					*current_head = head;
				return item;
			}

		protected:
			inline void		set_next(T* _next)						{ next_ = _next; }
			inline void		set_prev(T* _prev)						{ prev_ = _prev; }

			T*				next_;
			T*				prev_;
		};

	}
}

#endif