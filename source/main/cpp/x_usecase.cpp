#include "xbase\x_target.h"
#include "xbase\x_allocator.h"
#include "xbase\x_va_list.h"
#include "xbase\x_string_ascii.h"
#include "xbase\x_bit_field.h"

#include "xp2p\x_p2p.h"
#include "xp2p\x_peer.h"
#include "xp2p\x_msg.h"
#include "xp2p\private\x_allocator.h"

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


		class MyMessageAllocator : public imessage_allocator
		{
		public:
								MyMessageAllocator(x_iallocator* inSystemAllocator) : mSystemAllocator(inSystemAllocator) {}


			virtual message*	allocate(ipeer* _from, ipeer* _to, u32 _flags)
			{
				void* mem = mOurAllocator->allocate(sizeof(message), sizeof(void*));
				message* msg = new (mem) message(_from, _to, _flags);
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
				message_block* msg_block = new (block_mem) message_block(message_mem, _size, _flags);
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

			static MyMessageAllocator*	sCreate(x_iallocator* inSystemAllocator, u32 memsize)
			{
				// Create a heap allocator or any other type of allocator
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



		static void ExampleUseCase(x_iallocator* inSystemAllocator)
		{
			MyAllocator* ourSystemAllocator = MyAllocator::sCreate(inSystemAllocator, 2 * 1024 * 1024);
			MyMessageAllocator* ourMessageAllocator = MyMessageAllocator::sCreate(inSystemAllocator, 24 * 1024 * 1024);

			bool start_as_peer = true;

			if (start_as_peer)
			{
				xp2p::node system;
				xp2p::node* node = &system;
				ipeer* host = node->start(netip4().set_port(51888), ourSystemAllocator, ourMessageAllocator);

				// Let's connect to the tracker 
				ipeer* tracker = node->register_peer(netip4(10, 0, 14, 14).set_port(51888));
				node->connect_to(tracker);

				// Let's register the LAN multi-cast node
				ipeer* lpd = node->register_peer(netip4(239, 192, 152, 143).set_port(6771));

				incoming_messages* rcvd_messages;
				outgoing_messages out_messages;
				outgoing_messages* sent_messages;
				while (tracker != NULL)
				{
					if (node->event_loop(rcvd_messages, sent_messages, 1000))	// Wait a maximum of 1000 ms
					{
						while (rcvd_messages->has_message())
						{
							message* rcvdmsg = rcvd_messages->dequeue();

							if (rcvdmsg->is_from(tracker))
							{
								if (rcvdmsg->has_event())
								{
									if (rcvdmsg->event_is_connected())
									{
										message* msg2send = ourMessageAllocator->allocate(tracker, rcvdmsg->get_from(), MSG_FLAG_ANNOUNCE, 40);
										message_writer writer = msg2send->get_writer();
										writer.write_string("Hello tracker, how are you?");
										out_messages.enqueue(msg2send);
									}
									else if (rcvdmsg->event_disconnected())
									{
										// Remote peer has disconnected or cannot connect
										break;
									}
								}
								
								if (rcvdmsg->has_data())
								{
									message_reader reader = rcvdmsg->get_reader();

									char ip4_str[32];
									tracker->get_ip4().to_string(ip4_str, sizeof(ip4_str));

									if (xbfIsSet(rcvdmsg->get_flags(), MSG_FLAG_ANNOUNCE))
									{
										///@ actually we should be getting our peer-id from the message
									}

									u32 msgStringLen = 0;
									const char* msgString = "";
									reader.view_string(msgString, msgStringLen);
									x_printf("info: message \"%s\"received from tracker \"%s\"", x_va_list(x_va((const char*)msgString), x_va(ip4_str)));
								}
							}
							else
							{
								/// break;
							}

							// release received message back to the allocator
							ourMessageAllocator->deallocate(rcvdmsg);
						}

						// send all messages created in this iteration
						node->send(out_messages);

						// release all messages that where sent
						while (sent_messages->has_message())
						{
							message* msg = sent_messages->dequeue();
							ourMessageAllocator->deallocate(msg);
						}
					}
				}

				// Clear all pointers
				tracker = NULL;

				// Stop server, close all sockets, release all resources
				node->stop();
			}
			else
			{
				// Start as Tracker
				xp2p::node system;
				xp2p::node* node = &system;

				// Let's boot as a tracker which always has peerid '0'
				netip4 tracker_ep = netip4().set_port(51888);
				ipeer* tracker = node->start(tracker_ep, ourSystemAllocator, ourMessageAllocator);

				incoming_messages* rcvd_messages;
				outgoing_messages* sent_messages;
				while (tracker != NULL)
				{
					outgoing_messages out_messages;

					if (node->event_loop(rcvd_messages, sent_messages, 1000))	// Wait a maximum of 1000 ms
					{
						while (rcvd_messages->has_message())
						{
							message* rcvdmsg = rcvd_messages->dequeue();

							ipeer* peer = rcvdmsg->get_from();
							if (rcvdmsg->has_event())
							{
								if (rcvdmsg->event_is_connected())
								{
									// Remote peer has connected
								}
								else if (rcvdmsg->event_disconnected())
								{
									// Remote peer has disconnected or cannot connect
									break;
								}
							}
							
							if (rcvdmsg->has_data())
							{
								message_reader reader = rcvdmsg->get_reader();
								char ip4_str[32];
								peer->get_ip4().to_string(ip4_str, sizeof(ip4_str));

								/// Get a direct pointer to the string (instead of copying)
								u32 msgStringLen = 0;
								char const* msgString = "";
								reader.view_string(msgString, msgStringLen);
								x_printf("info: message \"%s\"received from peer \"%s\"", x_va_list(x_va((const char*)msgString), x_va(ip4_str)));

								// Send back a message
								message* tmsg = ourMessageAllocator->allocate(tracker, rcvdmsg->get_from(), MSG_FLAG_ANNOUNCE, 40);
								message_writer writer = tmsg->get_writer();
								writer.write_string("Hello peer, i am fine!");
								out_messages.enqueue(tmsg);
							}
						}
					}

					// send outgoing messages
					node->send(out_messages);

					// release all messages that where sent
					while (sent_messages->has_message())
					{
						message* msg = sent_messages->dequeue();
						ourMessageAllocator->deallocate(msg);
					}
				}

				// Clear all pointers
				tracker = NULL;

				// Stop server, close all sockets, release all resources
				node->stop();

			}

			// Release our allocators and their memory back to the system
			MyAllocator::sRelease(ourSystemAllocator);
			MyMessageAllocator::sRelease(ourMessageAllocator);
		}
	}
}
