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


	}
}