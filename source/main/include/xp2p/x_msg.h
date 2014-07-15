//==============================================================================
//  x_msg.h
//==============================================================================
#ifndef __XPEER_2_PEER_MSG_READER_WRITER_H__
#define __XPEER_2_PEER_MSG_READER_WRITER_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xbase\x_allocator.h"

namespace xcore
{
	// ==============================================================================================================================
	// ==============================================================================================================================
	// ==============================================================================================================================
	namespace xp2p
	{
		class ipeer;

		class message_block
		{
		public:
			inline				message_block() : flags_(0), size_(0), data_(NULL), const_data_(NULL) {}
			inline				message_block(void* _data, u32 _size) : flags_(0), size_(_size), data_(_data), const_data_(NULL) {}
			inline				message_block(void const* _data, u32 _size) : flags_(0), size_(_size), data_(NULL), const_data_(_data) {}

			u32					get_flags() const;
			void				set_flags(u32 _flags);

			u32					get_size() const;
			const void*			get_data() const;

			XCORE_CLASS_PLACEMENT_NEW_DELETE

		protected:

			u32					flags_;
			u32					size_;
			void*				data_;
			void const*			const_data_;
		};

		class message_reader
		{
		public:
			inline				message_reader(message_block* _block) : cursor_(0), block_(_block) {}

			void				set_cursor(u32);
			u32					cursor() const;

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

		protected:
			u32					cursor_;
			xbyte const*		data_;
			message_block*		block_;
		};

		class message_writer
		{
		public:
			inline				message_writer(message_block* _block) : cursor_(0), block_(_block) {}

			void				set_cursor(u32);
			u32					cursor() const;

			bool				can_write(u32 num_bytes = 0) const;

			void				write(bool);
			void				write(u8  );
			void				write(s8  );
			void				write(u16 );
			void				write(s16 );
			void				write(u32 );
			void				write(s32 );
			void				write(u64 );
			void				write(s64 );
			void				write(f32 );
			void				write(f64 );

			void				write_data(const xbyte*, u32);
			void				write_string(const char*);

		protected:
			u32					cursor_;
			xbyte*				data_;
			message_block*		block_;
		};


		class message
		{
		public:
			inline				message(ipeer* _from, ipeer* _to, u32 _flags) : from_(_from), to_(_to), flags_(_flags), nblocks_(0), pblocks_(NULL) {}

			bool				is_from(ipeer*) const;

			u32					get_flags() const;

			bool				has_event() const;
			bool				has_data() const;

			bool				event_is_connected() const;
			bool				event_disconnected() const;
			bool				event_cannot_connect() const;

			void				add_block(message_block* _block);

			XCORE_CLASS_PLACEMENT_NEW_DELETE

		protected:
			ipeer*				from_;
			ipeer*				to_;
			u32					flags_;
			u32					nblocks_;
			message_block*		pblocks_;
		};

		class outgoing_message
		{
		public:
			inline				 outgoing_message() : message_(NULL) {}
			inline				 outgoing_message(message* _message) : message_(_message) {}
								 outgoing_message(const outgoing_message&);
			inline				~outgoing_message() {}

			u32					num_blocks() const;
			void				add_block(message_block*);
			message_writer		get_writer(u32 _block_index=0) const;
			
			void				release(imessage_allocator*);

		protected:
			message*			message_;
		};


		class incoming_message
		{
		public:
			inline				 incoming_message() : message_(NULL) {}
			inline				 incoming_message(message* _message) : message_(_message) {}
								 incoming_message(const incoming_message&);
			inline				~incoming_message() {}

			bool				is_from(ipeer*) const;
			ipeer*				from();

			bool				has_event() const;
			bool				has_data() const;

			bool				event_is_connected() const;
			bool				event_disconnected() const;
			bool				event_cannot_connect() const;

			u32					num_blocks() const;
			message_reader		get_reader(u32 _index=0) const;

			void				release();

		protected:
			message*			message_;
		};

		class imessage_allocator
		{
		public:
			virtual message*		allocate(ipeer* _from, ipeer* _to, u32 _flags) = 0;
			virtual void			deallocate(message*) = 0;

			virtual message_block*	allocate(u32 _flags, u32 _size) = 0;
			virtual void			deallocate(message_block*) = 0;
		};

		class incoming_messages
		{
		public:
			virtual bool			dequeue(incoming_message&) = 0;

		protected:
			virtual					~incoming_messages() {}
		};

		class outgoing_messages
		{
		public:
			virtual bool			enqueue(outgoing_message&) = 0;
			virtual bool			dequeue(outgoing_message&) = 0;

		protected:
			virtual					~outgoing_messages() {}
		};

	}
}

#endif	///< __XPEER_2_PEER_MSG_READER_WRITER_H__
