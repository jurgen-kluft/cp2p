//==============================================================================
//  x_sha1.h
//==============================================================================
#ifndef __XP2P_SHA1_H__
#define __XP2P_SHA1_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

namespace xcore
{
	struct SHA1_CTX
	{
		u64 bitlen;
		u8 data[64];
		u32 datalen;
		u32 state[5];
		u32 k[4];
	};

	// API
	void sha1_init(SHA1_CTX *ctx);
	void sha1_update(SHA1_CTX *ctx, const u8 data[], u32 len);
	void sha1_final(SHA1_CTX *ctx, u8 hash[]);
}

#endif	///< __XP2P_SHA1_H__
