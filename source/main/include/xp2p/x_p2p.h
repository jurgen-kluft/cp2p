//==============================================================================
//  x_p2p.h
//==============================================================================
#ifndef __XPEER_2_PEER_H__
#define __XPEER_2_PEER_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"
#include "xp2p\x_msg.h"
#include "xp2p\x_peer.h"

namespace xcore
{
	// ==============================================================================================================================
	// ==============================================================================================================================
	// ==============================================================================================================================
	namespace xp2p
	{
		class IPeer
		{
		public:

		};


		/**
		\brief Example on how to create a Peer

		\code
		class MyMemoryAllocator : public MemoryAllocator
		{
			...
		};

		int main(void)
		{
			// In a real Application you need to allocate this and not like we do here 
			// putting it on the stack.
			MyMemoryAllocator allocator;

			P2P* p2p = new P2P();
			p2p->Start(5195, &allocator);

			IPeer* tracker = p2p->RegisterPeer("10.0.6.2:5599");
			p2p->ConnectTo(tracker);

			// Other symphony peers will also register their channel under this name.
			// This ensures that all messages send over this channel will only end up
			// in this channel and not in any other channels that might be added.
			const char* symphony_channel = "Symphony";

			// Peer is now running in the background, getting connected to the tracker.

			// Tell the Peer to stop. This will release all resources, it is also
			// blocking the thread we are on.
			p2p->Stop();

			// Destroy the P2P network 
			delete peer;

			return 0;
		}

		// 
		// Following below is a small example of how to connect to another peer, wait
		// for the connected event and return a message.
		// This only works because we assume that the only connection that exists is
		// the one that we will establish with the remote peer. If there where any
		// other incoming connections that are currently connecting than that would
		// mean that the channel could already contain messages to be received and the
		// first message would very likely not be the one from our remote peer.
		// This assumes that you already have received information of the other peer
		// and have registered it with:
		//     PeerID other_symphony_peer_id = ...;
		//     IPeer* other_symphony_peer = p2p->RegisterPeer(other_symphony_peer_id, "10.0.6.3:5599");
		// 

		p2p->ConnectTo(other_symphony_peer);

		const char* symphony_channel = "Symphony";

		xp2p::RxMessage* received_msg;
		received_msg = p2p->ReadMsg(symphony_channel);		// This will block until a message has been received

		// Do something with the message
		... 
		if (received_msg->FromPeerID() == other_symphony_peer->GetID())
		{
			if (received_msg->IsType(xp2p::Message::EVENT) && received_msg->IsEvent(xp2p::Message::CONNECTED))
			{
				// Send a simple message containing a piece of text
				const char* msg_text = "Hello peer!";
				// Make sure to reserve enough space when creating the message
				s32 msg_len = x_strlen(msg_text) + 1;
				xp2p::TxMessage* msg_to_send = p2p->WriteMsg(symphony_channel, other_symphony_peer, msg_len);
				
				// Write the text into the message data
				msg_to_send->Write(msg_text);

				// Queue up the message for sending
				p2p->SendMsg(msg_to_send);
			}
		}

		p2p->CloseMsg(received_msg);

		\endcode
		**/

		class MemoryAllocator
		{
		public:
			virtual void*		Allocate(u32 numberOfBytes, u32 alignment) = 0;
			virtual void		Deallocate(void*) = 0;
		};

		class P2P
		{
		public:
								P2P();

			void				Start(NetPort host_port, MemoryAllocator* memory_allocator);
			void				Stop();

			IPeer*				RegisterPeer(const char* p2p_endpoint_str);
			void				UnregisterPeer(IPeer*);

			// Host
			IPeer*				GetHost() const;

			void				ConnectTo(IPeer* peer);
			void				DisconnectFrom(IPeer* peer);
			u32					NumConnections() const;
			void				GetConnections(IPeer** outPeerList, u32 sizePeerList, u32& outPeerCnt);

			// Channels
			// Send
			TxMessage*			WriteMsg(const char* channel_name, IPeer* to, u32 size);
			void				SendMsg(TxMessage*);

			// Receive
			RxMessage*			ReadMsg(const char* channel_name);
			void				CloseMsg(RxMessage*);

		protected:
			class Implementation;
			Implementation*		mImplementation;
		};

	}
}

#endif	///< __XPEER_2_PEER_H__
