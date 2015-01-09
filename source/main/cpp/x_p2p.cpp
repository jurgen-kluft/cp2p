#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "xp2p\x_p2p.h"
#include "xp2p\x_peer.h"
#include "xp2p\x_msg.h"
#include "xp2p\private\x_allocator.h"
#include "xp2p\private\x_peer_registry.h"
#include "xp2p\private\x_netio.h"
#include "xp2p\private\x_netio_proto.h"

#define NOT		false == 

namespace xcore
{
	namespace xp2p
	{
		class peer_connection;

		class peer : public ipeer
		{
		public:
			peer() 
				: is_remote_(false)
				, status_(INACTIVE)
				, endpoint_()
			{
			}

			// peer interface
			virtual bool		is_remote() const			{ return is_remote_; }
			virtual estatus		get_status() const			{ return status_; }
			virtual netip4		get_ip4() const				{ return endpoint_; }

			inline bool			is_not_connected() const	{ estatus s = get_status(); return s==0 || s>=DISCONNECT; }
			inline void			set_status(estatus s)		{ status_ = s; }

			enum epeer { LOCAL_PEER, REMOTE_PEER };
			void				init(epeer _peer, estatus _status, netip4 _ip)
			{
				is_remote_ = _peer == REMOTE_PEER;
				status_ = _status;
				endpoint_ = _ip;
			}

		protected:
			bool				is_remote_;
			estatus				status_;
			netip4				endpoint_;
		};

		class node::node_imp : public peer, public io_protocol, public ns_allocator
		{
		public:
								node_imp(iallocator* _allocator, imessage_allocator* _message_allocator);
								~node_imp();

			ipeer*				start(netip4 _endpoint);
			void				stop();

			ipeer*				register_peer(netip4 _endpoint);
			void				unregister_peer(ipeer*);

			void				connect_to(ipeer* _peer);
			void				disconnect_from(ipeer* _peer);

			u32					connections(ipeer** _out_peers, u32 _in_max_peers);

			void				send(outgoing_messages&);

			void				event_wakeup();
			bool				event_loop(incoming_messages*& _rcvd, outgoing_messages*& _sent, u32 _ms_to_wait);

			// ns_allocator interface
			virtual void*		ns_allocate(u32 _size, u32 _alignment);
			virtual void		ns_deallocate(void* _old);

			// io_protocol interface
			virtual io_connection io_open(netip4 _ip);
			virtual void		io_close(io_connection);

			virtual bool		io_needs_write(io_connection);
			virtual bool		io_needs_read(io_connection);

			virtual s32			io_write(io_connection, io_writer*);
			virtual s32			io_read(io_connection, io_reader*);

			virtual void		io_callback(io_connection, event, void *evp);

			XCORE_CLASS_PLACEMENT_NEW_DELETE

			iallocator*			allocator_;
			imessage_allocator*	message_allocator_;
			ns_iserver*			server_;
			
			lqueue<peer_connection>* inactive_peers_;
			lqueue<peer_connection>* active_peers_;

			incoming_messages	incoming_messages_;
		};

		class marker32
		{
		public:
			inline				marker32() : text_(0) {}
			inline				marker32(char const* _str) : text_(0)
			{
				const char* s = _str;
				while (*s != '\0')
					add(*s++);
			}

			inline 				operator u32() const							{ return text_; }

			inline marker32&	add(char c)
			{
				if (c>='a' && c<='z') c -= 'a';
				else if (c>='A' && c<='Z') c -= 'A';
				else if (c>='0' && c<='9') c -= '0';
				if      (c <  2) text_ = (text_ << 1) | (c & 0x1);
				else if (c <  4) text_ = (text_ << 2) | (c & 0x3);
				else if (c <  8) text_ = (text_ << 3) | (c & 0x7);
				else if (c < 16) text_ = (text_ << 4) | (c & 0xF);
				else if (c < 32) text_ = (text_ << 5) | (c & 0x1F);
				return *this;
			}

		protected:
			u32					text_;
		};

		// -------------------------------------------------------------------------------------------
		// IO Message Writer
		// -------------------------------------------------------------------------------------------

		class imessage_io_writer
		{
		public:
			virtual s32			write(io_writer*) = 0;
		};
		
		class msg_hdr_io_writer : public imessage_io_writer
		{
		public:
			inline				msg_hdr_io_writer() : size_(0), written_(0), data_(NULL) {}
			
			bool				is_done() const
			{
				return written_ == size_; 
			}

			void				init_message(u32 _size, u32 _flags)
			{
				marker32 marker("MSGBEGIN");
				header_[0] = marker;
				header_[1] = _size;
				header_[2] = _flags;

				size_ = sizeof(header_);
				data_ = (xbyte const*)&header_[0];
				written_ = 0;
			}

			void				init_block(u32 _size, u32 _flags)
			{
				marker32 marker("MSGBLOCK");
				header_[0] = marker;
				header_[1] = _size;
				header_[2] = _flags;

				size_ = sizeof(header_);
				data_ = (xbyte const*)&header_[0];
				written_ = 0;
			}

			void				init_end()
			{
				marker32 marker("MSGEND");
				header_[0] = marker;
				header_[1] = 0;
				header_[2] = 0;

				size_ = sizeof(header_);
				data_ = (xbyte const*)&header_[0];
				written_ = 0;
			}

			virtual s32			write(io_writer* _writer)
			{
				if (written_ < size_)
				{
					s32 const w = _writer->write(data_ + written_, size_ - written_);
					if (w == -1)
						return -1;
					written_ += w;
				}
				return written_ == size_ ? 0 : 1;
			}

		protected:
			u32					header_[3];	/// { marker, _size, _flags }

			u32					size_;		/// Size = 12
			u32					written_;
			xbyte const*		data_;		/// Points to &header_[0]
		};

		class msg_data_io_writer : public imessage_io_writer
		{
		public:
			inline				msg_data_io_writer() : size_(0), written_(0), data_(NULL) {}

			bool				is_done() const
			{
				return written_ == size_; 
			} 

			void				init(u32 _size, void const* _data)
			{
				size_ = _size;
				data_ = (xbyte const*)_data;
				written_ = 0;
			}

			virtual s32			write(io_writer* _writer)
			{
				if (written_ < size_)
				{
					s32 const w = _writer->write(data_ + written_, size_ - written_);
					if (w == -1)
						return -1;
					written_ += w;
				}
				return written_ == size_ ? 0 : 1;
			}

		protected:
			u32					size_;		/// _size
			u32					written_;
			xbyte const*		data_;		/// _data
		};

		class peer_io_writer
		{
		public:
			inline				peer_io_writer() 
				: io_writer_state_(STATE_PREPARE_MESSAGE)
			{

			}

			// IO message writer
			enum estate
			{
				STATE_PREPARE_MESSAGE = 0,
				STATE_PREPARE_BLOCK = 1,
				STATE_WRITE_MESSAGE_HEADER = 10,
				STATE_WRITE_MESSAGE_END = 11,
				STATE_WRITE_BLOCK_HEADER = 20,
				STATE_WRITE_BLOCK_DATA = 30,
			};

			void				reset()
			{
				io_writer_state_ = STATE_PREPARE_MESSAGE;
			}

			s32					write(outgoing_messages& outgoing_messages, io_writer* iow)
			{
				bool everything_ok = true;
				while (everything_ok)
				{
					switch (io_writer_state_)
					{
					case STATE_PREPARE_MESSAGE:
						{
							// Prepare for another message or message-block
							if (outgoing_messages.has_message())
							{
								io_writer_msg_reader_ = outgoing_messages.get_reader();
								io_writer_header_.init_message(io_writer_msg_reader_.get_size(), io_writer_msg_reader_.get_flags());
								io_writer_state_ = STATE_WRITE_MESSAGE_HEADER;
							}
							else
							{
								/// No more outgoing messages to process so exit
								return 0;
							}

						} break;
					case STATE_PREPARE_BLOCK:
						{
							// Prepare for another message or message-block
							u32 msg_block_size = 0;
							xbyte const* msg_block_data = NULL;
							io_writer_msg_reader_.view_data(msg_block_data, msg_block_size);
							u32 const msg_block_flags = io_writer_msg_reader_.get_flags();
							io_writer_header_.init_block(msg_block_size, msg_block_flags);
							io_writer_block_data_.init(msg_block_size, msg_block_data);
							
							io_writer_msg_reader_.next_block();
							io_writer_state_ = STATE_WRITE_BLOCK_HEADER;
						} break;
					case STATE_WRITE_MESSAGE_HEADER:
						{
							s32 const result = io_writer_header_.write(iow);
							if (result == 0)
							{
								io_writer_state_ = STATE_PREPARE_BLOCK;
							}
							else if (result == -1)
							{
								// IO error (socket disconnect)
								return -1;
							}
							else
							{
								// IO write is full
								return 1;
							}
						} break;
					case STATE_WRITE_BLOCK_HEADER:
						{
							s32 const result = io_writer_header_.write(iow);
							if (result == 0)
							{
								io_writer_state_ = STATE_WRITE_BLOCK_DATA;
							}
							else if (result == -1)
							{
								// IO error (socket disconnect)
								return -1;
							}
							else
							{
								// IO write is full
								return 1;
							}
						} break;
					case STATE_WRITE_MESSAGE_END:
						{
							s32 const result = io_writer_header_.write(iow);
							if (result == 0)
							{
								io_writer_state_ = STATE_PREPARE_MESSAGE;
							}
							else if (result == -1)
							{
								// IO error (socket disconnect)
								return -1;
							}
							else
							{
								// IO write is full
								return 1;
							}
						} break;
					case STATE_WRITE_BLOCK_DATA:
						{
							s32 const result = io_writer_block_data_.write(iow);
							if (result == 0)
							{
								if (io_writer_msg_reader_.has_block())
								{
									io_writer_state_ = STATE_PREPARE_BLOCK;
								}
								else
								{
									io_writer_header_.init_end();
									io_writer_state_ = STATE_WRITE_MESSAGE_END;
								}
							}
							else if (result == -1)
							{
								// IO error (socket disconnect)
								return -1;
							}
							else
							{
								// IO write is full
								return 1;
							}
						} break;
					}
				}
				return everything_ok ? 1 : -1;
			}

			inline bool			needs_write() const
			{
				return NOT io_writer_header_.is_done() || NOT io_writer_block_data_.is_done() || io_writer_msg_reader_.has_block();
			}

			estate				io_writer_state_;
			msg_hdr_io_writer	io_writer_header_;
			msg_data_io_writer	io_writer_block_data_;
			message_reader		io_writer_msg_reader_;
		};


		// -------------------------------------------------------------------------------------------
		// IO Message Reader
		// -------------------------------------------------------------------------------------------

		class imessage_io_reader
		{
		public:
			virtual s32			read(io_reader*) = 0;
		};

		class msg_hdr_io_reader : public imessage_io_reader
		{
		public:
			inline				msg_hdr_io_reader() : size_(0), read_(0), data_(0) { }

			u32					get_size() const					{ return header_[1]; }
			u32					get_flags() const					{ return header_[2]; }

			bool				is_msg_begin_header() const			{ return header_[0] == marker32("MSGBEGIN"); }
			bool				is_block_header() const				{ return header_[0] == marker32("MSGBLOCK"); }
			bool				is_msg_end_header() const			{ return header_[0] == marker32("MSGEND"); }

			void				init()								{ read_ = 0; size_ = sizeof(header_); data_ = (xbyte*)&header_[0]; }

			virtual s32			read(io_reader* _reader)
			{
				if (read_ < size_)
				{
					s32 const r = _reader->read(data_ + read_, size_ - read_);
					if (r == -1)
						return -1;
					read_ += r;
				}
				return read_ == size_ ? 0 : 1;
			}

		protected:
			u32					header_[3];	/// { marker, _size, _flags }

			u32					size_;		/// Size = 12
			u32					read_;
			xbyte*				data_;		/// Points to &header_[0]
		};

		class msg_data_io_reader : public imessage_io_reader
		{
		public:
			inline				msg_data_io_reader() : read_(0), size_(0), block_(NULL) { }

			void				init(message_block* _block)			{ block_ = _block; read_ = 0; size_ = _block->get_size(); }

			virtual s32			read(io_reader* _reader)
			{
				if (read_ < size_)
				{
					s32 const r = _reader->read(block_->get_data() + read_, size_ - read_);
					if (r == -1)
						return -1;
					read_ += r;
				}
				return read_ == size_ ? 0 : 1;
			}

		protected:
			u32					read_;
			u32					size_;
			message_block*		block_;
		};

		class peer_io_reader
		{
		public:
			inline				peer_io_reader() 
				: io_reader_state_(STATE_READ_MSG_HEADER)
				, io_reader_msg_block_(NULL)
				, io_reader_msg_(NULL)
			{

			}

			// IO message reader
			enum estate
			{
				STATE_READ_MSG_HEADER = 1,
				STATE_READ_BLOCK_HEADER = 2,
				STATE_READ_BLOCK_DATA = 3,
			};

			void				reset(imessage_allocator* _message_allocator)
			{
				io_reader_state_ = STATE_READ_MSG_HEADER;
				if (io_reader_msg_block_ != NULL)
				{
					_message_allocator->deallocate(io_reader_msg_block_);
					io_reader_msg_block_ = NULL;
				}
				if (io_reader_msg_ != NULL)
				{
					_message_allocator->deallocate(io_reader_msg_);
					io_reader_msg_ = NULL;
				}
			}

			s32					read(peer* _local_peer, peer* _remote_peer, incoming_messages& _incoming_messages, imessage_allocator* _message_allocator, io_reader* _io_reader)
			{
				switch(io_reader_state_)
				{
				case STATE_READ_MSG_HEADER:
					{
						/// Read message header
						/// - allocate message
						/// - continue reading block header
						s32 const result = io_reader_header_.read(_io_reader);
						if (result == 0)
						{
							if (io_reader_header_.is_msg_begin_header())
							{
								io_reader_msg_ = _message_allocator->allocate(_remote_peer, _local_peer, io_reader_header_.get_flags());
								io_reader_header_.init();
								io_reader_state_ = STATE_READ_BLOCK_HEADER;
							}
							else
							{
								// protocol error
								return -100;
							}
						}
						else if (result == -1)
						{
							// IO error (socket disconnect)
							return -1;
						}
					} break;
				case STATE_READ_BLOCK_HEADER:
					{
						/// Read block header
						/// - if header is 'END' then move to read new message
						/// - allocate message block
						/// - move state to STATE_READ_BLOCK_DATA
						s32 const result = io_reader_header_.read(_io_reader);
						if (result == 0)
						{
							if (io_reader_header_.is_block_header())
							{
								u32 block_size = io_reader_header_.get_size();
								io_reader_msg_block_ = _message_allocator->allocate(io_reader_header_.get_flags(), block_size);
								io_reader_msg_block_data_.init(io_reader_msg_block_);
								io_reader_state_ = STATE_READ_BLOCK_DATA;
							}
							else if (io_reader_header_.is_msg_end_header())
							{
								// push message onto the incoming messages queue
								_incoming_messages.enqueue(io_reader_msg_);

								// prepare for new incoming message
								io_reader_msg_ = NULL;
								io_reader_header_.init();
								io_reader_state_ = STATE_READ_MSG_HEADER;
							}
							else
							{
								// protocol error
								return -100;
							}
						}
						else if (result == -1)
						{
							// IO error (socket disconnect)
							return -1;
						}
					} break;
				case STATE_READ_BLOCK_DATA:
					{
						/// Read message block data
						/// - read block data
						/// - when block data complete, add block to message
						/// - move state to STATE_READ_HEADER
						s32 const result = io_reader_msg_block_data_.read(_io_reader);
						if (result == 0)
						{
							io_reader_msg_->add_block(io_reader_msg_block_);
							io_reader_msg_block_ = NULL;
							io_reader_header_.init();
							io_reader_state_ = STATE_READ_BLOCK_HEADER;
						}
						else if (result == -1)
						{
							// IO error (socket disconnect)
							return -1;
						}
					} break;
				}
				return 0;
			}

			estate				io_reader_state_;
			msg_hdr_io_reader	io_reader_header_;
			message_block*		io_reader_msg_block_;
			msg_data_io_reader	io_reader_msg_block_data_;
			message*			io_reader_msg_;
		};


		// -------------------------------------------------------------------------------------------
		// Peer Connection
		// -------------------------------------------------------------------------------------------
		class peer_connection : public peer, public lqueue<peer_connection>
		{
		public:
			peer_connection() 
				: peer()
				, lqueue<peer_connection>(this) 
				, connection_(NULL)
			{
			}

			XCORE_CLASS_PLACEMENT_NEW_DELETE

			ns_connection*		connection_;

			outgoing_messages	outgoing_messages_;
			incoming_messages	incoming_messages_;

			peer_io_writer		io_message_writer_;
			peer_io_reader		io_message_reader_;
		};

		node::node_imp::node_imp(iallocator* _allocator, imessage_allocator* _message_allocator)
			: peer()
			, allocator_(_allocator)
			, message_allocator_(_message_allocator)
			, inactive_peers_(NULL)
			, active_peers_(NULL)
		{

		}

		node::node_imp::~node_imp()
		{

		}

		ipeer*	node::node_imp::start(netip4 _endpoint)
		{
			init(peer_connection::LOCAL_PEER, CONNECTED, _endpoint);

			server_ = ns_create_server(this);
			server_->start(this, this);
			return this;
		}

		void	node::node_imp::stop()
		{
			server_->release();
		}

		peer_connection*	_find_peer(lqueue<peer_connection>* _peers, netip4 _endpoint)
		{
			peer_connection* peer = NULL;
			if (_peers == NULL)
				return peer;

			peer_connection* iter = _peers->get();
			while (iter != NULL)
			{
				if (iter->get_ip4() == _endpoint)
				{
					peer = iter;
					break;
				}
				iter = iter->get_next();
			}
			return peer;
		}

		ipeer*	node::node_imp::register_peer(netip4 _endpoint)
		{
			peer_connection* peer = _find_peer(inactive_peers_, _endpoint);
			if (peer == NULL)
			{
				peer = _find_peer(active_peers_, _endpoint);
			}

			if (peer == NULL)
			{
				void * mem = allocator_->allocate(sizeof(peer_connection), sizeof(void*));
				peer = new (mem) peer_connection();
				peer->init(peer_connection::REMOTE_PEER, INACTIVE, _endpoint);
			}

			return peer;
		}

		void	node::node_imp::unregister_peer(ipeer* _peer)
		{
			peer_connection* p = (peer_connection*) _peer;
			if (p == active_peers_)
				active_peers_ = active_peers_->get_next();
			else if (p == inactive_peers_)
				inactive_peers_ = inactive_peers_->get_next();
			p->dequeue();
		}

		void	node::node_imp::connect_to(ipeer* _peer)
		{
			if (_peer == this)	// Do not connect to ourselves
				return;
			
			peer_connection* p = (peer_connection*) _peer;
			if (p->connection_ == NULL)
			{
				p->set_status(ipeer::CONNECT);
				p->connection_ = server_->connect(_peer->get_ip4());
			}
		}

		void	node::node_imp::disconnect_from(ipeer* _peer)
		{
			peer_connection* p = (peer_connection*) _peer;
			server_->disconnect(p->connection_);
			p->set_status(ipeer::DISCONNECTING);
		}

		u32		node::node_imp::connections(ipeer** _out_peers, u32 _in_max_peers)
		{
			u32 i = 0;
			if (active_peers_ != NULL)
			{
				peer_connection* iter = active_peers_->get();
				while (iter != NULL && i < _in_max_peers)
				{
					_out_peers[i++] = iter;
					iter = iter->get_next();
				}
			}
			return i;
		}

		void	node::node_imp::send(outgoing_messages& _msg)
		{
			message* m = _msg.dequeue();
			while (m != NULL)
			{
				peer_connection* to = (peer_connection*)(m->get_to());
				to->outgoing_messages_.enqueue(m);
				m = _msg.dequeue();
			}
		}

		void	node::node_imp::event_wakeup()
		{
			server_->wakeup();
		}

		bool	node::node_imp::event_loop(incoming_messages*&, outgoing_messages*& _sent, u32 _ms_to_wait)
		{
			s32 const num_connections = server_->poll(_ms_to_wait);
			return true;
		}

		// -------------------------------------------------------------------------------------------

		void	node::node_imp::io_callback(io_connection conn, event e, void *evp)
		{
			switch (e)
			{
				case io_protocol::EVENT_POLL: 
					break;
				case io_protocol::EVENT_ACCEPT: 
					{
						peer_connection* pc = (peer_connection*)conn;
						pc->set_status(ipeer::CONNECTED);

						u32 const flags = (message::MESSAGE_FLAG_EVENT) | (message::MESSAGE_FLAG_EVENT_CONNECTED);
						message* msg = message_allocator_->allocate(this, pc, flags);
						incoming_messages_.enqueue(msg);

					} break;
				case io_protocol::EVENT_CONNECT: 
					{
						peer_connection* pc = (peer_connection*)conn;
						pc->set_status(ipeer::CONNECTED);

						u32 const flags = (message::MESSAGE_FLAG_EVENT) | (message::MESSAGE_FLAG_EVENT_CONNECTED);
						message* msg = message_allocator_->allocate(this, pc, flags);
						incoming_messages_.enqueue(msg);

					} break;
				case io_protocol::EVENT_CLOSE:
					{
						peer_connection* pc = (peer_connection*)conn;
						pc->set_status(ipeer::DISCONNECTED);

						u32 const flags = (message::MESSAGE_FLAG_EVENT) | (message::MESSAGE_FLAG_EVENT_DISCONNECTED);
						message* msg = message_allocator_->allocate(this, pc, flags);
						incoming_messages_.enqueue(msg);

					} break;
			}
		}

		// -------------------------------------------------------------------------------------------

		void*		node::node_imp::ns_allocate(u32 _size, u32 _alignment)
		{
			return allocator_->allocate(_size, _alignment);
		}

		void		node::node_imp::ns_deallocate(void* _old)
		{
			allocator_->deallocate(_old);
		}

		// -------------------------------------------------------------------------------------------

		io_connection node::node_imp::io_open(netip4 _ip)
		{
			// Search in the peer registry, if not found create a new peer connection
			// Set the key on the peer connection so that we can find it next time

			return NULL;
		}

		void		node::node_imp::io_close(io_connection ioc)
		{
			peer_connection* pc = (peer_connection*)ioc;
		}

		bool		node::node_imp::io_needs_write(io_connection ioc)
		{
			peer_connection* pc = (peer_connection*)ioc;
			return pc->outgoing_messages_.has_message() || pc->io_message_writer_.needs_write();
		}

		bool		node::node_imp::io_needs_read(io_connection ioc)
		{
			return true;
		}

		s32			node::node_imp::io_write(io_connection ioc, io_writer* iow)
		{
			peer_connection* pc = (peer_connection*)ioc;
			return pc->io_message_writer_.write(pc->outgoing_messages_, iow);
		}

		s32			node::node_imp::io_read(io_connection ioc, io_reader* ior)
		{
			peer_connection* pc = (peer_connection*)ioc;
			return pc->io_message_reader_.read(this, pc, pc->incoming_messages_, message_allocator_, ior);
		}


		// -------------------------------------------------------------------------------------------
		// P2P Node
		// -------------------------------------------------------------------------------------------

		node::node() 
			: imp_(NULL)
		{
		}

		ipeer*				node::start(netip4 _endpoint, iallocator* _allocator, imessage_allocator* _message_allocator)
		{
			void * mem = _allocator->allocate(sizeof(node::node_imp), sizeof(void*));
			imp_ = new (mem) node::node_imp(_allocator, _message_allocator);
			return imp_->start(_endpoint);
		}

		void				node::stop()
		{
			imp_->stop();

		}

		ipeer*				node::register_peer(netip4 _endpoint)
		{
			return imp_->register_peer(_endpoint);
		}

		void				node::unregister_peer(ipeer* _peer)
		{
			imp_->unregister_peer(_peer);
		}

		void				node::connect_to(ipeer* _peer)
		{
			imp_->connect_to(_peer);
		}

		void				node::disconnect_from(ipeer* _peer)
		{
			imp_->disconnect_from(_peer);
		}

		u32					node::connections(ipeer** _out_peers, u32 _in_max_peers)
		{
			return imp_->connections(_out_peers, _in_max_peers);
		}

		void				node::send(outgoing_messages& _msg)
		{
			imp_->send(_msg);
		}

		void				node::event_wakeup()
		{
			imp_->event_wakeup();
		}

		bool				node::event_loop(incoming_messages*& _msgs, outgoing_messages*& _sent, u32 _wait_in_ms)
		{
			return imp_->event_loop(_msgs, _sent, _wait_in_ms);
		}
	}
}