#include "xbase\x_target.h"
#include "xbase\x_allocator.h"
#include "xbase\x_memory_std.h"
#include "xbase\x_bit_field.h"

#include "xp2p\private\x_netio_proto.h"
#include "xp2p\private\x_netio.h"

#include "xp2p\x_msg.h"

namespace xcore
{
	namespace xp2p
	{
		/// @DESIGN NOTE:
		///  The p2p message here has direct knowledge of ns_message and actually the
		///  structures at this layer are just wrappers around ns_message.

		/// ---------------------------------------------------------------------------------------
		/// Unaligned Data Reading/Writing
		/// ---------------------------------------------------------------------------------------
		inline u8			read_u8 (xbyte const* ptr)				{ return (u8)*ptr; }
		inline s8			read_s8 (xbyte const* ptr)				{ return (s8)*ptr; }
		inline u16			read_u16(xbyte const* ptr)				{ u16 b0 = *ptr++; u16 b1 = *ptr++; return (u16)((b1<<8) | b0); }
		inline s16			read_s16(xbyte const* ptr)				{ u16 b0 = *ptr++; u16 b1 = *ptr++; return (s16)((b1<<8) | b0); }
		inline u32			read_u32(xbyte const* ptr)				{ u32 b0 = *ptr++; u32 b1 = *ptr++; u32 b2 = *ptr++; u32 b3 = *ptr++; return (u32)((b3<<24) | (b2<<16) | (b1<<8) | (b0)); }
		inline s32			read_s32(xbyte const* ptr)				{ u32 b0 = *ptr++; u32 b1 = *ptr++; u32 b2 = *ptr++; u32 b3 = *ptr++; return (s32)((b3<<24) | (b2<<16) | (b1<<8) | (b0)); }
		inline f32			read_f32(xbyte const* ptr)				{ u32 b0 = *ptr++; u32 b1 = *ptr++; u32 b2 = *ptr++; u32 b3 = *ptr++; u32 u = ((b3<<24) | (b2<<16) | (b1<<8) | (b0)); return *((f32*)&u); }
		inline u64			read_u64(xbyte const* ptr)				{ u64 l0 = read_u32(ptr); u64 l1 = read_u32(ptr+4); return (u64)((l1 << 32) | l0); }
		inline s64			read_s64(xbyte const* ptr)				{ u64 l0 = read_u32(ptr); u64 l1 = read_u32(ptr+4); return (s64)((l1 << 32) | l0); }
		inline f64			read_f64(xbyte const* ptr)				{ u64 l0 = read_u32(ptr); u64 l1 = read_u32(ptr+4); u64 ll = (u64)((l1 << 32) | l0); return *((f64*)ll); }

		inline void			write_u8 (xbyte * ptr, u8  b)			{ *ptr = b; }
		inline void			write_s8 (xbyte * ptr, s8  b)			{ *ptr = b; }
		inline void			write_u16(xbyte * ptr, u16 b)			{ for (s32 i=0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
		inline void			write_s16(xbyte * ptr, s16 b)			{ for (s32 i=0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
		inline void			write_u32(xbyte * ptr, u32 b)			{ for (s32 i=0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
		inline void			write_s32(xbyte * ptr, s32 b)			{ for (s32 i=0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
		inline void			write_f32(xbyte * ptr, f32 f)			{ u32 b = *((u32*)&f); for (s32 i=0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
		inline void			write_u64(xbyte * ptr, u64 b)			{ for (s32 i=0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
		inline void			write_s64(xbyte * ptr, s64 b)			{ for (s32 i=0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
		inline void			write_f64(xbyte * ptr, f64 f)			{ u64 b = *((u64*)&f); for (s32 i=0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }

		/// ---------------------------------------------------------------------------------------
		/// Message Block
		/// ---------------------------------------------------------------------------------------

		u32					message_block::get_flags() const
		{
			return flags_;
		}

		void				message_block::set_flags(u32 _flags)
		{
			flags_ = _flags;
		}

		u32					message_block::get_size() const
		{
			return size_;
		}

		xbyte*				message_block::get_data()
		{
			return (xbyte*)data_;
		}

		const xbyte*			message_block::get_data() const
		{
			return (xbyte const*)data_;
		}

		/// ---------------------------------------------------------------------------------------
		/// Message Reader
		/// ---------------------------------------------------------------------------------------
		inline bool			_can_read(message_block* _block, u32 _cursor, u32 _num_bytes)
		{
			if (_block == NULL) return false;
			else return (_cursor + _num_bytes) <= _block->get_size();
		}

		void				message_reader::set_cursor(u32 c)
		{
			cursor_ = c;
			if (cursor_ > block_->get_size())
				cursor_ = block_->get_size();
		}
		u32					message_reader::get_cursor() const
		{
			return cursor_;
		}


		bool				message_reader::can_read(u32 number_of_bytes) const
		{
			return _can_read(block_, cursor_, number_of_bytes);
		}


		u32					message_reader::read(bool& b)
		{
			if (_can_read(block_, cursor_, 1))
			{
				xbyte const* ptr = (xbyte const*)block_->get_data() + cursor_;
				cursor_ += 1;
				b = read_u8(ptr) != 0;
				return 1;
			}
			return 0;
		}

		u32					message_reader::read(u8  & b)
		{
			if (_can_read(block_, cursor_, sizeof(b)))
			{
				u8 const* ptr = (u8 const*)block_->get_data() + cursor_;
				cursor_ += sizeof(b);
				b = read_u8(ptr);
				return sizeof(b);
			}
			return 0;
		}

		u32					message_reader::read(s8  & b)
		{
			if (_can_read(block_, cursor_, sizeof(b)))
			{
				u8 const* ptr = (u8 const*)block_->get_data() + cursor_;
				cursor_ += sizeof(b);
				b = read_s8(ptr);
				return sizeof(b);
			}
			return 0;
		}

		u32					message_reader::read(u16 & b)
		{
			if (_can_read(block_, cursor_, sizeof(b)))
			{
				u8 const* ptr = (u8 const*)block_->get_data() + cursor_;
				cursor_ += sizeof(b);
				b = read_u16(ptr);
				return sizeof(b);
			}
			return 0;
		}

		u32					message_reader::read(s16 & b)
		{
			if (_can_read(block_, cursor_, sizeof(b)))
			{
				u8 const* ptr = (u8 const*)block_->get_data() + cursor_;
				cursor_ += sizeof(b);
				b = read_s16(ptr);
				return sizeof(b);
			}
			return 0;
		}

		u32					message_reader::read(u32 & b)
		{
			if (_can_read(block_, cursor_, sizeof(b)))
			{
				u8 const* ptr = (u8 const*)block_->get_data() + cursor_;
				cursor_ += sizeof(b);
				b = read_u32(ptr);
				return sizeof(b);
			}
			return 0;
		}

		u32					message_reader::read(s32 & b)
		{
			if (_can_read(block_, cursor_, sizeof(b)))
			{
				u8 const* ptr = (u8 const*)block_->get_data() + cursor_;
				cursor_ += sizeof(b);
				b = read_s32(ptr);
				return sizeof(b);
			}
			return 0;
		}

		u32					message_reader::read(u64 & b)
		{
			if (_can_read(block_, cursor_, sizeof(b)))
			{
				u8 const* ptr = (u8 const*)block_->get_data() + cursor_;
				cursor_ += sizeof(b);
				b = read_u64(ptr);
				return sizeof(b);
			}
			return 0;
		}

		u32					message_reader::read(s64 & b)
		{
			if (_can_read(block_, cursor_, sizeof(b)))
			{
				u8 const* ptr = (u8 const*)block_->get_data() + cursor_;
				cursor_ += sizeof(b);
				b = read_s64(ptr);
				return sizeof(b);
			}
			return 0;
		}

		u32					message_reader::read(f32 & b)
		{
			if (_can_read(block_, cursor_, sizeof(b)))
			{
				u8 const* ptr = (u8 const*)block_->get_data() + cursor_;
				cursor_ += sizeof(b);
				b = read_f32(ptr);
				return sizeof(b);
			}
			return 0;
		}

		u32					message_reader::read(f64 & b)
		{
			if (_can_read(block_, cursor_, sizeof(b)))
			{
				u8 const* ptr = (u8 const*)block_->get_data() + cursor_;
				cursor_ += sizeof(b);
				b = read_f64(ptr);
				return sizeof(b);
			}
			return 0;
		}


		bool				message_reader::view_data(xbyte const*& data, u32 size)
		{
			if (!_can_read(block_, cursor_, size))
			{
				data = NULL;
				return false;
			}
			data = (xbyte const*)block_->get_data() + cursor_;
			cursor_ += size;
			return true;
		}

		bool				message_reader::view_string(char const*& str, u32& strlen)
		{
			if (block_ == NULL)
				return false;

			strlen = 0;
			str = (char const*)block_->get_data() + cursor_;
			while (cursor_ < block_->get_size())
			{
				if (str[strlen] == '\0')
					break;
				++strlen;
			}

			if (str[strlen] != '\0')
			{
				str = NULL;
				strlen = 0;
				return false;
			}

			cursor_ += strlen;
			return true;
		}

		void				message_reader::next_block()
		{
			if (block_ != NULL)
				block_ = block_->get_next();
		}


		/// ---------------------------------------------------------------------------------------
		/// Message Writer
		/// ---------------------------------------------------------------------------------------
		inline bool			_can_write(message_block* _block, u32 _cursor, u32 _num_bytes)
		{
			if (_block == NULL) return false;
			else return (_cursor + _num_bytes) <= _block->get_size();
		}

		void				message_writer::set_cursor(u32 c)
		{
			cursor_ = c;
			if (cursor_ > block_->get_size())
				cursor_ = block_->get_size();
		}

		u32					message_writer::get_cursor() const
		{
			return cursor_;
		}


		bool				message_writer::can_write(u32 num_bytes) const
		{
			return _can_write(block_, cursor_, num_bytes);
		}


		void				message_writer::write(bool b)
		{
			if (_can_write(block_, cursor_, 1))
			{
				write_u8((xbyte*)block_->get_data() + cursor_, b ? 1 : 0);
				cursor_ += 1;
			}
		}

		void				message_writer::write(u8   b)
		{
			if (_can_write(block_, cursor_, sizeof(b)))
			{
				write_u8((xbyte*)block_->get_data() + cursor_, b);
				cursor_ += sizeof(b);
			}
		}

		void				message_writer::write(s8   b)
		{
			if (_can_write(block_, cursor_, sizeof(b)))
			{
				write_u8((xbyte*)block_->get_data() + cursor_, b);
				cursor_ += sizeof(b);
			}
			write_u8((xbyte*)block_->get_data() + cursor_, b);
		}

		void				message_writer::write(u16  b)
		{
			if (_can_write(block_, cursor_, sizeof(b)))
			{
				write_u16((xbyte*)block_->get_data() + cursor_, b);
				cursor_ += sizeof(b);
			}
			write_u16((xbyte*)block_->get_data() + cursor_, b);
		}

		void				message_writer::write(s16  b)
		{
			if (_can_write(block_, cursor_, sizeof(b)))
			{
				write_s16((xbyte*)block_->get_data() + cursor_, b);
				cursor_ += sizeof(b);
			}
			write_s16((xbyte*)block_->get_data() + cursor_, b);
		}

		void				message_writer::write(u32  b)
		{
			if (_can_write(block_, cursor_, sizeof(b)))
			{
				write_u32((xbyte*)block_->get_data() + cursor_, b);
				cursor_ += sizeof(b);
			}
			write_u32((xbyte*)block_->get_data() + cursor_, b);
		}

		void				message_writer::write(s32  b)
		{
			if (_can_write(block_, cursor_, sizeof(b)))
			{
				write_s32((xbyte*)block_->get_data() + cursor_, b);
				cursor_ += sizeof(b);
			}
			write_s32((xbyte*)block_->get_data() + cursor_, b);
		}

		void				message_writer::write(u64  b)
		{
			if (_can_write(block_, cursor_, sizeof(b)))
			{
				write_u64((xbyte*)block_->get_data() + cursor_, b);
				cursor_ += sizeof(b);
			}
			write_u64((xbyte*)block_->get_data() + cursor_, b);
		}

		void				message_writer::write(s64  b)
		{
			if (_can_write(block_, cursor_, sizeof(b)))
			{
				write_s64((xbyte*)block_->get_data() + cursor_, b);
				cursor_ += sizeof(b);
			}
			write_s64((xbyte*)block_->get_data() + cursor_, b);
		}

		void				message_writer::write(f32  b)
		{
			if (_can_write(block_, cursor_, sizeof(b)))
			{
				write_f32((xbyte*)block_->get_data() + cursor_, b);
				cursor_ += sizeof(b);
			}
		}

		void				message_writer::write(f64  b)
		{
			if (_can_write(block_, cursor_, sizeof(b)))
			{
				write_f64((xbyte*)block_->get_data() + cursor_, b);
				cursor_ += sizeof(b);
			}
		}


		void				message_writer::write_data(const xbyte* _data, u32 _size)
		{
			if (_can_write(block_, cursor_, _size))
			{
				xbyte* dst = (xbyte*)block_->get_data() + cursor_;
				for (u32 i=0; i<_size; i++)
					*dst++ = *_data++;
				cursor_ += _size;
			}
		}

		void				message_writer::write_string(const char* _str, u32 _len)
		{
			if (_can_write(block_, cursor_, _len))
			{
				char* dst = (char*)block_->get_data() + cursor_;
				for (u32 i=0; i<_len; i++)
					*dst++ = *_str++;
				cursor_ += _len;
			}
		}

		void				message_writer::next_block()
		{
			if (block_ != NULL)
				block_ = block_->get_next();
		}


		/// ---------------------------------------------------------------------------------------
		/// Message
		/// ---------------------------------------------------------------------------------------
		ipeer*				message::get_from() const
		{
			return from_;
		}

		ipeer*				message::get_to() const
		{
			return to_;
		}

		u32					message::get_flags() const
		{
			return flags_;
		}


		bool				message::is_from(ipeer* p) const
		{
			return p == from_;
		}


		bool				message::has_event() const
		{
			return xbfIsSet(flags_, MESSAGE_FLAG_EVENT);
		}

		bool				message::has_data() const
		{
			return xbfIsSet(flags_, MESSAGE_FLAG_DATA);
		}


		bool				message::event_is_connected() const
		{
			u32 const e = flags_ & MESSAGE_FLAG_EVENT_MASK;
			return e == MESSAGE_FLAG_EVENT_CONNECTED;
		}

		bool				message::event_disconnected() const
		{
			u32 const e = flags_ & MESSAGE_FLAG_EVENT_MASK;
			return e == MESSAGE_FLAG_EVENT_DISCONNECTED;
		}

		bool				message::event_cannot_connect() const
		{
			u32 const e = flags_ & MESSAGE_FLAG_EVENT_MASK;
			return e == MESSAGE_FLAG_EVENT_CANNOT_CONNECT;
		}

		void				message::add_block(message_block* _block)
		{
			if (pblocks_ == NULL)
			{
				pblocks_ = _block;
			}
			else
			{
				pblocks_->enqueue(_block);
			}
			nblocks_ += 1;
		}

		message_reader		message::get_reader() const
		{
			return message_reader(pblocks_);
		}

		message_writer		message::get_writer() const
		{
			return message_writer(pblocks_);
		}

		void				message::release(imessage_allocator* a)
		{
			nblocks_ = 0;
			message_block* current = pblocks_;
			while (current != NULL)
			{
				message_block* previous = current;
				current = current->get_next();
				a->deallocate(previous);
			}
			a->deallocate(this);
		}



		/// ---------------------------------------------------------------------------------------
		/// Outgoing Message
		/// ---------------------------------------------------------------------------------------
		ipeer*				outgoing_messages::get_from() const
		{
			return message_->get_from();
		}

		ipeer*				outgoing_messages::get_to() const
		{
			return message_->get_to();
		}

		u32					outgoing_messages::get_flags() const
		{
			return message_->get_flags();
		}

		message_reader		outgoing_messages::get_reader() const
		{
			return message_->get_reader();
		}

		message_writer		outgoing_messages::get_writer() const
		{
			return message_->get_writer();
		}

		void				outgoing_messages::enqueue(message* m)
		{
			if (message_ == NULL) 
				message_ = m;
			else
				message_->enqueue(m);
		}

		message*			outgoing_messages::dequeue()
		{
			if (message_ == NULL) 
				return NULL;
			else 
				return message_->dequeue();
		}


		/// ---------------------------------------------------------------------------------------
		/// Incoming Message
		/// ---------------------------------------------------------------------------------------

		bool				incoming_messages::is_from(ipeer* from) const
		{
			return message_->is_from(from);
		}

		ipeer*				incoming_messages::get_from()
		{
			return message_->get_from();
		}

		u32					incoming_messages::get_flags() const
		{
			return message_->get_flags();
		}


		bool				incoming_messages::has_event() const
		{
			return message_->has_event();
		}

		bool				incoming_messages::has_data() const
		{
			return message_->has_data();
		}


		bool				incoming_messages::event_is_connected() const
		{
			return message_->event_is_connected();
		}

		bool				incoming_messages::event_disconnected() const
		{
			return message_->event_disconnected();
		}

		bool				incoming_messages::event_cannot_connect() const
		{
			return message_->event_cannot_connect();
		}


		message_reader		incoming_messages::get_reader() const
		{
			return message_->get_reader();
		}

		void				incoming_messages::enqueue(message* m)
		{
			if (message_ == NULL) 
				message_ = m;
			else
				message_->enqueue(m);
		}

		message*			incoming_messages::dequeue()
		{
			if (message_ == NULL) 
				return NULL;
			else 
				return message_->dequeue();
		}
	}
}