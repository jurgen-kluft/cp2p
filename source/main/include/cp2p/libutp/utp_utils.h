#ifndef __UTP_UTILS_H__
#define __UTP_UTILS_H__

#include "xp2p/libutp/utp_types.h"

typedef struct UTPSocket					utp_socket;
typedef struct struct_utp_context			utp_context;

class utp_system
{
public:
	virtual uint16 get_udp_mtu(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len) = 0;
	virtual uint16 get_udp_overhead(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len) = 0;
	virtual uint64 get_milliseconds(utp_context *ctx, utp_socket *s) = 0;
	virtual uint64 get_microseconds(utp_context *ctx, utp_socket *s) = 0;
	virtual uint32 get_random(utp_context *ctx, utp_socket *s) = 0;
	virtual size_t get_read_buffer_size(utp_context *ctx, utp_socket *s) = 0;

	static uint64 default_get_udp_mtu(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len);
	static uint64 default_get_udp_overhead(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len);
	static uint64 default_get_random(utp_context *ctx, utp_socket *s);
	static uint64 default_get_milliseconds(utp_context *ctx, utp_socket *s);
	static uint64 default_get_microseconds(utp_context *ctx, utp_socket *s);
};


#endif