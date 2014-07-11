//==============================================================================
//  x_msg.h (private)
//==============================================================================
#ifndef __XPEER_2_PEER_MSG_PRIVATE_H__
#define __XPEER_2_PEER_MSG_PRIVATE_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

namespace xcore
{
	namespace xnetio
	{
		class ns_allocator;
		struct ns_message;

		struct ns_message_list_node
		{
			inline				ns_message_list_node() : next_(NULL), prev_(NULL)	{}
			ns_message*			next_;
			ns_message*			prev_;
		};

		struct ns_message_system
		{
			inline				ns_message_system() : allocator_(NULL), flags_(0)	{}
			ns_allocator*		allocator_;
			u32					flags_;
		};

		struct ns_message_io_state
		{
			inline				ns_message_io_state() : length_(0)		{}
			u32					length_;	// received or sent payload length
		};

		struct ns_message_header
		{
			u32					magic_;		// 'XP2P'
			u32					length_;	// payload size (0-256KiB)
			u32					from_;
			u32					to_;

			bool				is_valid() const;
		};

		struct ns_message_payload
		{
			//void*				body;
		};

		struct ns_message
		{
			ns_message_list_node		list_;			// linked-list
			ns_message_system			system_;		
			ns_message_io_state			io_state_;
			ns_message_header			header_;
			ns_message_payload			payload_;		// <--- IncommingMessage is received here
		};

		ns_message*		ns_message_alloc(ns_allocator * _allocator, ns_message_system const& _system, ns_message_header const& _header);
		void			ns_message_dealloc(ns_message * );

		ns_message*		ns_message_dequeue(ns_message *& );
		void			ns_message_enqueue(ns_message *& , ns_message * );
	}
}

#endif	///< __XPEER_2_PEER_MSG_PRIVATE_H__
