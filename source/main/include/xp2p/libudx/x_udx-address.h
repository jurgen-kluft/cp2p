//==============================================================================
//  x_udx-address.h
//==============================================================================
#ifndef __XP2P_UDX_ADDRESS_H__
#define __XP2P_UDX_ADDRESS_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_address
	{
	public:
		virtual void			to_string(char* str, u32 maxlen) const = 0;
	};

	class udx_iaddress_factory
	{
	public:
		virtual udx_address*	create(void* addrin, u32 addrinlen) = 0;
		virtual void			destroy(udx_address*) = 0;
	};

	class udx_iaddrin2address
	{
	public:
		virtual	udx_address*	get_assoc(void* addrin, u32 addrinlen) const = 0;
		virtual	void			set_assoc(void* addrin, u32 addrinlen, udx_address* addr) = 0;
	};

	class udx_iaddress2peer
	{
	public:
		virtual	udx_peer*		get_assoc(udx_address* address) const = 0;
		virtual	void			set_assoc(udx_address* address, udx_peer* peer) = 0;
	};

	struct udx_addrin
	{
		u32		m_len;
		u8		m_data[64];
	};

	struct udx_hash
	{
		u32		m_len;
		u8		m_hash[32];
	};

	class udx_ihashing
	{
	public:
		virtual udx_hash		compute_hash(void* addrin, u32 addrinlen) = 0;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_address_factory : public udx_iaddrin2address, public udx_iaddress2peer, public udx_ihashing
	{
	public:
		virtual udx_address*	create(void* addrin, u32 addrinlen);
		virtual void			destroy(udx_address*);

		virtual	udx_address*	get_assoc(void* addrin, u32 addrinlen);
		virtual	void			set_assoc(void* addrin, u32 addrinlen, udx_address* addr);

		virtual	udx_peer*		get_assoc(udx_address* address);
		virtual	void			set_assoc(udx_address* address, udx_peer* peer);

	private:
		virtual udx_hash		compute_hash(void* addrin, u32 addrinlen);
	};
}

#endif
