//==============================================================================
//  x_udx.h
//==============================================================================
#ifndef __XP2P_UDX_H__
#define __XP2P_UDX_H__
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
	// [PUBLIC] API
	class udx_time
	{
	public:
		static u64		get_time_us();
	};

	
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	struct udx_message
	{
		inline			udx_message(void* data, u32 size) : data_ptr(data), data_size(size) {}
		void*			data_ptr;
		u32				data_size;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_alloc
	{
	public:
		virtual void*		alloc(u32 _size) = 0;
		virtual void		commit(void*, u32 _size) = 0;
		virtual void		dealloc(void*) = 0;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_filter
	{
	public:
		virtual void	init(u64* window, u32 size);

		virtual u64		add(u64 value) = 0;
		virtual u64		get() const = 0;
	};



}

#endif