//==============================================================================
//  x_msg.h
//==============================================================================
#ifndef __XPEER_2_PEER_MSG_READER_WRITER_H__
#define __XPEER_2_PEER_MSG_READER_WRITER_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

namespace xcore
{
	// ==============================================================================================================================
	// ==============================================================================================================================
	// ==============================================================================================================================
	namespace xp2p
	{
		class ipeer;

		struct message_type
		{
			inline			message_type(bool _is_event) : is_event_(_is_event) {}

			inline bool		is_event() const					{ return is_event_; }
			inline bool		has_data() const					{ return !is_event_; }

		private:
			bool			is_event_;
		};

		struct message_event
		{
			inline			message_event(bool _is_connected, bool _cannot_connect) : is_connected_(_is_connected), cannot_connect_(_cannot_connect) {}

			inline bool		is_connected() const				{ return is_connected_; }
			inline bool		is_not_connected() const			{ return !is_connected_; }

			inline bool		cannot_connect() const				{ return cannot_connect_; }

		private:
			bool			is_connected_;
			bool			cannot_connect_;
		};

		class outgoing_message
		{
		public:
			inline			 outgoing_message() : data_(0), cursor_(0) {}
			inline			~outgoing_message() {}

			u32				current_size() const;
			u32				maximum_size() const;
			bool			can_write(u32 num_bytes) const;		// Check if we still can write N number of bytes

			void			write(bool);
			void			write(u8 );
			void			write(s8 );
			void			write(u16);
			void			write(s16);
			void			write(u32);
			void			write(s32);
			void			write(u64);
			void			write(s64);
			void			write(f32);
			void			write(f64);
			void			write(const char*);

			void			release();

		protected:
			inline			 outgoing_message(const outgoing_message&) : data_(0), cursor_(0) {}
			
			void*			data_;
			u32				cursor_;
		};

		class incoming_message
		{
		public:
			inline			 incoming_message() : data_(0), cursor_(0) {}
			inline			~incoming_message() {}

			bool			is_empty() const;
			message_type	type() const;
			message_event	event() const;

			bool			is_from(ipeer*) const;
			peerid			from() const;

			u32				read_size() const;
			u32				total_size() const;
			bool			can_read(u32 number_of_bytes) const;		// check if we still can read n number of bytes

			bool			read(bool&) const;
			bool			read(u8 &) const;
			bool			read(s8 &) const;
			bool			read(u16&) const;
			bool			read(s16&) const;
			bool			read(u32&) const;
			bool			read(s32&) const;
			bool			read(u64&) const;
			bool			read(s64&) const;
			bool			read(f32&) const;
			bool			read(f64&) const;

			bool			read_string(const char* str, u32 maxstrlen, u32& strlen);

			bool			next(incoming_message& previous);
			void			release();

		protected:
			inline			 incoming_message(const incoming_message&) : data_(0), cursor_(0) {}

			void*			data_;
			u32				cursor_;
		};
	}
}

#endif	///< __XPEER_2_PEER_MSG_READER_WRITER_H__
