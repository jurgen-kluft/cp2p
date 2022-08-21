#ifndef __UTP_PACKEDSOCKADDR_H__
#define __UTP_PACKEDSOCKADDR_H__

#include "xp2p/libutp/utp_types.h"

struct PACKED_ATTRIBUTE PackedSockAddr 
{
	// The values are always stored here in network byte order
	union 
	{
		byte _in6[16];		// IPv6
		uint16 _in6w[8];	// IPv6, word based (for convenience)
		uint32 _in6d[4];	// Dword access
		in6_addr _in6addr;	// For convenience
	} _in;

	// Host byte order
	uint16 _port;

	#define _sin4 _in._in6d[3]	// IPv4 is stored where it goes if mapped

	#define _sin6 _in._in6
	#define _sin6w _in._in6w
	#define _sin6d _in._in6d

	byte get_family() const;
	bool operator==(const PackedSockAddr& rhs) const;
	bool operator!=(const PackedSockAddr& rhs) const;
	void set(const SOCKADDR_STORAGE* sa, socklen_t len);

	PackedSockAddr(const SOCKADDR_STORAGE* sa, socklen_t len);
	PackedSockAddr(void);

	SOCKADDR_STORAGE get_sockaddr_storage(socklen_t *len) const;
	cstr fmt(str s, size_t len) const;

	uint32 compute_hash() const;
} ALIGNED_ATTRIBUTE(4);

#endif //__UTP_PACKEDSOCKADDR_H__
