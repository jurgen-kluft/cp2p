//==============================================================================
//  x_udx-bitstream.h
//==============================================================================
#ifndef __XP2P_UDX_BITSTREAM_H__
#define __XP2P_UDX_BITSTREAM_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API

	// A simple bitstream class for setting and clearing bits in a stream of bits.
	// This is for example used to construct an ACK-list.
	class udx_bitstream
	{
	public:
		const u32 ALL_BITS = 0xffffffff;
		const u32 NIL_BITS = 0;
		const u32 HIGH_BIT = 0x80000000;
		const u32 BITS = 32;

		udx_bitstream(u32 maxbits, u32* stream)
			: m_maxbits(maxbits)
			, m_bitstream(stream) { }

		inline void 		set_false(u32 i)
		{
			if (i >= m_maxbits)
				return;
			u32 const n = i/BITS;
			u32 const b = HIGH_BIT>>(i&(BITS-1));
			m_bitstream[n] |= b;
		}

		inline void 		set_bit(u32 i, bool bit)
		{
			if (i >= m_maxbits)
				return;
			u32 const n = i/BITS;
			u32 const b = HIGH_BIT>>(i&(BITS-1));
			switch (bit)
			{
				case true: m_bitstream[n] |= b; break;
				case false: m_bitstream[n] &= ~b; break;
			}
		}

		inline void 		set_false(u32 i)
		{
			if (i >= m_maxbits)
				return;
			u32 const n = i/BITS;
			u32 const b = HIGH_BIT>>(i&(BITS-1));
			m_bitstream[n] &= ~b;
		}

		void 		set_range_true(u32 from, u32 to)
		{
			if (from >= to || from >= m_maxbits)
				return;

			if (to > m_maxbits)
				to = m_maxbits;

			u32 hmask = HIGH_BIT>>(from&(BITS-1));
			hmask = hmask | (hmask - 1);
			u32 lmask = HIGH_BIT>>(to  &(BITS-1));
			lmask = ~(lmask | (lmask - 1));
			u32 const c = (to/BITS) - (from/BITS);

			if (c == 0)
			{
				u32 mask = hmask & lmask;
				u32 n = (from/HIGH_BIT);
				m_bitstream[n] |= mask;
			}
			else
			{
				u32 n = (from/BITS);
				u32 end = (to/BITS);
				m_bitstream[n++] |= hmask;
				m_bitstream[end] |= lmask;

				while (n < end)
				{
					m_bitstream[n++] = ALL_BITS;
				}
			}
		}

		void 		set_range_false(u32 from, u32 to)
		{
			if (from >= to || from >= m_maxbits)
				return;

			if (to > m_maxbits)
				to = m_maxbits;

			u32 hmask = HIGH_BIT>>(from&(BITS-1));
			hmask = ~(hmask | (hmask - 1));
			u32 lmask = HIGH_BIT>>(to  &(BITS-1));
			lmask = (lmask | (lmask - 1));
			u32 const c = (to/BITS) - (from/BITS);

			if (c == 0)
			{
				u32 mask = hmask | lmask;
				u32 n = (from/BITS);
				m_bitstream[n] &= mask;
			}
			else
			{
				u32 n = (from/BITS);
				u32 end = (to/BITS);
				m_bitstream[n++] &= hmask;
				m_bitstream[end] &= lmask;

				while (n < end)
				{
					m_bitstream[n++] = NIL_BITS;
				}
			}
		}

		void 		set_range_bit(u32 from, u32 to, bool bit)
		{
			if (from >= to || from >= m_maxbits)
				return;

			if (to > m_maxbits)
				to = m_maxbits;

			u32 hmask = HIGH_BIT>>(from&(BITS-1));
			hmask = ~(hmask | (hmask - 1));
			u32 lmask = HIGH_BIT>>(to  &(BITS-1));
			lmask = (lmask | (lmask - 1));
			u32 const c = (to/BITS) - (from/BITS);

			if (c == 0)
			{
				u32 mask = hmask | lmask;
				u32 n = (from/BITS);
				switch (bit)
				{
					case true: m_bitstream[n] |= ~mask; break;
					case false: m_bitstream[n] &= mask; break;
				}
			}
			else
			{
				u32 n = (from/BITS);
				u32 end = (to/BITS);

				switch (bit)
				{
					case true:
						m_bitstream[n++] |= ~hmask;
						m_bitstream[end] |= ~lmask;
						break;
					case false:
						m_bitstream[n++] &= hmask;
						m_bitstream[end] &= lmask;
						break;
				}

				u32 const fill = bit ? ALL_BITS : NIL_BITS;
				while (n < end)
				{
					m_bitstream[n++] = fill;
				}
			}
		}

		inline void 	set_all_true()					{ u32 const l = maxbits / 64; for (u32 i=0; i<=l; i++) m_bitstream[i] = ALL_BITS; }
		inline void 	set_all_false()					{ u32 const l = maxbits / 64; for (u32 i=0; i<=l; i++) m_bitstream[i] = 0; }

		inline bool 	is_bit(u32 i, bool bit) const	{ u32 const n = i / BITS; u32 const b = HIGH_BIT>>(i&(BITS-1)); if (i < m_maxbits) return (m_bitstream[n] & b)==0 ? !bit : bit; else return false; }
		inline bool		is_true(u32 i) const			{ u32 const n = i / BITS; u32 const b = HIGH_BIT>>(i&(BITS-1)); if (i < m_maxbits) return (m_bitstream[n] & b)==b; else return false; }
		inline bool 	is_false(u32 i) const			{ u32 const n = i / BITS; u32 const b = HIGH_BIT>>(i&(BITS-1)); if (i < m_maxbits) return (m_bitstream[n] & b)==0; else return false; }

		bool		is_range_true(u32 from, u32 to) const
		{
			if (from >= to || from >= m_maxbits)
				return true;

			if (to > m_maxbits)
				to = m_maxbits;

			u32 hmask = HIGH_BIT>>(from&(BITS-1));
			hmask = (hmask | (hmask - 1));
			u32 lmask = HIGH_BIT>>(to  &(BITS-1));
			lmask = ~(lmask | (lmask - 1));
			u32 const c = (to/BITS) - (from/BITS);

			if (c == 0)
			{
				u32 mask = hmask & lmask;
				u32 n = (from/BITS);
				return (m_bitstream[n] & mask) == mask;
			}
			else
			{
				u32 n = (from/BITS);
				u32 end = (to/BITS);
				if ((m_bitstream[n++] & hmask) != hmask)
					return false;
				if ((m_bitstream[end] & lmask) != lmask)
					return false;

				while (n < end)
				{
					if (m_bitstream[n++] != ALL_BITS)
						return false;
				}
				return true;
			}
		}

		bool		is_range_false(u32 from, u32 to) const
		{
			if (from >= to || from >= m_maxbits)
				return true;

			if (to > m_maxbits)
				to = m_maxbits;

			u32 hmask = HIGH_BIT>>(from&(BITS-1));
			hmask = (hmask | (hmask - 1));
			u32 lmask = HIGH_BIT>>(to  &(BITS-1));
			lmask = ~(lmask | (lmask - 1));
			u32 const c = (to/BITS) - (from/BITS);

			if (c == 0)
			{
				u32 mask = hmask & lmask;
				u32 n = (from/BITS);
				return (m_bitstream[n] & mask) == 0;
			}
			else
			{
				u32 n = (from/BITS);
				u32 end = (to/BITS);
				if ((m_bitstream[n++] & hmask) != 0)
					return false;
				if ((m_bitstream[end] & lmask) != 0)
					return false;

				while (n < end)
				{
					if (m_bitstream[n++] != 0)
						return false;
				}
				return true;
			}
		}

		void 		copy_range(u32 iter, u32 count, udx_bitstream& dst) const
		{
			u32 end = iter + count;
			u32 diter = 0;
			while (iter < end)
			{
				u32 len = 1;
				bool bit = is_set(iter++);
				while (test(iter) == bit && iter < end)
				{
					++len;
					++iter;
				}

				if (len > 1)
				{
					dst.set_range_bit(diter, len, bit);
				}
				else
				{
					dst.set_bit(diter, bit);
				}
				diter += len;
			}
		}

		void 		compact_range_RLE(u32 iter, u32 count, udx_bitstream& dest, u32 maxtrue, u32 maxfalse) const
		{
			u32 end = iter + count;
			u32 diter = 0;
			while (iter < end)
			{
				bool bit = is_set(iter++);
				u32 len = (bit ? maxtrue : maxfalse) - 1;
				while (is_set(iter) == bit && iter < end && len >= 0)
				{
					--len;
					++iter;
				}

				dest.set_bit(diter, bit);

				if (dest == dest_end)
					break;
			}
		}

	protected:
		u32 			m_maxbits;
		u32				*m_bitstream;
	};


}

#endif
