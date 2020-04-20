#ifndef NET_EVENTLOOPTHREAD_H
#define NET_EVENTLOOPTHREAD_H

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"

#include <boost/noncopyable.hpp>


namespace net
{

	class EventLoop;

	class EventLoopThread : boost::noncopyable
	{
	public:
		typedef std::function<void(EventLoop*)> ThreadInitCallback;

		EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
		~EventLoopThread();
		EventLoop* startLoop();	// 启动线程，该线程就成为了IO线程

	private:
		void threadFunc();		// 线程函数

		EventLoop* loop_;			// loop_指针指向一个EventLoop对象
		bool exiting_;
		Thread thread_;
		MutexLock mutex_;
		Condition cond_;
		ThreadInitCallback callback_;		// 回调函数在EventLoop::loop事件循环之前被调用
	};

}

#endif  // NET_EVENTLOOPTHREAD_H

