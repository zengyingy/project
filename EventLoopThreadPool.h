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

		EventLoop * baseLoop_;	// ��Acceptor����EventLoop��ͬ
		bool started_;
		int numThreads_;		// �߳���
		int next_;			// �����ӵ�������ѡ���EventLoop�����±�
		std::vector<std::shared_ptr<EventLoopThread>> threads_;		// IO�߳��б�
		std::vector<EventLoop*> loops_;					// EventLoop�б�
	};

}

#endif  // NET_EVENTLOOPTHREADPOOL_H
