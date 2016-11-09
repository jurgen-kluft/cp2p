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
		u32				m_index;
		u32				m_hash[4];
		u32				m_data[16];
	};

}

#endif