//==============================================================================
//  x_p2p.h
//==============================================================================
#ifndef __XPEER_2_PEER_H__
#define __XPEER_2_PEER_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_msg.h"

namespace xcore
{
	// ==============================================================================================================================
	// ==============================================================================================================================
	// ==============================================================================================================================
	namespace xp2p
	{
		typedef		u32			NetPoint;
		typedef		u16			NetPort;
		typedef		u64			PeerID;
		typedef		u32			ChannelID;

		struct NetAddress
		{
			NetPoint	mPoint;
			NetPort		mPort;
		};

		// IPv4 = "10.0.0.1:5066"
		extern NetAddress const*gCreateAddress(const char* address_str);
		
		// "Peer ID - Address" Dictionary
		extern PeerID			gRegisterHost(NetPort port);
		extern void				gRegisterPeer(PeerID, Address const*);
		extern NetAddress const*gFindAddressOfPeer(PeerID);
		extern PeerID			gFindPeerByAddress(NetAddress const*);
		extern void				gUnregisterPeer(PeerID);

		struct Message
		{
			xbyte const*		mMsgData;
			u32					mMsgDataMaxSize; 

			enum EType { EVENT, MESSAGE };
			u16					mType;
			enum EEvent { NONE, CONNECTED, CONNECTION_FAILED, DISCONNECTED };
			u16					mEvent;

			// Actual message in memory starts here
			// This is the header that will be filled
			// by our implementation.
			u32					mMsgDataSize;	// Includes this header
			ChannelID			mChannelId;		// Channel ID
			PeerID				mPeerId;		// From ID
		};

		// P2P - Message Channel
		class IChannel
		{
		public:
			virtual const char*		Name() const = 0;

			// Send
			virtual Message const*	CreateMsg(PeerID to, u32 size) = 0;
			virtual void			WriteMsg(xbyte* data, u32 size) = 0;
			virtual void			QueueMsg() = 0;

			// Receive
			virtual void			ReadMsg(Message const*& msg) = 0;
			virtual void			CloseMsg(Message const* msg) = 0;

		protected:
			virtual					~IChannel() {}
		};

		// 
		// We can create different kind of channels:
		// 
		// 1. A channel where the IN and OUT are just allocating directly from the system
		//    using malloc() and closing a message will directly call free().
		//
		// 2. A channel where the IN and OUT use a specialized allocator that uses
		//    a pre-allocated chunk of memory.
		//
		// 3. A channel where the IN and OUT hold a lockless queue with a finite amount 
		//    of pointers to fixed size chunks (512 bytes or so). Here a message can never
		//    use more than 512 bytes, but even a 8 byte message will occupy 512 bytes in
		//    memory.
		//
		// Blocking IO:
		// The IN channel could block the calling thread when there are no messages.
		// The OUT channel could block when the channel has no memory left for inserting
		// another message, the calling thread has to wait until messages are being closed
		// by the network system sending and closing the messages.
		// 
		enum EChannelMode
		{
			CM_UNRELIABLE,
			CM_UNRELIABLE_SEQUENCED,								///< Late packets are dropped
			CM_RELIABLE,											///< Reliable unordered, lost packets will be resent
			CM_RELIABLE_INORDER,									///< Reliable ordered, lost packets will be resent and early packets are withheld
			CM_DEFAULT          = CM_RELIABLE_INORDER
		};

		extern IChannel*		gCreateChannel(const char* channel_name);
		extern IChannel*		gCreateChannel(const char* channel_name, EChannelMode mode);
		extern void				gDestroyChannel(IChannel*);

		// P2P - Peer
		// This is the local Peer. Using the Peer you can connect to and disconnect from
		// remote Peers. Using the created IChannel you can send and receive messages.
		//
		class Peer
		{
		public:
			virtual void		AddChannel(IChannel* channel) = 0;
			
			virtual void		Start(PeerID tracker) = 0;
			virtual void		Stop() = 0;

			virtual PeerID		GetId() const = 0;

			virtual void		ConnectTo(PeerID peerId) = 0;
			virtual void		DisconnectFrom(PeerID peerId) = 0;

		protected:
			virtual				~Peer() {}
		};

		/**
		\brief Example on how to create a Peer

		\code
		int main(void)
		{
			PeerID const hostID = gRegisterHost(5599);

			xp2p::Address const* tracker_address = gCreateAddress("10.0.6.2:5599");
			PeerID const trackerID = 0;
			gRegisterPeer(trackerID, tracker_address);

			// Other symphony peers will also register their channel under this name.
			// This ensures that all messages send over this channel will only endup
			// in this channel and not in any other channels that might be added.
			// Actually the ID of the channel will be a 32 bit string hash of the name
			// of the channel.
			// The channel created here by default is RELIABLE INORDER, if you need to
			// send messages in an unreliable manner than you can pass CM_UNRELIABLE.
			xp2p::Peer* peer = gCreateHost(hostID);
			xp2p::IChannel* symphony_channel = gCreateChannel("Symphony");
			peer->AddChannel(symphony_channel);
			peer->Start(trackerID);
			// Peer is now running in the background
			// We wait until the Peer has shutdown
			peer->WaitUntilExit();
			return 0;
		}

		// Following below is a small example of how to connect to another peer, wait
		// for the connected message and return a message.
		// This only works because we assume that the only connections that exist is
		// the one that we will establish with the remote peer. If there where any
		// other incoming connections that are currently connecting than that would
		// mean that the channel would already contain message to be received and the
		// first message would not be the one from our remote peer.
		// This assumes that you already have received information of the other peer
		// and have registered it with 
		//     xp2p::Address const* other_symphony_peer_address = gCreateAddress("10.0.6.3:5599");
		//     gRegisterPeer(other_symphony_peerID, other_symphony_peer_address).
		
		PeerID other_symphony_peerID = ...;
		peer->ConnectTo(other_symphony_peerID);

		xp2p::Message* received_msg;
		symphony_channel->ReadMsg(received_msg);		// This will block until a message has been received

		// Do something with the message
		... 
		if (received_msg->mPeerID == other_symphony_peerID)
		{
			if (received_msg->mType==xp2p::Message::EVENT && received_msg->mEvent==xp2p::Message::CONNECTED)
			{
				// Return a simple message containing a value of 100
				xp2p::Message const* msg_to_send = symphony_channel->CreateMsg(other_symphony_peerID, 4);
				u32 value = 100;
				symphony_channel->WriteMsg(&value, sizeof(value));
				symphony_channel->QueueMsg();
			}
		}

		symphony_channel->CloseMsg(received_msg);

		\endcode
		**/

		extern Peer*			gCreateHost(PeerID id);
		extern void				gDestroyHost(Peer*);

	}
}

#endif	///< __XPEER_2_PEER_H__
