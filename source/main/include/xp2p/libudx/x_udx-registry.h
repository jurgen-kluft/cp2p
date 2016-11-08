//==============================================================================
//  x_udx-registry.h
//==============================================================================
#ifndef __XP2P_UDX_REGISTRY_H__
#define __XP2P_UDX_REGISTRY_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xbase\x_allocator.h"

namespace xcore
{
	class udx_address;
	class udx_socket;

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] udx registry of 'address' to 'socket'
	class udx_registry
	{
	public:
		virtual udx_address*	find(void const* data, u32 size) const = 0;
		virtual udx_address*	add(void const* data, u32 size) = 0;
		virtual udx_socket*		find(udx_address* key) = 0;
		virtual void			add(udx_address* k, udx_socket* v) = 0;
	};


}

#endif