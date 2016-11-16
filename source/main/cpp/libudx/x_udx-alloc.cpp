#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "xp2p\x_sha1.h"
#include "xp2p\libudx\x_udx.h"
#include "xp2p\libudx\x_udx-alloc.h"
#include "xp2p\private\x_sockets.h"

#include <chrono>

namespace xcore
{
	class udx_alloc_sys : public udx_alloc
	{
		x_iallocator*	m_allocator;

	public:
		void			init()
		{
			m_allocator = gCreateSystemAllocator();
		}

		void*			alloc(u32 _size)
		{
			return m_allocator->allocate(_size, 8);
		}

		void			commit(void* _ptr, u32 _size)
		{
			
		}

		void			dealloc(void* _ptr)
		{
			return m_allocator->deallocate(_ptr);
		}
	};


	class udx_allocator_for_messages : public udx_alloc
	{
		struct alloc_node
		{
			alloc_node*	m_prev;
			u32			m_size;
			u32			m_refc;
			alloc_node*	m_next;
		};

		u8					*m_base_begin, *m_base_end;
		u8					*m_head;

		u32					m_header_size;

	public:
		void				init(u64 memory_size, u32 header_size)
		{
			m_base_begin = (u8*)_aligned_malloc(memory_size, 8);
			m_base_end = m_base_begin + memory_size;
			m_head = m_base_begin;
			m_header_size = header_size;

			alloc_node* node = (alloc_node*)m_head;
			node->m_size = 0;
			node->m_refc = 0;
			node->m_prev = NULL;
			node->m_next = NULL;
		}

		virtual void*		alloc(u32 _size)
		{
			// We do not allow to allocate 0 size memory
			if (_size == 0)
				return NULL;

			alloc_node* node = (alloc_node*)m_head;
			if (node->m_size != 0)
			{
				commit(_size);
			}

			node = (alloc_node*)m_head;
			node->m_refc = 1;
			node->m_size = _size;
			u8* msg = (m_head + m_header_size);
			return msg;
		}

		virtual void		commit(u32 _size)
		{
			alloc_node* node = (alloc_node*)m_head;
			if (node->m_size!= 0)
			{
				node->m_size = m_header_size + ((_size + 3) & 0xfffffff8);

				m_head += node->m_size;

				alloc_node* head = (alloc_node*)m_head;
				head->m_size = 0;
				head->m_refc = 0;
				head->m_prev = node;
				head->m_next = NULL;

				node->m_next = head;
			}
		}

		virtual void		dealloc(void* p)
		{
			alloc_node* n = (alloc_node*)((u8*)p - m_header_size);
			if (--n->m_refc == 0)
			{
				// Zero size nodes are 'free' nodes
				n->m_size = 0;

				if (n->m_prev != NULL && n->m_prev->m_size == 0)
				{	// Merge with previous
					n->m_prev->m_next = n->m_next;
					n = n->m_prev;
				}

				if (n->m_next != NULL && n->m_next->m_size == 0)
				{	// Merge with next
					n->m_next = n->m_next->m_next;
				}

			}
		}
	};


}
