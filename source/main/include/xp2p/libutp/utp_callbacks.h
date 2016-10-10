#ifndef __UTP_CALLBACKS_H__
#define __UTP_CALLBACKS_H__

#include "xp2p/libutp/utp.h"
#include "xp2p/libutp/utp_internal.h"

class utp_callbacks
{
public:
	virtual int on_firewall(utp_context *ctx, const struct sockaddr *address, socklen_t address_len) = 0;
	virtual void on_accept(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len) = 0;
	virtual void on_connect(utp_context *ctx, utp_socket *s) = 0;
	virtual void on_error(utp_context *ctx, utp_socket *s, int error_code) = 0;
	virtual void on_read(utp_context *ctx, utp_socket *s, const byte *buf, size_t len) = 0;
	virtual void on_overhead_statistics(utp_context *ctx, utp_socket *s, int send, size_t len, int type) = 0;
	virtual void on_delay_sample(utp_context *ctx, utp_socket *s, int sample_ms) = 0;
	virtual void on_state_change(utp_context *ctx, utp_socket *s, int state) = 0;
	virtual uint16 get_udp_mtu(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len) = 0;
	virtual uint16 get_udp_overhead(utp_context *ctx, utp_socket *s, const struct sockaddr *address, socklen_t address_len) = 0;
	virtual uint64 get_milliseconds(utp_context *ctx, utp_socket *s) = 0;
	virtual uint64 get_microseconds(utp_context *ctx, utp_socket *s) = 0;
	virtual uint32 get_random(utp_context *ctx, utp_socket *s) = 0;
	virtual size_t get_read_buffer_size(utp_context *ctx, utp_socket *s) = 0;
	virtual void log(utp_context *ctx, utp_socket *s, const byte *buf) = 0;
	virtual void sendto(utp_context *ctx, utp_socket *s, const byte *buf, size_t len, const struct sockaddr *address, socklen_t address_len, uint32 flags) = 0;
};

#endif // __UTP_CALLBACKS_H__
