#include "xbase\x_target.h"
#include "xbase\x_allocator.h"
#include "xbase\x_string_std.h"

#include "xp2p\x_p2p.h"
#include "xp2p\x_peer.h"
#include "xp2p\x_msg.h"
#include "xp2p\private\x_allocator.h"
#include "xp2p\private\x_channel.h"

namespace xcore
{
	namespace xp2p
	{
		x_iallocator* gCreateHeapAllocator(void* mem, u32 memSize)
		{
			return NULL;
		}

		static void gEndPointToStr(NetIP4 ip, NetPort port, char* outString, u32 inStringMaxLen)
		{
		}

		class PublicChannelListener : public IDelegate
		{
		public:
			virtual void	Handle()
			{
				// Signal the wait object
			}

			void			WaitForMsgReceived()
			{
				// Wait for the object to be signaled
			}
		};

		class MyAllocator : public IAllocator
		{
		public:
								MyAllocator(x_iallocator* inSystemAllocator) : mSystemAllocator(inAllocator) {}

			virtual void*		Alloc(u32 inSize, u32 inAlignment)
			{
				return mOurAllocator->allocate(inSize, inAlignment);
			}
			virtual void*		Realloc(void* inOldMem, u32 inNewSize, u32 inNewAlignment)
			{
				return mOurAllocator->reallocate(inOldMem, inNewSize, inNewAlignment);
			}
			virtual void		Dealloc(void* inOldMem)
			{
				mOurAllocator->deallocate(inOldMem);
			}

			static MyAllocator*	sCreate(x_iallocator* inSystemAllocator, u32 memsize)
			{
				// Create a heap allocator or any other type of allocator
				return NULL;
			}

			static void			sRelease(MyAllocator*& allocator)
			{
				// Should we call the destructor of our allocator?
				mSystemAllocator->deallocate(mOurAllocator);
				allocator = NULL;
			}

		private:
							~MyAllocator() { }

			x_iallocator*	mSystemAllocator;
			x_iallocator*	mOurAllocator;
		};

		static void ExampleUseCase(x_iallocator* inSystemAllocator)
		{
			MyAllocator* ourSystemAllocator = MyAllocator::sCreate(inSystemAllocator, 2 * 1024 * 1024);
			xp2p::System system;
			xp2p::System* node = &system;
			IPeer* host = node->Start(5008, ourSystemAllocator);

			MyAllocator* publicChannelAllocator = MyAllocator::sCreate(inSystemAllocator, 1 * 1024 * 1024);

			// Register the channel
			// We have implemented a listener which provides functionality to wait (blocking thread) for
			// incoming messages.
			PublicChannelListener publicChannelListener;
			IChannel* publicChannel = node->RegisterChannel("public", publicChannelAllocator, &publicChannelListener);

			// Let's connect to a peer
			IPeer* remotePeer = node->ConnectTo("10.0.0.24:5008");
			while (remotePeer != NULL)
			{
				publicChannelListener.WaitForMsgReceived();

				ReceiveMessage rmsg(node, publicChannel);
				if (rmsg.FromPeer() == remotePeer)
				{
					if (rmsg.IsEvent()) 
					{
						if (rmsg.GetEvent()==ReceiveMessage::EVENT_CONNECTED)
						{
							SendMessage tmsg(node, publicChannel, remotePeer);
							tmsg.WriteStr("Hello remote peer, how are you?");
						}
						else if (rmsg.GetEvent()==ReceiveMessage::EVENT_DISCONNECTED)
						{
							// Remote peer has disconnected
							remotePeer = NULL;
							break;
						}
					}
					else if (rmsg.HasData())
					{
						char msgString[256];
						rmsg.ReadStr(msgString, sizeof(msgString));
						x_printf("Message \"%s\"received from Peer \"%s\"", x_va_list(x_va(msgString), x_va(remotePeer->GetStr())));
					}
				}
				else
				{
					break;
				}
			}

			// Clear all pointers
			publicChannel = NULL;
			remotePeer = NULL;

			// Stop all threads, close all sockets, release all resources
			node->Stop();

			// Release our allocators and their memory back to the system allocator
			MyAllocator::sRelease(publicChannelAllocator);
			MyAllocator::sRelease(ourSystemAllocator);
		}
	}
}