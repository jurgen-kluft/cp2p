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
		enum EMsgInfo
		{
			MSG_NONE						= 0, 
			MSG_DATA_RECEIVED				= 1,
			MSG_EVENT_CONNECTED				= 100, 
			MSG_EVENT_CONNECTION_FAILED		= -100, 
			MSG_EVENT_DISCONNECTED			= -200 
		};

		typedef void*				WriteHandle;
		typedef void*				ReadHandle;

		// P2P - Message Channel (single threaded access)
		class IChannel
		{
		public:
			virtual const char*		Name() const = 0;

			// Send
			virtual WriteHandle		CreateMsg(PeerID to, u32 size) = 0;
			virtual void			QueueMsg(WriteHandle hnd) = 0;

			// Receive
			virtual ReadHandle		ReadMsg() = 0;
			virtual void			CloseMsg(ReadHandle hnd) = 0;

		protected:
			virtual					~IChannel() {}
		};

		// 
		// We can create different kind of channels:
		// 
		// 1. A channel where it is allocating directly from the system using malloc() 
		//    and closing a message will directly call free().
		//
		// 2. A channel where it uses a specialized allocator that uses a pre-allocated 
		//    chunk of memory.
		//
		// 3. A channel where it holds a lockless queue with a finite amount of pointers 
		//    to fixed size chunks (512 bytes or so). Here a message can never use more 
		//    than 512 bytes, but even a 8 byte message will occupy 512 bytes in memory.
		//
		// Blocking IO:
		//    The channel could block the calling thread when there are no messages.
		//    The channel could block when the channel has no memory left for inserting
		//    another message, the calling thread has to wait until messages are being closed
		//    by the network system sending and closing the messages.
		// 

		// API for Channel
		class Channels
		{
		public:
			virtual IChannel*	Create(const char* channel_name) = 0;
			virtual void		Destroy(IChannel*) = 0;

		private:
			virtual 			~Channels() {}
		};

		// P2P - Peer
		// This is the local Peer. Using the Peer you can connect to and disconnect from
		// remote Peers. Using the created IChannel you can send and receive messages.
		// Note: You will work with remote peers through their PeerID.
		class Peer
		{
		public:
			virtual void		AddChannel(IChannel* channel) = 0;
			
			virtual void		Start() = 0;
			virtual void		Stop() = 0;

			virtual PeerID		GetId() const = 0;

			virtual void		ConnectTo(PeerID peerId) = 0;
			virtual u32			NumConnections() const = 0;
			virtual void		GetConnections(PeerID* outPeerList, u32 sizePeerList, u32& outPeerCnt) = 0;
			virtual void		DisconnectFrom(PeerID peerId) = 0;

		protected:
			virtual				~Peer() {}
		};

		// API for Host
		class Host
		{
		public:
			virtual Peer*		Create() = 0;
			virtual void		Destroy(Peer*) = 0;

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

			NetAddress			RegisterAddress(const char* address_str);
			void				RegisterPeer(PeerID, NetAddress);
			void				UnregisterPeer(PeerID);

			NetAddress			FindAddressOfPeer(PeerID) const;
			PeerID				FindPeerByAddress(NetAddress) const;

			// Host
			PeerID				GetId() const;

			void				ConnectTo(PeerID peerId);
			void				DisconnectFrom(PeerID peerId);
			u32					NumConnections() const;
			void				GetConnections(PeerID* outPeerList, u32 sizePeerList, u32& outPeerCnt);

			// Channels
			IChannel*			Create(const char* channel_name);
			void				Destroy(IChannel*);

		protected:
			class Implementation;
			Implementation*		mImplementation;
		};

	}
}

#endif	///< __XPEER_2_PEER_H__
