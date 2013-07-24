#include "xbase\x_target.h"
#include "xbase\x_allocator.h"
#include "xp2p\x_p2p.h"
#include "xp2p\x_msg.h"
#include "xp2p\private\x_channel.h"

namespace xcore
{
	namespace xp2p
	{
		class System::Implementation
		{
		public:
		};


		System::System() : mImplementation(NULL)
		{
		}

		IPeer*				System::Start(NetPort inHostPort, IAllocator* inSystemAllocator)
		{
			return 0;
		}

		void				System::Stop()
		{
		}

		// Peers
		IPeer*				System::ConnectTo(const char* inEndpoint)
		{
			return 0;
		}

		void				System::DisconnectFrom(IPeer*)
		{
		}

		// Channels
		IChannel*			System::RegisterChannel(const char* inChannelName, IAllocator* inMsgAllocator, IDelegate* inChannelReceiveListener)
		{
			return 0;
		}

		// Messages
		IMessage*			System::CreateMessage(IChannel* inChannel, IPeer* inTo, u32 inMaxMsgSizeInBytes)
		{
			return 0;
		}

		void				System::SendMessage(IMessage* inMsg)
		{
		}

		IMessage*			System::ReceiveMessage(IChannel* inChannel)
		{
			return 0;
		}

		void				System::DestroyMessage(IMessage* inMsg)
		{
			
		}

	}
}