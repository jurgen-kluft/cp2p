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
			inline				ns_message_system() : flags_(0)		{}
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
		};

		struct ns_message_payload
		{
			//void*				body;
		};

		struct ns_message
		{
			ns_allocator*				allocator_;
			ns_message_list_node		list_;			// linked-list
			ns_message_system			system_;		
			ns_message_io_state			io_state_;
			ns_message_header			header_;
			ns_message_payload			payload_;		// <--- IncommingMessage is received here
		};

		bool			is_message_header_ok(ns_message_header const& _header);

		ns_message*		create_event_connect_msg(ns_allocator * _allocator, u32 _remote);
		ns_message*		create_event_disconnect_msg(ns_allocator * _allocator, u32 _remote);
			
		ns_message*		create_send_payload_msg(ns_allocator * _allocator, ns_message_header const& _header);
		ns_message*		create_received_payload_msg(ns_allocator * _allocator, ns_message_header const& _header);

		ns_message*		pop_msg(ns_message *& );
		void			push_msg(ns_message *& , ns_message * );
		void			release_msg(ns_message * );
	}
}

#endif	///< __XPEER_2_PEER_MSG_PRIVATE_H__
