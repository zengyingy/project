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
		EventLoop* startLoop();	// �����̣߳����߳̾ͳ�Ϊ��IO�߳�

	private:
		void threadFunc();		// �̺߳���

		EventLoop* loop_;			// loop_ָ��ָ��һ��EventLoop����
		bool exiting_;
		Thread thread_;
		MutexLock mutex_;
		Condition cond_;
		ThreadInitCallback callback_;		// �ص�������EventLoop::loop�¼�ѭ��֮ǰ������
	};

}

#endif  // NET_EVENTLOOPTHREAD_H

