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
		struct netip
		{
			enum etype
			{
				NETIP_NONE = 0,
				NETIP_IPV4 = 4,
				NETIP_IPV6 = 16,
			};

			inline		netip()												{ type_ = NETIP_NONE; port_ = 0; }
			inline		netip(etype _type, u16 _port, xbyte* _ip)			{ type_ = _type; port_ = _port;  for (s32 i = 0; i < type_; ++i) { ip_[i] = _ip[i]; } }

			etype		get_type() const									{ return (etype)type_; }
			u16			get_port() const									{ return port_; }

			bool		is_ip4() const										{ return (etype)type_ == NETIP_IPV4; }
			bool		is_ip6() const										{ return (etype)type_ == NETIP_IPV6; }

			bool		operator == (const netip& _other) const				{ return is_equal(_other); }
			bool		operator != (const netip& _other) const				{ return !is_equal(_other); }

			void		to_string(char* s, u32 l) const;

			bool		is_equal(const netip& ip) const
			{
				if (type_ == ip.type_)
				{
					if (port_ == ip.port_)
					{
						for (s32 i = 0; i < type_; ++i)
						{
							if (ip_[i] != ip.ip_[i])
								return false;
						}
						return true;
					}
				}
				return false;
			}

			u16			type_;
			u16			port_;
			xbyte		ip_[16];
		};
	}
}

#endif	///< __XPEER_2_PEER_TYPES_H__
