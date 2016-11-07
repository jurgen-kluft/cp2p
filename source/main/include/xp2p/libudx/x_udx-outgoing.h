//==============================================================================
//  x_udx-outgoing.h
//==============================================================================
#ifndef __XP2P_UDX_OUTGOING_H__
#define __XP2P_UDX_OUTGOING_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\libudx\x_udx-packet.h"
#include "xp2p\libudx\x_udx-packetqueue.h"

namespace xcore
{
	// --------------------------------------------------------------------------------------------
	// [PUBLIC] API

	// This is the outgoing (sending) packet queue, the additional functionality is the requirement
	// for dealing with received ACK data and releasing packages that are confirmed as received.
	// - Process ACK data
	// - Release packets that are sent

}

#endif