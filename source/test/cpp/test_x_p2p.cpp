#include "xp2p\x_p2p.h"
#include "xp2p\x_peer.h"
#include "xp2p\x_msg.h"
#include "xp2p\private\x_allocator.h"

#include "xbase\x_target.h"
#include "xbase\x_allocator.h"
#include "xbase\x_va_list.h"
#include "xbase\x_string_ascii.h"
#include "xbase\x_bit_field.h"

#include "xunittest\xunittest.h"

using namespace xcore;
extern x_iallocator* gTestAllocator;




namespace xcore
{
	namespace xp2p
	{
		enum emessage_flags
		{
			MSG_FLAG_ANNOUNCE = 1,
			MSG_FLAG_SYMPHONY = 2,
			MSG_FLAG_CHUNK = 4,
		};


		class MyAllocator : public iallocator
		{
		public:
			MyAllocator(x_iallocator* inSystemAllocator) : mSystemAllocator(inSystemAllocator) {}

			virtual void*		allocate(u32 inSize, u32 inAlignment)
			{
				return mOurAllocator->allocate(inSize, inAlignment);
			}
			virtual void		deallocate(void* inOldMem)
			{
				mOurAllocator->deallocate(inOldMem);
			}

			static MyAllocator*	sCreate(x_iallocator* inSystemAllocator)
			{
				// Create a heap, pool, caching or any other type of allocator
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


		class MyMessageAllocator : public imessage_allocator
		{
		public:
			MyMessageAllocator(x_iallocator* inSystemAllocator) : mSystemAllocator(inSystemAllocator) {}


			virtual message*	allocate(ipeer* _from, ipeer* _to, u32 _flags)
			{
				void* mem = mOurAllocator->allocate(sizeof(message), sizeof(void*));
				message* msg = new (mem)message(_from, _to, _flags);
				return msg;
			}

			virtual void			deallocate(message* _msg)
			{
				mOurAllocator->deallocate(_msg);
			}

			virtual message_block*	allocate(u32 _flags, u32 _size)
			{
				void* message_mem = mOurAllocator->allocate(_size, sizeof(void*));
				void* block_mem = mOurAllocator->allocate(sizeof(message_block), sizeof(void*));
				message_block* msg_block = new (block_mem)message_block(message_mem, _size, _flags);
				return msg_block;
			}

			virtual void			deallocate(message_block* _msg_block)
			{
				mOurAllocator->deallocate(_msg_block);
			}

			message*				allocate(ipeer* _from, ipeer* _to, u32 _flags, u32 _size)
			{
				message* message = allocate(_from, _to, _flags);
				message_block* block = allocate(_flags, _size);
				message->add_block(block);
				return message;
			}

			static MyMessageAllocator*	sCreate(x_iallocator* inSystemAllocator)
			{
				// Create a heap, pool, caching or any other type of allocator
				return NULL;
			}

			static void					sRelease(MyMessageAllocator*& allocator)
			{
				// Should we call the destructor of our allocator?
				allocator->mSystemAllocator->deallocate(allocator->mOurAllocator);
				allocator = NULL;
			}

		private:
			~MyMessageAllocator() { }

			x_iallocator*	mSystemAllocator;
			x_iallocator*	mOurAllocator;
		};

		class MyPeer
		{
		public:
			xp2p::node*		node;
			xp2p::ipeer*	host;
			xp2p::ipeer*	remote;

			MyAllocator*		ourSystemAllocator;
			MyMessageAllocator*	ourMessageAllocator;

			MyPeer()
			{
				node = NULL;
				host = NULL;
				remote = NULL;
				ourSystemAllocator = NULL;
				ourMessageAllocator = NULL;
			}

			void Start(u16 local_port, u16 remote_port, x_iallocator* inSystemAllocator)
			{
				ourSystemAllocator = MyAllocator::sCreate(inSystemAllocator);
				ourMessageAllocator = MyMessageAllocator::sCreate(inSystemAllocator);

				host = node->start(netip4().set_port(local_port), ourSystemAllocator, ourMessageAllocator);

				// Let's connect to the remote if requested
				if (remote_port > 0)
				{
					remote = node->register_peer(netip4(127, 0, 0, 1).set_port(remote_port));
					node->connect_to(remote);
				}
			}

			void Tick(bool do_send_message)
			{
				incoming_messages* rcvd_messages = NULL;
				outgoing_messages* sent_messages = NULL;

				if (node->event_loop(rcvd_messages, sent_messages, 100))	// Wait a maximum of 1000 ms
				{
					outgoing_messages to_send;

					while (rcvd_messages->has_message())
					{
						ipeer* from = rcvd_messages->get_from();
						{
							if (rcvd_messages->has_event())
							{
								if (rcvd_messages->event_is_connected())
								{
									if (do_send_message)
									{
										message* tmsg = ourMessageAllocator->allocate(from, rcvd_messages->get_from(), MSG_FLAG_ANNOUNCE, 40);
										message_writer writer = tmsg->get_writer();
										writer.write_string("Hello how are you?");
										to_send.enqueue(tmsg);
									}
								}
								else if (rcvd_messages->event_disconnected())
								{
									// Remote peer has disconnected or cannot connect
									break;
								}
							}

							if (rcvd_messages->has_data())
							{
								message_reader reader = rcvd_messages->get_reader();

								char ip4_str[32];
								from->get_ip4().to_string(ip4_str, sizeof(ip4_str));

								if (xbfIsSet(rcvd_messages->get_flags(), MSG_FLAG_ANNOUNCE))
								{
									///@ actually we should be getting our peer-id from the message
								}

								u32 msgStringLen = 0;
								const char* msgString = "";
								reader.view_string(msgString, msgStringLen);
								x_printf("info: message \"%s\"received from \"%s\"", x_va_list(x_va((const char*)msgString), x_va(ip4_str)));
							}
						}

						message* msg = rcvd_messages->dequeue();
						ourMessageAllocator->deallocate(msg);
					}
					node->send(to_send);

					// release all messages that where sent
					while (sent_messages->has_message())
					{
						message* msg = sent_messages->dequeue();
						ourMessageAllocator->deallocate(msg);
					}
				}
			}

			void Stop()
			{
				// Clear all pointers
				remote = NULL;

				// Stop server, close all sockets, release all resources
				node->stop();

				// Release our allocators and their memory back to the system
				MyAllocator::sRelease(ourSystemAllocator);
				MyMessageAllocator::sRelease(ourMessageAllocator);
			}
		};

	}
}




UNITTEST_SUITE_BEGIN(p2p)
{
	UNITTEST_FIXTURE(main)
	{
		UNITTEST_FIXTURE_SETUP()
		{
		}

		UNITTEST_FIXTURE_TEARDOWN()
		{
		}

		UNITTEST_TEST(alloc_dealloc)
		{
		}

		UNITTEST_TEST(peer_to_tracker)
		{
			xcore::xp2p::MyPeer		local;
			xcore::xp2p::MyPeer		tracker;

			local.Start(51888, 51889, gTestAllocator);
			tracker.Start(51889, 0, gTestAllocator);

			local.Tick(true);
			tracker.Tick(false);

			local.Stop();
			tracker.Stop();
		}

		UNITTEST_TEST(dequeue)
		{
		}
	}
}
UNITTEST_SUITE_END
