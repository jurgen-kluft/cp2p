#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "xp2p\private\x_netmsg.h"
#include "xp2p\private\x_netio.h"

namespace xcore
{
	namespace xnetio
	{
		
		bool	ns_message_header::is_valid() const
		{
			return magic_ == 'XP2P';
		}


		ns_message*		ns_message_alloc(ns_allocator * _allocator, ns_message_type _type, u32 _sizeof_payload)
		{
			u32 const message_size = sizeof(ns_message) + ((_sizeof_payload + 3) & 0xfffffffc);
			ns_message * message = (ns_message*)_allocator->alloc(message_size, sizeof(void*));
			message->list_ = ns_message_list_node();
			message->system_ = ns_message_system(_allocator, _type);
			message->io_state_ = ns_message_io_state();
			message->header_ = ns_message_header(_sizeof_payload);
			return message;
		}

		void			ns_message_dealloc(ns_message * _msg)
		{
			_msg->system_.allocator_->dealloc(_msg);
		}

		ns_message*		ns_message_dequeue(ns_message *& _msg_queue)
		{
			if (_msg_queue == NULL)
				return NULL;

			ns_message * head = _msg_queue;
			ns_message * tail = _msg_queue->list_.prev_;
			ns_message * msg = tail;
			tail = tail->list_.prev_;
			head->list_.prev_ = tail;
			tail->list_.next_ = head;

			_msg_queue = (head == msg) ? NULL : head;

			msg->list_ = ns_message_list_node();
			return msg;
		}

		void			ns_message_enqueue(ns_message *& _msg_queue, ns_message * _msg)
		{
			if (_msg_queue == NULL)
			{
				_msg->list_.next_ = _msg;
				_msg->list_.prev_ = _msg;
				_msg_queue = _msg;
			}
			else
			{
				ns_message * head = _msg_queue;
				ns_message * tail = _msg_queue->list_.prev_;
				head->list_.prev_ = _msg;
				_msg->list_.next_ = head;
				tail->list_.next_ = _msg;
				_msg->list_.prev_ = tail;
				_msg_queue = _msg;
			}
		}

	}
}