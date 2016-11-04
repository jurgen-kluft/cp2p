//==============================================================================
//  x_udx-seqnr.h
//==============================================================================
#ifndef __XP2P_UDX_SEQNR_H__
#define __XP2P_UDX_SEQNR_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xbase\x_allocator.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API

	// Internally we calculate with a sequence number that is 64-bit so we do not have to consider
	// any wrap-around logic except when converting a sequence number from a packet to an internal
	// sequence number.
	struct udx_seqnr
	{
		const static u32 NUM_BITS = 24;
		const static u64 NIL = 0xFFFFFFFFFFFFFFFFUL;
		const static u64 MAX = 0x7FFFFFFFFFFFFFFFUL;
		const static u64 MIN = 0;

					udx_seqnr(u64 seqnr) : m_seq_nr(seqnr)			{ }

		u64			get() const										{ return m_seq_nr; }

		u64			inc()											{ m_seq_nr++; }
		u64			dec()											{ m_seq_nr--; }

		bool		operator == (const udx_seqnr& other) const		{ return get() == other.get(); }
		bool		operator != (const udx_seqnr& other) const		{ return get() != other.get(); }
		bool		operator <  (const udx_seqnr& other) const		{ return get() <  other.get(); }
		bool		operator >  (const udx_seqnr& other) const		{ return get() >  other.get(); }
		bool		operator <= (const udx_seqnr& other) const		{ return get() <= other.get(); }
		bool		operator >= (const udx_seqnr& other) const		{ return get() >= other.get(); }

		static udx_seqnr	nil()									{ return udx_seqnr(NIL); }
		static udx_seqnr	min()									{ return udx_seqnr(MIN); }
		static udx_seqnr	max()									{ return udx_seqnr(MAX); }

	private:
		u64			m_seq_nr;
	};

	struct udx_seqnrs
	{
		const static u32 NUM_BITS = udx_seqnr::NUM_BITS;
		const static u64 MASK_LOW = 0xFFFFFFFF >> (32 - NUM_BITS);
		const static u64 MASK_HIGH = 0xFFFFFFFFFFFFFFFFUL << NUM_BITS;
		const static u64 ONE = 1 << NUM_BITS;
		const static u64 HALF = 1 << (NUM_BITS - 1);

		udx_seqnr	new_seqnr()
		{
			udx_seqnr seqnr(m_current_seq);
			m_current_seq++;
			return seqnr;
		}
		
		udx_seqnr	from_pktseqnr(u32 seqnr) const
		{
			if (seqnr < HALF)
			{
				seqnr = seqnr + (m_current_seq & MASK_HIGH) + ONE;
			}
			else
			{
				seqnr = seqnr + (m_current_seq & MASK_HIGH);
			}
		}

		u32			to_pktseqnr(udx_seqnr seqnr) const
		{
			return seqnr.get() & MASK_LOW;
		}

	private:
		u64			m_current_seq;
	};


}

#endif