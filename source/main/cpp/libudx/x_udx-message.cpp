#include "xbase\x_target.h"

#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-message.h"

namespace xcore
{
	/// ---------------------------------------------------------------------------------------
	/// Unaligned Data Reading/Writing
	/// ---------------------------------------------------------------------------------------
	inline u8			read_u8(xbyte const* ptr) { return (u8)*ptr; }
	inline s8			read_s8(xbyte const* ptr) { return (s8)*ptr; }
	inline u16			read_u16(xbyte const* ptr) { u16 b0 = *ptr++; u16 b1 = *ptr++; return (u16)((b1 << 8) | b0); }
	inline s16			read_s16(xbyte const* ptr) { u16 b0 = *ptr++; u16 b1 = *ptr++; return (s16)((b1 << 8) | b0); }
	inline u32			read_u32(xbyte const* ptr) { u32 b0 = *ptr++; u32 b1 = *ptr++; u32 b2 = *ptr++; u32 b3 = *ptr++; return (u32)((b3 << 24) | (b2 << 16) | (b1 << 8) | (b0)); }
	inline s32			read_s32(xbyte const* ptr) { u32 b0 = *ptr++; u32 b1 = *ptr++; u32 b2 = *ptr++; u32 b3 = *ptr++; return (s32)((b3 << 24) | (b2 << 16) | (b1 << 8) | (b0)); }
	inline f32			read_f32(xbyte const* ptr) { u32 b0 = *ptr++; u32 b1 = *ptr++; u32 b2 = *ptr++; u32 b3 = *ptr++; u32 u = ((b3 << 24) | (b2 << 16) | (b1 << 8) | (b0)); return *((f32*)&u); }
	inline u64			read_u64(xbyte const* ptr) { u64 l0 = read_u32(ptr); u64 l1 = read_u32(ptr + 4); return (u64)((l1 << 32) | l0); }
	inline s64			read_s64(xbyte const* ptr) { u64 l0 = read_u32(ptr); u64 l1 = read_u32(ptr + 4); return (s64)((l1 << 32) | l0); }
	inline f64			read_f64(xbyte const* ptr) { u64 l0 = read_u32(ptr); u64 l1 = read_u32(ptr + 4); u64 ll = (u64)((l1 << 32) | l0); return *((f64*)ll); }

	inline void			write_u8(xbyte * ptr, u8  b) { *ptr = b; }
	inline void			write_s8(xbyte * ptr, s8  b) { *ptr = b; }
	inline void			write_u16(xbyte * ptr, u16 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_s16(xbyte * ptr, s16 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_u32(xbyte * ptr, u32 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_s32(xbyte * ptr, s32 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_f32(xbyte * ptr, f32 f) { u32 b = *((u32*)&f); for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_u64(xbyte * ptr, u64 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_s64(xbyte * ptr, s64 b) { for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }
	inline void			write_f64(xbyte * ptr, f64 f) { u64 b = *((u64*)&f); for (s32 i = 0; i<sizeof(b); ++i) { ptr[i] = (u8)b; b = b >> 8; } }

	/// ---------------------------------------------------------------------------------------
	/// Message Reader
	/// ---------------------------------------------------------------------------------------
	inline bool			_can_read(udx_msg const& _msg, u32 _cursor, u32 _num_bytes)
	{
		if (_msg.data_ptr == NULL) return false;
		else return (_cursor + _num_bytes) <= _msg.data_size;
	}

	void				udx_msg_reader::set_cursor(u32 c)
	{
		m_cursor = c;
		if (m_cursor >= m_msg.data_size)
			m_cursor = m_msg.data_size;
	}
	u32					udx_msg_reader::get_cursor() const
	{
		return m_cursor;
	}

	u32					udx_msg_reader::get_size() const
	{
		if (m_msg.data_ptr == NULL) return 0;
		return m_msg.data_size - m_cursor;
	}

	bool				udx_msg_reader::can_read(u32 number_of_bytes) const
	{
		return _can_read(m_msg, m_cursor, number_of_bytes);
	}


	u32					udx_msg_reader::read(bool& b)
	{
		if (_can_read(m_msg, m_cursor, 1))
		{
			u8 const* ptr = (u8 const*)m_msg.data_ptr + m_cursor;
			m_cursor += 1;
			b = read_u8(ptr) != 0;
			return 1;
		}
		return 0;
	}

	u32					udx_msg_reader::read(u8  & b)
	{
		if (_can_read(m_msg, m_cursor, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)m_msg.data_ptr + m_cursor;
			m_cursor += sizeof(b);
			b = read_u8(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_reader::read(s8  & b)
	{
		if (_can_read(m_msg, m_cursor, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)m_msg.data_ptr + m_cursor;
			m_cursor += sizeof(b);
			b = read_s8(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_reader::read(u16 & b)
	{
		if (_can_read(m_msg, m_cursor, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)m_msg.data_ptr + m_cursor;
			m_cursor += sizeof(b);
			b = read_u16(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_reader::read(s16 & b)
	{
		if (_can_read(m_msg, m_cursor, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)m_msg.data_ptr + m_cursor;
			m_cursor += sizeof(b);
			b = read_s16(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_reader::read(u32 & b)
	{
		if (_can_read(m_msg, m_cursor, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)m_msg.data_ptr + m_cursor;
			m_cursor += sizeof(b);
			b = read_u32(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_reader::read(s32 & b)
	{
		if (_can_read(m_msg, m_cursor, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)m_msg.data_ptr + m_cursor;
			m_cursor += sizeof(b);
			b = read_s32(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_reader::read(u64 & b)
	{
		if (_can_read(m_msg, m_cursor, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)m_msg.data_ptr + m_cursor;
			m_cursor += sizeof(b);
			b = read_u64(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_reader::read(s64 & b)
	{
		if (_can_read(m_msg, m_cursor, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)m_msg.data_ptr + m_cursor;
			m_cursor += sizeof(b);
			b = read_s64(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_reader::read(f32 & b)
	{
		if (_can_read(m_msg, m_cursor, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)m_msg.data_ptr + m_cursor;
			m_cursor += sizeof(b);
			b = read_f32(ptr);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_reader::read(f64 & b)
	{
		if (_can_read(m_msg, m_cursor, sizeof(b)))
		{
			u8 const* ptr = (u8 const*)m_msg.data_ptr + m_cursor;
			m_cursor += sizeof(b);
			b = read_f64(ptr);
			return sizeof(b);
		}
		return 0;
	}


	bool				udx_msg_reader::view_data(xbyte const*& data, u32 size)
	{
		if (!_can_read(m_msg, m_cursor, size))
		{
			data = NULL;
			return false;
		}
		data = (xbyte const*)m_msg.data_ptr + m_cursor;
		m_cursor += size;
		return true;
	}

	bool				udx_msg_reader::view_string(char const*& str, u32& strlen)
	{
		if (m_msg.data_ptr == NULL)
			return false;

		strlen = 0;
		str = (char const*)m_msg.data_ptr + m_cursor;
		while (m_cursor < m_msg.data_size)
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

		m_cursor += strlen;
		return true;
	}

	/// ---------------------------------------------------------------------------------------
	/// Message Writer
	/// ---------------------------------------------------------------------------------------
	inline bool			_can_write(udx_msg const& _msg, u32 _cursor, u32 _num_bytes)
	{
		if (_msg.data_ptr == NULL) return false;
		else return (_cursor + _num_bytes) <= _msg.data_size;
	}

	void				udx_msg_writer::set_cursor(u32 c)
	{
		m_cursor = c;
		if (m_cursor > m_msg.data_size)
			m_cursor = m_msg.data_size;
	}

	u32					udx_msg_writer::get_cursor() const
	{
		return m_cursor;
	}


	bool				udx_msg_writer::can_write(u32 num_bytes) const
	{
		return _can_write(m_msg, m_cursor, num_bytes);
	}


	u32					udx_msg_writer::write(bool b)
	{
		if (_can_write(m_msg, m_cursor, 1))
		{
			write_u8((xbyte*)m_msg.data_ptr + m_cursor, b ? 1 : 0);
			m_cursor += 1;
			return 1;
		}
		return 0;
	}

	u32					udx_msg_writer::write(u8   b)
	{
		if (_can_write(m_msg, m_cursor, sizeof(b)))
		{
			write_u8((xbyte*)m_msg.data_ptr + m_cursor, b);
			m_cursor += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_writer::write(s8   b)
	{
		if (_can_write(m_msg, m_cursor, sizeof(b)))
		{
			write_u8((xbyte*)m_msg.data_ptr + m_cursor, b);
			m_cursor += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_writer::write(u16  b)
	{
		if (_can_write(m_msg, m_cursor, sizeof(b)))
		{
			write_u16((xbyte*)m_msg.data_ptr + m_cursor, b);
			m_cursor += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_writer::write(s16  b)
	{
		if (_can_write(m_msg, m_cursor, sizeof(b)))
		{
			write_s16((xbyte*)m_msg.data_ptr + m_cursor, b);
			m_cursor += sizeof(b);
		}
		return sizeof(b);
	}

	u32					udx_msg_writer::write(u32  b)
	{
		if (_can_write(m_msg, m_cursor, sizeof(b)))
		{
			write_u32((xbyte*)m_msg.data_ptr + m_cursor, b);
			m_cursor += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_writer::write(s32  b)
	{
		if (_can_write(m_msg, m_cursor, sizeof(b)))
		{
			write_s32((xbyte*)m_msg.data_ptr + m_cursor, b);
			m_cursor += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_writer::write(u64  b)
	{
		if (_can_write(m_msg, m_cursor, sizeof(b)))
		{
			write_u64((xbyte*)m_msg.data_ptr + m_cursor, b);
			m_cursor += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_writer::write(s64  b)
	{
		if (_can_write(m_msg, m_cursor, sizeof(b)))
		{
			write_s64((xbyte*)m_msg.data_ptr + m_cursor, b);
			m_cursor += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_writer::write(f32  b)
	{
		if (_can_write(m_msg, m_cursor, sizeof(b)))
		{
			write_f32((xbyte*)m_msg.data_ptr + m_cursor, b);
			m_cursor += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}

	u32					udx_msg_writer::write(f64  b)
	{
		if (_can_write(m_msg, m_cursor, sizeof(b)))
		{
			write_f64((xbyte*)m_msg.data_ptr + m_cursor, b);
			m_cursor += sizeof(b);
			return sizeof(b);
		}
		return 0;
	}


	u32					udx_msg_writer::write_data(const xbyte* _data, u32 _size)
	{
		if (_can_write(m_msg, m_cursor, _size))
		{
			xbyte* dst = (xbyte*)m_msg.data_ptr + m_cursor;
			for (u32 i = 0; i<_size; i++)
				*dst++ = *_data++;
			m_cursor += _size;
			return _size;
		}
		return 0;
	}

	u32					udx_msg_writer::write_string(const char* _str, u32 _len)
	{
		if (_can_write(m_msg, m_cursor, _len))
		{
			char* dst = (char*)m_msg.data_ptr + m_cursor;
			for (u32 i = 0; i<_len; i++)
				*dst++ = *_str++;
			m_cursor += _len;
			return _len;
		}
		return 0;
	}


}
