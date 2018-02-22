#include "xbase/x_target.h"

#include "xp2p/libudx/x_udx-filter.h"

namespace xcore
{

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] IMP
	class x_udx_filter_sma : public udx_filter
	{
	public:
		void			init(u32 wnd_size, udx_alloc* _allocator)
		{
			m_allocator = _allocator;

			m_window = (u64*)m_allocator->alloc(wnd_size * sizeof(u64));
			m_allocator->commit(m_window, wnd_size * sizeof(u64));

			m_average = 0;
			m_count = 0;
			m_size = wnd_size;
		}

		virtual u64		add(u64 value)
		{
			m_average -= m_window[m_index];
			m_window[m_index++] = value;
			m_average += value;
			m_index += 1;
			m_count += 1;
			if (m_count > m_size)
				m_count = m_size;

			return get();
		}

		virtual u64		get() const
		{
			return m_average / m_count;
		}

		virtual	void	release();

		XCORE_CLASS_PLACEMENT_NEW_DELETE

	protected:
		udx_alloc*		m_allocator;
		u64				m_average;
		u64				m_index;
		u64				m_count;

		u32				m_size;
		u64				*m_window;
	};

	void	x_udx_filter_sma::release()
	{
		m_allocator->dealloc(m_window);
		m_allocator->dealloc(this);
	}

	udx_filter*		gCreateSmaFilter(u32 wnd_size, udx_alloc* _allocator)
	{
		void * filter_class_mem = _allocator->alloc(sizeof(x_udx_filter_sma));
		x_udx_filter_sma* filter = new (filter_class_mem) x_udx_filter_sma();
		filter->init(wnd_size, _allocator);
		return filter;
	}

}
