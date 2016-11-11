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
		void		get(void*& addrin, u32& addrinlen);
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_addresses
	{
	public:
		virtual udx_address*	add(void* addrin, u32 addrinlen) = 0;
		virtual udx_address*	find(void* addrin, u32 addrinlen) = 0;
	};

}

#endif