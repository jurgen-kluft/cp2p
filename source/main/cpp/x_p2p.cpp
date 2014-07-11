#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "xp2p\x_p2p.h"

namespace xcore
{
	namespace xp2p
	{
		class P2P::Implementation
		{
		public:
		};


		P2P::P2P(IAllocator* inSystemAllocator) : mAllocator(inSystemAllocator), mImplementation(NULL)
		{
		}

		IPeer*				P2P::Start(NetIP4 inHostPort)
		{
			return 0;
		}

		void				P2P::Stop()
		{
		}

		// Peers
		void				P2P::ConnectTo(IPeer* _peer)
		{
			
		}

		void				P2P::DisconnectFrom(IPeer*)
		{
		}

		// Messages
		bool				P2P::CreateMsg(OutgoingMessage& _msg, IPeer* _to, u32 _size)
		{
			return 0;
		}

		void				P2P::SendMsg(OutgoingMessage& _msg)
		{
		}

		bool				P2P::ReceiveMsg(IncomingMessage& _msg, u32 _wait_in_ms)
		{
			return false;
		}
	}
}