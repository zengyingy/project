#ifndef BASE_THREADPOOL_H
#define BASE_THREADPOOL_H

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"


#include <boost/noncopyable.hpp>

#include <deque>
#include <functional>
#include <vector>
#include <string>
#include <memory>

namespace net
{

	class ThreadPool : boost::noncopyable
	{
	public:
		typedef std::function<void()> Task;

		explicit ThreadPool(const std::string& name = std::string());
		~ThreadPool();

		void start(int numThreads);
		void stop();

		void run(const Task& f);

	private:
		void runInThread();
		Task take();

		MutexLock mutex_;
		Condition cond_;
		std::string name_;
		std::vector<std::shared_ptr<Thread>> threads_;
		std::deque<Task> queue_;
		bool running_;
	};

}

#endif
