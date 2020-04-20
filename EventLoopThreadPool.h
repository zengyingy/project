#ifndef NET_EVENTLOOPTHREADPOOL_H
#define NET_EVENTLOOPTHREADPOOL_H

#include "Condition.h"
#include "Mutex.h"

#include <vector>
#include <memory>
#include <functional>
#include <boost/noncopyable.hpp>



namespace net
{

	class EventLoop;
	class EventLoopThread;

	class EventLoopThreadPool : boost::noncopyable
	{
	public:
		typedef std::function<void(EventLoop*)> ThreadInitCallback;

		EventLoopThreadPool(EventLoop* baseLoop);
		~EventLoopThreadPool();
		void setThreadNum(int numThreads) { numThreads_ = numThreads; }
		void start(const ThreadInitCallback& cb = ThreadInitCallback());
		EventLoop* getNextLoop();

	private:

		EventLoop * baseLoop_;	// 与Acceptor所属EventLoop相同
		bool started_;
		int numThreads_;		// 线程数
		int next_;			// 新连接到来，所选择的EventLoop对象下标
		std::vector<std::shared_ptr<EventLoopThread>> threads_;		// IO线程列表
		std::vector<EventLoop*> loops_;					// EventLoop列表
	};

}

#endif  // NET_EVENTLOOPTHREADPOOL_H
