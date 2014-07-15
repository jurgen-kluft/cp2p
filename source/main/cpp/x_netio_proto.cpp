#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "xp2p\private\x_netio_proto.h"
#include "xp2p\private\x_netio.h"
#include "xp2p\x_msg.h"

namespace xcore
{
	namespace xp2p
	{

		class message_allocator
		{
		public:
			virtual void*		allocate(u32 size, u32 alignment) = 0;
			virtual void		deallocate(void*) = 0;
		};

		struct message_header
		{
			inline				message_header() : flags_(0), from_(0), to_(0) {}
			inline				message_header(u32 _flags, peerid _from, peerid _to) : flags_(_flags), from_(_from), to_(_to) {}

			bool				is_event() const;
			bool				is_data() const;

			bool				is_connected() const;
			bool				is_not_connected() const;
			bool				cannot_connect() const;

			bool				is_from(peerid) const;

		private:
			u32					flags_;
			peerid				from_;
			peerid				to_;
		};

		struct message_data
		{
			inline				message_data() : data_(NULL), size_(0), cursor_(0) {}

			u32					size_;
			message_allocator*	allocator_;
			void*				data_;
			u32					cursor_;
		};


		class protocol : public xnetio::io_protocol, public incoming_messages, public outgoing_messages
		{
		public:
			// API for xp2p::node user
			virtual bool			dequeue(incoming_message&);
			virtual bool			enqueue(outgoing_message&);
			virtual bool			dequeue(outgoing_message&);

			// xnetio API
			virtual connection_t	open(void*);
			virtual void			close(connection_t);

			virtual bool			needs_write(connection_t);
			virtual bool			needs_read(connection_t);

			virtual s32				write(connection_t, xnetio::io_writer*);
			virtual s32				read(connection_t, xnetio::io_reader*);

			// ---------------------------------------------------------------
			// an allocator for messages
			message_allocator*		message_allocator_;

			// a queue container for incoming messages

			struct conn
			{
				// bookkeeping data for receiving and sending messages
			};

		};

	}
}