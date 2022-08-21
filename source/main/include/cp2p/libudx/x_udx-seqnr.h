//==============================================================================
//  x_udx-seqnr.h
//==============================================================================
#ifndef __XP2P_UDX_SEQNR_H__
#define __XP2P_UDX_SEQNR_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xbase/x_allocator.h"

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
		const static u64 MASK_LOW = 0xFFFFFFFF >> (32 - NUM_BITS);
		const static u64 MASK_HIGH = 0xFFFFFFFFFFFFFFFFUL << NUM_BITS;

		inline		udx_seqnr() : m_seq_nr(NIL)						{ }
		inline		udx_seqnr(u64 seqnr) : m_seq_nr(seqnr)			{ }
		inline		udx_seqnr(const udx_seqnr& o) : m_seq_nr(o.m_seq_nr) { }

		bool		nill() const									{ return m_seq_nr == NIL; }
		u64			get() const										{ return m_seq_nr; }

		u64			inc()											{ m_seq_nr++; }
		u64			dec()											{ m_seq_nr--; }

		u32			to_pktseqnr() const								{ return get() & MASK_LOW; }

		udx_seqnr	operator =  (const udx_seqnr& other)			{ m_seq_nr = other.m_seq_nr; return *this; }

		bool		operator == (const udx_seqnr& other) const		{ return get() == other.get(); }
		bool		operator != (const udx_seqnr& other) const		{ return get() != other.get(); }
		bool		operator <  (const udx_seqnr& other) const		{ return get() <  other.get(); }
		bool		operator >  (const udx_seqnr& other) const		{ return get() >  other.get(); }
		bool		operator <= (const udx_seqnr& other) const		{ return get() <= other.get(); }
		bool		operator >= (const udx_seqnr& other) const		{ return get() >= other.get(); }

		udx_seqnr	operator  - (const udx_seqnr& other) const		{ return udx_seqnr(get() - other.get()); }

		static udx_seqnr	nil()									{ return udx_seqnr(NIL); }
		static udx_seqnr	min()									{ return udx_seqnr(MIN); }
		static udx_seqnr	max()									{ return udx_seqnr(MAX); }

	private:
		u64			m_seq_nr;
	};

	// Outgoing packets get a continues sequence number
	struct udx_seqnrs_out
	{
		udx_seqnr	get()
		{
			udx_seqnr seqnr(m_current_seq);
			m_current_seq++;
			return seqnr;
		}
		
	private:
		u64			m_current_seq;
	};

	// Incoming packets carry 24-bit sequence numbers and they need to
	// be converted into a continues 64-bit sequence number.
	struct udx_seqnrs_in
	{
		const static u32 NUM_BITS = udx_seqnr::NUM_BITS;
		const static u64 MASK_LOW = 0xFFFFFFFF >> (32 - NUM_BITS);
		const static u64 ONE = 1UL << (NUM_BITS);
		const static u32 HALF = 1 << (NUM_BITS - 1);
		const static u64 THRESHOLD = 1 << (NUM_BITS - 2);

		udx_seqnr	get(u32 seqnr)
		{
			detect_carry(seqnr);
			if (seqnr < HALF)
				return udx_seqnr(((u64)seqnr & MASK_LOW) + m_current_seq + m_current_carry);
			else
				return udx_seqnr(((u64)seqnr & MASK_LOW) + m_current_seq);
		}

	private:
		u64			m_below_half_count;
		u64			m_above_half_count;
		u64			m_current_carry;
		u64			m_current_seq;

		void		detect_carry(u32 seqnr)
		{
			// Logic here is to detect wrap around of the N-bit seqnr and put it into
			// a 64-bit seqnr that is incremental and never wraps.
			if (seqnr < HALF)
			{
				m_below_half_count += 1;
				m_above_half_count = 0;
			}
			else
			{
				m_above_half_count += 1;
				m_below_half_count = 0;
			}

			if (m_current_carry == 0 && m_above_half_count == THRESHOLD)
			{
				m_current_carry = ONE;
			}
			else if (m_current_carry != 0 && m_below_half_count == THRESHOLD)
			{
				m_current_seq += m_current_carry;
				m_current_carry = 0;
			}
		}
	};


}

#endif