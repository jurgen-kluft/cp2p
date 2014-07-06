//==============================================================================
//  x_queue.h
//==============================================================================
#ifndef __XPEER_2_PEER_QUEUE_H__
#define __XPEER_2_PEER_QUEUE_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace xcore
{
	namespace xp2p
	{
		/// Multi-threaded queue
		template <typename T>
		class Queue
		{
		public:

			T pop()
			{
				std::unique_lock<std::mutex> mlock(mutex_);
				while (queue_.empty())
				{
					cond_.wait(mlock);
				}
				auto item = queue_.front();
				queue_.pop();
				return item;
			}

			void pop(T& item)
			{
				std::unique_lock<std::mutex> mlock(mutex_);
				while (queue_.empty())
				{
					cond_.wait(mlock);
				}
				item = queue_.front();
				queue_.pop();
			}

			void push(const T& item)
			{
				std::unique_lock<std::mutex> mlock(mutex_);
				queue_.push(item);
				mlock.unlock();
				cond_.notify_one();
			}

			void push(T&& item)
			{
				std::unique_lock<std::mutex> mlock(mutex_);
				queue_.push(std::move(item));
				mlock.unlock();
				cond_.notify_one();
			}

		private:
			std::queue<T>			queue_;
			std::mutex				mutex_;
			std::condition_variable	cond_;
		};

	}
}

#endif