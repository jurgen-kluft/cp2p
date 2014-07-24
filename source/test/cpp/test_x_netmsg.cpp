#include "xp2p\private\x_netio.h"
#include "xp2p\private\x_netio_proto.h"

#include "xbase\x_allocator.h"
#include "xunittest\xunittest.h"

using namespace xcore;
extern x_iallocator* gTestAllocator;

class ns_test_allocator : public xcore::xp2p::ns_allocator
{
public:
	virtual void*	ns_allocate(u32 _size, u32 _alignment)
	{
		return gTestAllocator->allocate(_size, _alignment);
	}

	virtual void	ns_deallocate(void* _old)
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
		}

		UNITTEST_TEST(enqueue)
		{
		}

		UNITTEST_TEST(dequeue)
		{
		}
	}
}
UNITTEST_SUITE_END
