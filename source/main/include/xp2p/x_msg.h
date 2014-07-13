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

		class message_allocator
		{
		public:
			virtual void*		allocate(u32 size, u32 alignment) = 0;
			virtual void		deallocate(void*) = 0;
		};

		struct message_header
		{
			inline				message_header() : flags_(0), from_(0), to_(0) {}
			inline				message_header(u32 _flags, peerid _from, peerid _to) : flags_(_flags), from_(_from), to_(_to) {}

			bool				is_event() const;
			bool				is_data() const;

			bool				is_connected() const;
			bool				is_not_connected() const;
			bool				cannot_connect() const;

			bool				is_from(peerid) const;

		private:
			u32					flags_;
			peerid				from_;
			peerid				to_;
		};

		struct message_data
		{
			inline				message_data() : data_(NULL), size_(0), cursor_(0) {}

			u32					size_;
			message_allocator*	allocator_;
			void*				data_;
			u32					cursor_;
		};

		class outgoing_message
		{
		public:
			inline			 outgoing_message() {}
							 outgoing_message(const outgoing_message&);
			inline			~outgoing_message() {}

			u32				cursor() const;
			u32				size() const;

			bool			can_write(u32 num_bytes) const;		// Check if we still can write N number of bytes

			void			write(bool);
			void			write(u8  );
			void			write(s8  );
			void			write(u16 );
			void			write(s16 );
			void			write(u32 );
			void			write(s32 );
			void			write(u64 );
			void			write(s64 );
			void			write(f32 );
			void			write(f64 );

			void			write_data(const xbyte*, u32);
			void			write_string(const char*, u32);

			void			release();

		protected:			
			message_header	header_;
			message_data	data_;
		};

		class incoming_message;

		class incoming_messages
		{
		public:
			virtual bool				is_empty() const = 0;
			
			virtual incoming_message	allocate() = 0;
			virtual incoming_message	dequeue() = 0;
		};

		class incoming_message
		{
		public:
			inline				 incoming_message() {}
								 incoming_message(const incoming_message&);
			inline				~incoming_message() {}

			message_header 		header() const;

			bool				is_empty() const;

			u32					cursor() const;
			u32					size() const;

			bool				can_read(u32 number_of_bytes) const;		// check if we still can read n number of bytes

			bool				read(bool&) const;
			bool				read(u8  &) const;
			bool				read(s8  &) const;
			bool				read(u16 &) const;
			bool				read(s16 &) const;
			bool				read(u32 &) const;
			bool				read(s32 &) const;
			bool				read(u64 &) const;
			bool				read(s64 &) const;
			bool				read(f32 &) const;
			bool				read(f64 &) const;

			bool				read_data(xbyte* data, u32 size, u32& written);
			bool				read_string(char* str, u32 maxstrlen, u32& strlen);

			void				release();

		protected:
			message_header		header_;
			message_data		data_;
		};
	}
}

#endif	///< __XPEER_2_PEER_MSG_READER_WRITER_H__
