#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "xp2p/libutp/utp_types.h"
#include "xp2p/libutp/utp_hash.h"
#include "xp2p/libutp/utp_packedsockaddr.h"

#include "xp2p/libutp/libutp_inet_ntop.h"

byte PackedSockAddr::get_family() const
{
	#if defined(__sh__)
		return ((_sin6d[0] == 0) && (_sin6d[1] == 0) && (_sin6d[2] == htonl(0xffff)) != 0) ?
			AF_INET : AF_INET6;
	#else
		return (IN6_IS_ADDR_V4MAPPED(&_in._in6addr) != 0) ? AF_INET : AF_INET6;
	#endif // defined(__sh__)
}

bool PackedSockAddr::operator==(const PackedSockAddr& rhs) const
{
	if (&rhs == this)
		return true;
	if (_port != rhs._port)
		return false;
	return memcmp(_sin6, rhs._sin6, sizeof(_sin6)) == 0;
}

bool PackedSockAddr::operator!=(const PackedSockAddr& rhs) const
{
	return !(*this == rhs);
}

uint32 PackedSockAddr::compute_hash() const {
	return utp_hash_mem(&_in, sizeof(_in)) ^ _port;
}

void PackedSockAddr::set(const SOCKADDR_STORAGE* sa, socklen_t len)
{
	if (sa->ss_family == AF_INET) {
		assert(len >= sizeof(sockaddr_in));
		const sockaddr_in *sin = (sockaddr_in*)sa;
		_sin6w[0] = 0;
		_sin6w[1] = 0;
		_sin6w[2] = 0;
		_sin6w[3] = 0;
		_sin6w[4] = 0;
		_sin6w[5] = 0xffff;
		_sin4 = sin->sin_addr.s_addr;
		_port = ntohs(sin->sin_port);
	} else {
		assert(len >= sizeof(sockaddr_in6));
		const sockaddr_in6 *sin6 = (sockaddr_in6*)sa;
		_in._in6addr = sin6->sin6_addr;
		_port = ntohs(sin6->sin6_port);
	}
}

PackedSockAddr::PackedSockAddr(const SOCKADDR_STORAGE* sa, socklen_t len)
{
	set(sa, len);
}

PackedSockAddr::PackedSockAddr(void)
{
	SOCKADDR_STORAGE sa;
	socklen_t len = sizeof(SOCKADDR_STORAGE);
	memset(&sa, 0, len);
	sa.ss_family = AF_INET;
	set(&sa, len);
}

SOCKADDR_STORAGE PackedSockAddr::get_sockaddr_storage(socklen_t *len = NULL) const
{
	SOCKADDR_STORAGE sa;
	const byte family = get_family();
	if (family == AF_INET) {
		sockaddr_in *sin = (sockaddr_in*)&sa;
		if (len) *len = sizeof(sockaddr_in);
		memset(sin, 0, sizeof(sockaddr_in));
		sin->sin_family = family;
		sin->sin_port = htons(_port);
		sin->sin_addr.s_addr = _sin4;
	} else {
		sockaddr_in6 *sin6 = (sockaddr_in6*)&sa;
		memset(sin6, 0, sizeof(sockaddr_in6));
		if (len) *len = sizeof(sockaddr_in6);
		sin6->sin6_family = family;
		sin6->sin6_addr = _in._in6addr;
		sin6->sin6_port = htons(_port);
	}
	return sa;
}

// #define addrfmt(x, s) x.fmt(s, sizeof(s))
cstr PackedSockAddr::fmt(str s, size_t len) const
{
	memset(s, 0, len);
	const byte family = get_family();
	str i;
	if (family == AF_INET) {
		INET_NTOP(family, (uint32*)&_sin4, s, len);
		i = s;
		while (*++i) {}
	} else {
		i = s;
		*i++ = '[';
		INET_NTOP(family, (in6_addr*)&_in._in6addr, i, len-1);
		while (*++i) {}
		*i++ = ']';
	}
	snprintf(i, len - (i-s), ":%u", _port);
	return s;
}
