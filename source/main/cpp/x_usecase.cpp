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

		static void gEndPointToStr(netip4 ip, char* outString, u32 inStringMaxLen)
		{
		}


		class MyAllocator : public iallocator
		{
		public:
								MyAllocator(x_iallocator* inSystemAllocator) : mSystemAllocator(inSystemAllocator) {}

			virtual void*		alloc(u32 inSize, u32 inAlignment)
			{
				return mOurAllocator->allocate(inSize, inAlignment);
			}
			virtual void		dealloc(void* inOldMem)
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
			
			xp2p::node system(ourSystemAllocator);
			xp2p::node* node = &system;
			ipeer* host = node->start(netip4().port(51888));

			// Let's connect to the tracker
			ipeer* remote_peer = node->register_peer(peerid(0), netip4(10, 0, 8, 12).port(51888));
			node->connect_to(remote_peer);

			incoming_messages rcvd_messages;
			while (remote_peer != NULL)
			{
				if (node->event_loop(rcvd_messages, 1000))	// Wait a maximum of 1000 ms
				{
					while (!rcvd_messages.is_empty())
					{
						incoming_message rmsg = rcvd_messages.dequeue();

						if (rmsg.header().is_from(remote_peer->get_id()))
						{
							if (rmsg.header().is_event())
							{
								if (rmsg.header().is_connected())
								{
									outgoing_message tmsg;
									node->create_message(tmsg, remote_peer, 40);
									tmsg.write("Hello remote peer, how are you?");
								}
								else if (rmsg.header().is_not_connected())
								{
									// Remote peer has disconnected or cannot connect
									break;
								}
							}
							else if (rmsg.header().is_data())
							{
								char ip4_str[32];
								remote_peer->get_ip4().to_string(ip4_str, sizeof(ip4_str));

								char msgString[256];
								u32 msgStringLen;
								rmsg.read_string(msgString, sizeof(msgString), msgStringLen);
								x_printf("info: message \"%s\"received from Peer \"%s\"", x_va_list(x_va((const char*)msgString), x_va(ip4_str)));
							}
						}
						else
						{
							//break;
						}
					}
				}
			}

			// Clear all pointers
			remote_peer = NULL;

			// Stop all threads, close all sockets, release all resources
			node->stop();

			// Release our allocators and their memory back to the system allocator
			MyAllocator::sRelease(ourSystemAllocator);
		}
	}
}