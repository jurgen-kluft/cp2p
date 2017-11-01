//==============================================================================
//  x_udx-registry.h
//==============================================================================
#ifndef __XP2P_UDX_REGISTRY_H__
#define __XP2P_UDX_REGISTRY_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

namespace xcore
{
	class udx_address;
	class udx_peer;

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] udx registry of 'address' to 'socket'
	class udx_registry
	{
	public:
		virtual void			set(udx_address* k, udx_peer* v) = 0;
		virtual udx_peer*		get(udx_address* key) = 0;
		virtual bool			del(udx_address* key) = 0;
	};


}

#endif