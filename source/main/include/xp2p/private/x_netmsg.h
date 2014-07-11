//==============================================================================
//  x_netmsg.h (private)
//==============================================================================
#ifndef __XPEER_2_PEER_NETWORK_MSG_PRIVATE_H__
#define __XPEER_2_PEER_NETWORK_MSG_PRIVATE_H__
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
			inline				ns_message_system(ns_allocator* _a, u32 _f) : allocator_(_a), flags_(_f) {}
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
			inline				ns_message_header() : magic_('XP2P'), length_(0) {}
			inline				ns_message_header(u32 _length) : magic_('XP2P'), length_(_length) {}

			u32					magic_;		// 'XP2P'
			u32					length_;	// payload size (0-256KiB)

			bool				is_valid() const;
		};

		struct ns_message
		{
			ns_message_list_node		list_;			// linked-list
			ns_message_system			system_;		
			ns_message_io_state			io_state_;
			ns_message_header			header_;
			inline xbyte*				payload()		{ return (xbyte*)&header_ + sizeof(ns_message_header); }
		};

		enum ns_message_type
		{
			NS_MSG_TYPE_EVENT_CONNECTED,
			NS_MSG_TYPE_EVENT_DISCONNECTED,
			NS_MSG_TYPE_DATA,
		};

		ns_message*		ns_message_alloc(ns_allocator * _allocator, ns_message_type _type, u32 _sizeof_payload);
		void			ns_message_dealloc(ns_message * );

		ns_message*		ns_message_dequeue(ns_message *& );
		void			ns_message_enqueue(ns_message *& , ns_message * );
	}
}

#endif	///< __XPEER_2_PEER_NETWORK_MSG_PRIVATE_H__
