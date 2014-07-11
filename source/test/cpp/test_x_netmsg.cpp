#include "xp2p\private\x_netio.h"
#include "xp2p\private\x_netmsg.h"

#include "xbase\x_allocator.h"
#include "xunittest\xunittest.h"

using namespace xcore;
extern x_iallocator* gTestAllocator;

class ns_test_allocator : public xcore::xnetio::ns_allocator
{
public:
	virtual void*	alloc(u32 _size, u32 _alignment)
	{
		return gTestAllocator->allocate(_size, _alignment);
	}

	virtual void	dealloc(void* _old)
	{
		gTestAllocator->deallocate(_old);
	}
};

UNITTEST_SUITE_BEGIN(x_netmsg)
{
	UNITTEST_FIXTURE(main)
	{
		static ns_test_allocator ns_allocator_object;
		static ns_test_allocator* ns_allocator = &ns_allocator_object;

		UNITTEST_FIXTURE_SETUP()
		{
		}

		UNITTEST_FIXTURE_TEARDOWN()
		{
		}

		UNITTEST_TEST(alloc_dealloc)
		{
			xnetio::ns_message * m1 = xnetio::ns_message_alloc(ns_allocator, xnetio::NS_MSG_TYPE_EVENT_CONNECTED, 4);

			xnetio::ns_message_dealloc(m1);
		}

		UNITTEST_TEST(enqueue)
		{
			xnetio::ns_message * m1 = xnetio::ns_message_alloc(ns_allocator, xnetio::NS_MSG_TYPE_EVENT_CONNECTED, 4);
			xnetio::ns_message * m2 = xnetio::ns_message_alloc(ns_allocator, xnetio::NS_MSG_TYPE_EVENT_CONNECTED, 4);
			xnetio::ns_message * m3 = xnetio::ns_message_alloc(ns_allocator, xnetio::NS_MSG_TYPE_EVENT_CONNECTED, 4);

			xnetio::ns_message * queue = NULL;
			ns_message_enqueue(queue, m1);
			CHECK_EQUAL(queue, m1);
			ns_message_enqueue(queue, m2);
			CHECK_EQUAL(queue, m2);
			ns_message_enqueue(queue, m3);
			CHECK_EQUAL(queue, m3);

			xnetio::ns_message_dealloc(m1);
			xnetio::ns_message_dealloc(m2);
			xnetio::ns_message_dealloc(m3);
		}

		UNITTEST_TEST(dequeue)
		{
			xnetio::ns_message * m1 = xnetio::ns_message_alloc(ns_allocator, xnetio::NS_MSG_TYPE_EVENT_CONNECTED, 4);
			xnetio::ns_message * m2 = xnetio::ns_message_alloc(ns_allocator, xnetio::NS_MSG_TYPE_EVENT_CONNECTED, 4);
			xnetio::ns_message * m3 = xnetio::ns_message_alloc(ns_allocator, xnetio::NS_MSG_TYPE_EVENT_CONNECTED, 4);

			xnetio::ns_message * queue = NULL;
			ns_message_enqueue(queue, m1);
			CHECK_EQUAL(queue, m1);
			ns_message_enqueue(queue, m2);
			CHECK_EQUAL(queue, m2);
			ns_message_enqueue(queue, m3);
			CHECK_EQUAL(queue, m3);

			xnetio::ns_message * d1 = ns_message_dequeue(queue); 
			CHECK_EQUAL(queue, m3);
			CHECK_EQUAL(d1, m1);
			xnetio::ns_message * d2 = ns_message_dequeue(queue); 
			CHECK_EQUAL(queue, m3);
			CHECK_EQUAL(d2, m2);
			xnetio::ns_message * d3 = ns_message_dequeue(queue); 
			CHECK_EQUAL(queue, (xnetio::ns_message *)NULL);
			CHECK_EQUAL(d3, m3);

			xnetio::ns_message_dealloc(m1);
			xnetio::ns_message_dealloc(m2);
			xnetio::ns_message_dealloc(m3);
		}
	}
}
UNITTEST_SUITE_END
