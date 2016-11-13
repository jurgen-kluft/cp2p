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
		enum
		{
			DATA_SIZE = 64,
		}

		void 			from_string(const char*);
		void			get_addrin(void*& addrin, u32& addrinlen) const { addrin = m_addrin; addrinlen = m_addrinlen; }

		void 			set_peer(udx_peer*);
		udx_peer*		get_peer();

	protected:
		u32				m_addrinlen;
		u8				m_addrin[DATA_SIZE];

	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_addresses
	{
	public:
		virtual udx_address*	add(void* addrin, u32 addrinlen) = 0;
		virtual udx_address*	add(const char* addr) = 0;
		virtual udx_address*	find(void* addrin, u32 addrinlen) = 0;
	};

}

#endif
