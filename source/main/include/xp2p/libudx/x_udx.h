//==============================================================================
//  x_udx.h
//==============================================================================
#ifndef __XP2P_UDX_H__
#define __XP2P_UDX_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif


namespace xcore
{
	class udx_peer;

	class udx_factory
	{
	public:
		virtual udx_peer*		create_peer(udx_address*) = 0;
		virtual udx_address*	create_address(void* data, u32 size) = 0;
	};

}

#endif
