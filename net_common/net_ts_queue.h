#pragma once
#include <deque>
#include <shared_mutex>

namespace net
{
	template<typename T>
	class tsqueue
	{
	public:
		tsqueue() = default;

		tsqueue(const tsqueue<T>&) = delete;

		virtual ~tsqueue()
		{
			clear();
		}

		const T& front() 
		{
			std::shared_lock<std::shared_mutex> lock(mutex);
			return queue.front();
		}

		const T& back() 
		{
			std::shared_lock<std::shared_mutex> lock(mutex);
			return queue.back();
		}

		void push_front(const T& item)
		{
			std::unique_lock<std::shared_mutex> lock(mutex);
			queue.push_front(item);
		}

		void push_back(const T& item)
		{
			std::unique_lock<std::shared_mutex> lock(mutex);
			queue.push_back(item);
		}

		void clear()
		{
			std::unique_lock<std::shared_mutex> lock(mutex);
			queue.clear();
		}

		size_t size()
		{
			std::shared_lock<std::shared_mutex> lock(mutex);
			return queue.size();
		}

		bool empty()
		{
			std::shared_lock<std::shared_mutex> lock(mutex);
			return queue.empty();
		}

		T popFront()
		{
			std::unique_lock<std::shared_mutex> lock(mutex);
			auto t = queue.front();
			queue.pop_front();
			return t;
		}

		T popBack()
		{
			std::unique_lock<std::shared_mutex> lock(mutex);
			auto t = queue.back();
			queue.pop_back();
			return t;
		}

	private:
		std::shared_mutex mutex;
		std::deque<T> queue;
	};
}
