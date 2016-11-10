#include "xbase\x_target.h"

#include "xp2p\libudx\x_udx-filter.h"

namespace xcore
{

	// --------------------------------------------------------------------------------------------
	// [PRIVATE] IMP
	class x_udx_filter_sma : public udx_filter
	{
	public:
		virtual void	init(u64* window, u32 size)
		{
			m_window = window;
			m_average = 0;
			m_count = 0;
			m_size = size;
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

	protected:
		u64				m_average;
		u64				m_index;
		u64				m_count;

		u32				m_size;
		u64				*m_window;
	};

}
