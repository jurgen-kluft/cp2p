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
		struct NetIP4 
		{
			inline		NetIP4() { ip_[0] = 127; ip_[1] = 0; ip_[2] = 0; ip_[3] = 1; port_ = 0; }
			inline		NetIP4(xbyte n4, xbyte n3, xbyte n2, xbyte n1) { ip_[0] = n4; ip_[1] = n3; ip_[2] = n2; ip_[3] = n1; port_ = 0; }

			NetIP4&		Port(u16 _port)				{ port_ = _port; return *this; }
			void		ToString(char* s, u32 l)
			{
				s32 i = 0;
				/// @TODO: implement
			}
			xbyte		ip_[4]; 
			u16			port_;
		};

		typedef void*				NetAddress;
		typedef u32					PeerID;

		typedef void*				MsgHandle;
	}
}

#endif	///< __XPEER_2_PEER_TYPES_H__
