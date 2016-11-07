//==============================================================================
//  x_udx-incoming.h
//==============================================================================
#ifndef __XP2P_UDX_INCOMING_H__
#define __XP2P_UDX_INCOMING_H__
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

	// This is the incoming (receiving) packet queue, the additional functionality is the requirement
	// for building ACK data and releasing 'in-order/reliable' continues received packets.
	// - Build ACK data
	// - Dequeue received packets


}

#endif