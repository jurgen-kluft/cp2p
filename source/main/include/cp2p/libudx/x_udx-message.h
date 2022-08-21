//==============================================================================
//  x_udx-message.h
//==============================================================================
#ifndef __XP2P_UDX_MESSAGE_H__
#define __XP2P_UDX_MESSAGE_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	struct udx_msg
	{
		inline			udx_msg(void* data, u32 size) : data_ptr(data), data_size(size) {}
		void*			data_ptr;
		u32				data_size;
	};

	class udx_msg_reader
	{
	public:
		inline				udx_msg_reader(udx_msg const& _msg) : m_msg(_msg) { }

		void				set_cursor(u32);
		u32					get_cursor() const;

		u32					get_size() const;

		bool				can_read(u32 number_of_bytes) const;		// check if we still can read n number of bytes

		u32					read(bool&);
		u32					read(u8  &);
		u32					read(s8  &);
		u32					read(u16 &);
		u32					read(s16 &);
		u32					read(u32 &);
		u32					read(s32 &);
		u32					read(u64 &);
		u32					read(s64 &);
		u32					read(f32 &);
		u32					read(f64 &);

		bool				view_data(xbyte const*& _data, u32 _size);
		bool				view_string(char const*& _str, u32& _out_len);

	protected:
		u32					m_cursor;
		udx_msg				m_msg;
	};

	class udx_msg_writer
	{
	public:
		inline				udx_msg_writer(udx_msg const& _msg) : m_msg(_msg) { }

		void				set_cursor(u32);
		u32					get_cursor() const;

		bool				can_write(u32 num_bytes = 0) const;

		u32					write(bool);
		u32					write(u8);
		u32					write(s8);
		u32					write(u16);
		u32					write(s16);
		u32					write(u32);
		u32					write(s32);
		u32					write(u64);
		u32					write(s64);
		u32					write(f32);
		u32					write(f64);

		u32					write_data(const xbyte*, u32);
		u32					write_string(const char*, u32 _len = 0);

	protected:
		u32					m_cursor;
		udx_msg				m_msg;
	};
}

#endif