//==============================================================================
//  x_udx-packetlist.h
//==============================================================================
#ifndef __XP2P_UDX_PACKET_LIST_H__
#define __XP2P_UDX_PACKET_LIST_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace xcore
{
	struct udx_list_node
	{
		inline			udx_list_node() : m_item(NULL), m_next(NULL), m_prev(NULL) {}

		void*			m_item;

		udx_list_node*	m_next;
		udx_list_node*	m_prev;

		void			push_back(udx_list_node* node)
		{
			node->m_prev = this;
			node->m_next = m_next;
			m_next = node;
		}

		void			push_front(udx_list_node* node)
		{
			node->m_next = this;
			node->m_prev = m_prev;
			m_prev->m_next = node;
			m_prev = node;
		}

		udx_list_node*	head()
		{
			udx_list_node* node = m_prev;
			return node;
		}

		udx_list_node*	pop_front()
		{
			udx_list_node* node = m_prev;
			m_prev = node->m_prev;
			m_prev->m_next = this;
			node->m_next = NULL;
			node->m_prev = NULL;
			return node;
		}
	};

	struct udx_list
	{
		u32				m_count;
		udx_list_node	m_root;

		void			init()
		{
			m_root.m_next = &m_root;
			m_root.m_prev = &m_root;
			m_count = 0;
		}

		void			push(udx_list_node* node)
		{
			m_root.push_back(node);
			m_count += 1;
		}

		void			push_front(udx_list_node* node)
		{
			m_root.push_front(node);
			m_count += 1;
		}

		bool			head(udx_list_node*& node)
		{
			if (m_count == 0)
			{
				node = NULL;
				return false;
			}
			node = m_root.head();
			return true;
		}

		bool			pop(udx_list_node*& node)
		{
			if (m_count == 0)
			{
				node = NULL;
				return false;
			}
			node = m_root.pop_front();
			m_count -= 1;
			return true;
		}

	};

}

#endif	/// __XP2P_UDX_PACKET_LIST_H__