#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "udt\udt.h"

namespace xcore
{
	namespace xp2p
	{
		/// IO Write Thread
		/// - For all UDT sockets that require writing:
		///   - schedule them for epoll
		///   - add the signal socket (which can wake up this thread when new sockets require writing)

		/// IO Read and Accept Thread
		/// Reading messages is using the read-message-memory-allocator
		/// - For all UDT sockets that require reading:
		///   - schedule them for epoll
		///   - add the listen socket
		///   - add the signal socket (which can wake up this thread when new sockets require reading)


		/// Note: Ownership of messages (data) to the one using xp2p would be preferred:
		/// Receiving messages on the

		class Node
		{
		public:

		protected:
			static void		sThreadProc(Node*);

			struct socket_t
			{
				UDTSOCKET	socket_;
			};

			// Queue<WriteItem>		writes_;	/// System is pushing write messages in here
			// Queue<ReadItem>		reads_;		/// Push all the read messages in here

			// std::queue<socket_t>			sockets_to_write;
			// std::vector<out_message>		messages_to_send;
		};

		void		Node::sThreadProc(Node* self)
		{
			UDTSOCKET serv = UDT::socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in my_addr;
			my_addr.sin_family = AF_INET;
			my_addr.sin_port = htons(9000);
			my_addr.sin_addr.s_addr = INADDR_ANY;
			memset(&(my_addr.sin_zero), '\0', 8);

			if (UDT::ERROR != UDT::bind(serv, (sockaddr*)&my_addr, sizeof(my_addr)))
			{
				UDT::listen(serv, 10);

				s32 epoller = UDT::epoll_create();
			
				// Gather all the sockets that need writing
				// Schedule all sockets for reading
				// Can we add the listen socket also ?
				// Add a trigger socket that we can use to wake-up this thread when
				// new sockets require writing ?
			
				// Process the 

				// Process the sockets that have data on them, read the messages and
				// push them in the read queue.
			
				UDT::epoll_release(epoller);
			}
		}
	}
}