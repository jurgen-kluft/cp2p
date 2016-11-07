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
	class udx_address
	{
	public:
		u32				m_index;
		u32				m_hash[4];
		u32				m_data[16];
	};
	
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	struct udx_message
	{
		void*			data_ptr;
		u32				data_size;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	struct udx_packet_info;
	struct udx_packet_hdr;

	struct udx_packet	// align(8)
	{

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
	// [PUBLIC] udx registry of 'address' to 'socket'
	class udx_registry
	{
	public:
		virtual udx_address*	find(void const* data, u32 size) const = 0;
		virtual udx_address*	add(void const* data, u32 size) = 0;
		virtual udx_socket*		find(udx_address* key) = 0;
		virtual void			add(udx_address* k, udx_socket* v) = 0;
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