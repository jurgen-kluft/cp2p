#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "xp2p\private\x_netio_proto.h"
#include "xp2p\private\x_netio.h"
#include "xp2p\x_msg.h"

namespace xcore
{
	namespace xp2p
	{
		class imessage_sender
		{
		public:
			virtual s32				write(xnetio::io_writer*) = 0;
		};

		class imessage_receiver
		{
		public:
			virtual s32				read(xnetio::io_reader*) = 0;
		};



	}
}