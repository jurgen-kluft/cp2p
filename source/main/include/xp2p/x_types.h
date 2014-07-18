//==============================================================================
//  x_types.h
//==============================================================================
#ifndef __XPEER_2_PEER_TYPES_H__
#define __XPEER_2_PEER_TYPES_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

namespace xcore
{
	// ==============================================================================================================================
	// ==============================================================================================================================
	// ==============================================================================================================================
	namespace xp2p
	{
		struct netip4 
		{
			inline		netip4()											{ ip_.uip_ = 0; port_ = 0; }
			inline		netip4(xbyte n4, xbyte n3, xbyte n2, xbyte n1)		{ ip_.aip_[0] = n4; ip_.aip_[1] = n3; ip_.aip_[2] = n2; ip_.aip_[3] = n1; port_ = 0; }

			netip4&		set_port(u16 _port)									{ port_ = _port; return *this; }
			u16			get_port() const									{ return port_; }

			bool		operator == (const netip4& _other) const			{ return port_==_other.port_ && ip_.uip_==_other.ip_.uip_; }
			bool		operator != (const netip4& _other) const			{ return port_!=_other.port_ || ip_.uip_!=_other.ip_.uip_; }

			void		to_string(char* s, u32 l)
			{
				s32 i = 0;
				/// @TODO: implement
			}
			
			union ip
			{
				u32			uip_;
				xbyte		aip_[4];
			};

			ip			ip_;
			u16			port_;
		};

		typedef u32					peerid;
	}
}

#endif	///< __XPEER_2_PEER_TYPES_H__
