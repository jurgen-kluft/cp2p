#include "xbase\x_target.h"
#include "xbase\x_allocator.h"
#include "xbase\x_va_list.h"
#include "xbase\x_string_ascii.h"

#include "xp2p\x_p2p.h"
#include "xp2p\x_peer.h"
#include "xp2p\x_msg.h"
#include "xp2p\private\x_allocator.h"

namespace xcore
{
	namespace xp2p
	{
		x_iallocator* gCreateHeapAllocator(void* mem, u32 memSize)
		{
			return NULL;
		}

		static void gEndPointToStr(NetIP4 ip, char* outString, u32 inStringMaxLen)
		{
		}


		class MyAllocator : public IAllocator
		{
		public:
								MyAllocator(x_iallocator* inSystemAllocator) : mSystemAllocator(inSystemAllocator) {}

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
				allocator->mSystemAllocator->deallocate(allocator->mOurAllocator);
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
			
			xp2p::P2P system(ourSystemAllocator);
			xp2p::P2P* node = &system;
			IPeer* host = node->Start(NetIP4().Port(5008));

			// Let's connect to a peer
			IPeer* remotePeer = node->RegisterPeer(PeerID(1234), NetIP4(10, 0, 0, 24).Port(5008));
			node->ConnectTo(remotePeer);

			while (remotePeer != NULL)
			{
				IncomingMessage rmsg;
				if (node->ReceiveMsg(rmsg, 1000))	// Wait 1000 ms
				{
					if (rmsg.IsFrom(remotePeer))
					{
						if (rmsg.Type().IsEvent())
						{
							if (rmsg.Event().IsConnected())
							{
								OutgoingMessage tmsg;
								node->CreateMsg(tmsg, remotePeer, 40);
								tmsg.Write("Hello remote peer, how are you?");
								node->SendMsg(tmsg);
							}
							else if (rmsg.Event().IsDisconnected())
							{
								// Remote peer has disconnected
								remotePeer = NULL;
								break;
							}
						}
						else if (rmsg.Type().HasData())
						{
							char ip4_str[32];
							remotePeer->GetIP4().ToString(ip4_str, sizeof(ip4_str));

							char msgString[256];
							u32 msgStringLen;
							rmsg.ReadStr(msgString, sizeof(msgString), msgStringLen);
							x_printf("Message \"%s\"received from Peer \"%s\"", x_va_list(x_va((const char*)msgString), x_va(ip4_str)));
						}
					}
					else
					{
						//break;
					}
				}
			}

			// Clear all pointers
			remotePeer = NULL;

			// Stop all threads, close all sockets, release all resources
			node->Stop();

			// Release our allocators and their memory back to the system allocator
			MyAllocator::sRelease(ourSystemAllocator);
		}
	}
}