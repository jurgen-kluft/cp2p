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
	struct udx_addrin
	{
		u32		m_len;
		u8		m_data[64];

		s32		from_string(const char* str);
		s32		to_string(char* str, u32 maxstrlen) const;
	};

	struct udx_hash
	{
		u32		m_len;
		u8		m_hash[32];
	};


	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_address
	{
	public:
		virtual bool				from_string(char* str) const = 0;
		virtual s32					to_string(char* str, u32 maxstrlen) const = 0;
		virtual udx_addrin const&	get_addrin() const = 0;
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
		virtual	bool			get_assoc(void* addrin, u32 addrinlen, udx_address*& assoc) = 0;
		virtual	void			set_assoc(void* addrin, u32 addrinlen, udx_address* addr) = 0;
		virtual	void			del_assoc(void* addrin, u32 addrinlen) = 0;
	};

	class udx_iaddress2idx
	{
	public:
		virtual	bool			get_assoc(udx_address* address, u32& assoc) = 0;
		virtual	void			set_assoc(udx_address* address, u32 idx) = 0;
		virtual	void			del_assoc(udx_address* address) = 0;
	};

	class udx_ihashing
	{
	public:
		virtual udx_hash		compute_hash(void* addrin, u32 addrinlen) = 0;
	};

	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API
	class udx_address_factory : public udx_iaddress_factory, public udx_iaddrin2address, public udx_iaddress2idx, public udx_ihashing
	{
	public:
		virtual udx_address*	create(void* addrin, u32 addrinlen);
		virtual void			destroy(udx_address*);

		virtual	bool			get_assoc(void* addrin, u32 addrinlen, udx_address*& assoc);
		virtual	void			set_assoc(void* addrin, u32 addrinlen, udx_address* addr);
		virtual	void			del_assoc(void* addrin, u32 addrinlen);

		virtual	bool			get_assoc(udx_address* address, u32& assoc);
		virtual	void			set_assoc(udx_address* address, u32 idx);
		virtual	void			del_assoc(udx_address* address);

		void*					operator new(xcore::xsize_t num_bytes, void* mem) { return mem; }
		void					operator delete(void* mem, void*) { }
		void*					operator new(xcore::xsize_t num_bytes) { return NULL; }
		void					operator delete(void* mem) { }

	private:
		virtual udx_hash		compute_hash(void* addrin, u32 addrinlen);
	};
}

#endif
