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

namespace xcore
{
	// ==============================================================================================================================
	// ==============================================================================================================================
	// ==============================================================================================================================
	namespace xp2p
	{
		class IPeer;
		class IChannel;

		enum EMsgInfo
		{
			MSG_NONE						= 0, 
			MSG_DATA_RECEIVED				= 1,
			MSG_EVENT_CONNECTED				= 100, 
			MSG_EVENT_CONNECTION_FAILED		= -100, 
			MSG_EVENT_DISCONNECTED			= -200 
		};

	
		class IPeer
		{
		public:
			enum EConnection { 


		private:

		};

		// P2P - Host
		// This is the local Peer called Host. Using the Host you can connect to and 
		// disconnect from remote Peers. Using the created IChannel you can send and 
		// receive messages.
		// Note: You will work with remote peers using IPeer*.
		class Host
		{
		public:
			virtual void		AddChannel(IChannel* channel) = 0;
			
			virtual void		Start() = 0;
			virtual void		Stop() = 0;

			virtual void		ConnectTo(IPeer* peer) = 0;
			virtual u32			NumConnections() const = 0;
			virtual void		GetConnections(IPeer** outPeerList, u32 sizePeerList, u32& outPeerCnt) = 0;
			virtual void		DisconnectFrom(IPeer* peer) = 0;

		protected:
			virtual				~Host() {}
		};


		/**
		\brief Example on how to create a Peer

		\code
		int main(void)
		{
			PeerID const hostID = gRegisterHost(5599);

			xp2p::Address const* tracker_address = gCreateAddress("10.0.6.2:5599");
			PeerID const trackerID = 0;	// ID 0 is reserved by the Tracker 
			gRegisterPeer(trackerID, tracker_address);

			// Other symphony peers will also register their channel under this name.
			// This ensures that all messages send over this channel will only endup
			// in this channel and not in any other channels that might be added.
			xp2p::Peer* peer = gCreateHost(hostID);
			xp2p::IChannel* symphony_channel = gCreateChannel("Symphony");
			peer->AddChannel(symphony_channel);
			peer->ConnectTo(trackerID);
			// Peer is now running in the background
			// Tell the Peer to stop
			peer->Stop();
			// Destroy the Peer nicely and do a correct clean-up
			gDestroyHost(peer);
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
		//     xp2p::Address const* other_symphony_peer_address = gCreateAddress("10.0.6.3:5599");
		//     gRegisterPeer(other_symphony_peerID, other_symphony_peer_address).
		// 

		PeerID other_symphony_peerID = ...;
		peer->ConnectTo(other_symphony_peerID);

		xp2p::ReadHandle received_msg;
		received_msg = symphony_channel->ReadMsg();		// This will block until a message has been received

		// Do something with the message
		... 
		if (received_msg->mPeerID == other_symphony_peerID)
		{
			if (received_msg->mType==xp2p::Message::EVENT && received_msg->mEvent==xp2p::Message::CONNECTED)
			{
				// Send a simple message containing a piece of text
				// Reserve enough space when creating the message
				xp2p::Message const* msg_to_send = symphony_channel->CreateMsg(other_symphony_peerID, 16);
				const char* msg_text = "Hello peer!";
				// Write the text into the message data
				symphony_channel->WriteMsg(msg_text, x_strlen(msg_text));
				// Queue up the message for sending
				symphony_channel->QueueMsg();
			}
		}

		symphony_channel->CloseMsg(received_msg);

		\endcode
		**/

		class P2P
		{
		public:
								P2P();

			void				Start(NetPort host_port, x_iallocator* mem_allocator);
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
			MsgHandle			AllocMsg(const char* channel_name, IPeer* to, u32 size);
			void				SendMsg(MsgHandle msg);

			// Receive
			MsgHandle			ReceiveMsg(const char* channel_name);
			void				FreeMsg(MsgHandle msg);

		protected:
			class Implementation;
			Implementation*		mImplementation;
		};

	}
}

#endif	///< __XPEER_2_PEER_H__
