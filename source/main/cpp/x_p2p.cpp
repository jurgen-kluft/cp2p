#include "xbase\x_target.h"
#include "xp2p\x_p2p.h"

namespace xcore
{
	namespace xp2p
	{
		/** ======= P2P Backend implementations =======

		Using this p2p library to establish a p2p overlay
		likely will not result a lot of incoming and
		outgoing TCP connections. On average there will
		be around 15 TCP connections.

		We have a couple of choices
		1. A simple single-threaded TCP client/server implementation
		2. A worker-thread oriented IOCP TCP client/server implementation
		3. A single threaded RUDP model 
		4. A worker-thread oriented RUDP implementation (prefered)

		======= UDT based network back-end =======
	
		Create listening socket
		Create 'listen' thread and run the listening and accept logic 
		Create 'IO' thread and use the epoll API of UDT to detect 
		readable/writeable sockets, we also need to wake-up when a new
		connection has been created. We also need to wake-up when the
		user has scheduled packets to be send.
		For this we can connect to the listening socket and create a
		socket that we use to trigger the 'IO' thread whenever we have
		packets to send. Actually we can 


		======= xlang with UDT based network back-end =======

		Doing this requires that Symphony also to become a xlang::Actor and
		every Slurpie Session as well.

		One actor for the listening-accept logic, whenever a new connection has
		been received we send a message to the IO actor that handles the send
		and receive logic.
		The accept() call will block the Actor, so we have to be able to put
		this Actor on its own dedicated thread.

		The thing is that an epoll() call will block the Actor which is
		not what we want since that might block a worker-thread.
		We can put the real IO on a seperate non-Actor thread, we can wake up
		this thread with our virtual remote socket.
		We can send messages from this thread to Actors and we can send messages
		to the IO thread using a ..
		
		Sounds to me we are duplicating an Actor here, maybe we should look at
		how to extend xlang with the functionality of locking an Actor to its
		own thread, bool Actor::RequiresDedicatedThread() const;
		And then to see how to wake-up the Actor when it is blocked!, with
		sockets it is easy if we can overload a function of Actor implement
		the wake-up logic.

		The IO thread will run and detect new messages to be send and received.
		We do need to be able to build a list of sockets that we would like to
		query if they are ready for sending packets, how do we do this?


		
		======= Thoughts =======

		For UDP we require a couple of protocols for sending messages:
		1. Unreliable
		2. Unreliable Sequenced (late packets are dropped)
		3. Reliable (unordered) (lost packets are resent)
		4. Reliable Ordered (lost packets are resent and early packets are withheld)
		5. Reliable Sequenced

		We could do the above by opening 4 UDP sockets, one for every method. Doing
		like that means that we lock a single protocol to one socket for receiving
		and sending.

		Also for file-transfer (slurpie) we could also open a dedicated UDP
		socket for one transfer instead of multiplexing and demultiplexing the
		data ourselves. We could activate the RBUDP protocol (Reliable Blast UDP)
		on this.

		Is it possible to even use seperate UDP sockets for sending, apart from those
		that we use for receiving?

		**/
		struct IPv4 { u8 mIPv4[4]; };
		struct IPv6 { u8 mIPv4[16]; };

		namespace detail
		{
			struct Address : public Address
			{
			public:
									Address(IPv4 const& ip, u16 port);
									Address(IPv6 const& ip, u16 port);

				u32					WriteTo(xbyte*) const;
				u32					ReadFrom(xbyte const*);
			};
		}

		NetAddress const*			gFindAddress(PeerID);
		void						gUpdatePeer(PeerID, NetAddress const*);




	}
}