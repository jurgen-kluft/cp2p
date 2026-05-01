#ifndef __CPEER_2_PEER_PEER_H__
#define __CPEER_2_PEER_PEER_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "cp2p/c_types.h"

namespace ncore
{
    namespace np2p
    {
        // P2P - Peer
        // This represents a peer in the network.
        class peer
        {
        public:
            enum estatus
            {
                INACTIVE      = 0,
                CONNECT       = 1,
                CONNECTING    = 2,
                CONNECTED     = 3,
                DISCONNECT    = 11,
                DISCONNECTING = 12,
                DISCONNECTED  = 13
            };

            virtual bool    is_remote() const  = 0;
            virtual estatus get_status() const = 0;
            virtual netip   get_ip() const     = 0;

        protected:
            virtual ~peer() {}
        };

    }  // namespace np2p
}  // namespace ncore

#endif  // __CPEER_2_PEER_PEER_H__
