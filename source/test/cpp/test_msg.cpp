#include "cp2p/c_msg.h"

#include "cbase/c_allocator.h"
#include "cunittest/cunittest.h"

using namespace ncore;
extern alloc_t* gTestAllocator;

class test_message_allocator : public ncore::np2p::message_allocator
{
public:
	virtual np2p::message*		allocate(np2p::peer* _from, np2p::peer* _to, u32 _flags)
	{
		void* mem = gTestAllocator->allocate(sizeof(np2p::message), sizeof(void*));
		np2p::message* msg = new (mem) np2p::message(_from, _to, _flags);
		return msg;
	}

	virtual void				deallocate(np2p::message* _msg)
	{
		gTestAllocator->deallocate(_msg);
	}

	virtual np2p::message_block* allocate(u32 _flags, u32 _size)
	{
		void* mem = gTestAllocator->allocate(sizeof(np2p::message_block) + ((_size + 3) & 0xfffffffc), sizeof(void*));
		void* data = (void*)((byte*)mem + sizeof(np2p::message_block));
		np2p::message_block* block = new (mem) np2p::message_block(data, _size, _flags);
		return block;
	}

	virtual void				deallocate(np2p::message_block* _block)
	{
		gTestAllocator->deallocate(_block);
	}

};

UNITTEST_SUITE_BEGIN(msg)
{
	UNITTEST_FIXTURE(main)
	{
		static test_message_allocator	message_allocator_object;
		static test_message_allocator*	message_allocator = &message_allocator_object;

		UNITTEST_FIXTURE_SETUP()
		{
		}

		UNITTEST_FIXTURE_TEARDOWN()
		{
		}

		UNITTEST_TEST(alloc_dealloc)
		{
		}

		UNITTEST_TEST(reader)
		{
			np2p::peer* from = NULL;
			np2p::peer* to = NULL;

			np2p::message * m1 = message_allocator->allocate(from, to, 128);

			np2p::message_block * b1 = message_allocator->allocate(0, 128);
			m1->set_block(b1);

			np2p::message_writer writer = m1->get_writer();
			np2p::message_reader reader = m1->get_reader();

			u8 u8w=50, u8r=0;
			CHECK_EQUAL(1, writer.write(u8w));
			CHECK_EQUAL(1, reader.read(u8r));
			CHECK_EQUAL(u8w,u8r);

			float fw = 1.0f, fr = 0.0f;
			CHECK_EQUAL(4, writer.write(fw));
			CHECK_EQUAL(4, reader.read(fr));
			CHECK_EQUAL(fw,fr);

			message_allocator->deallocate(b1);
			message_allocator->deallocate(m1);
		}

		UNITTEST_TEST(dequeue)
		{
		}
	}
}
UNITTEST_SUITE_END
