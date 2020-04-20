#include "EventLoopThreadPool.h"

#include "EventLoop.h"
#include "EventLoopThread.h"

#include <functional>

using namespace net;


EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop)
	: baseLoop_(baseLoop),
	started_(false),
	numThreads_(0),
	next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
	// Don't delete loop, it's stack variable
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
	assert(!started_);
	baseLoop_->assertInLoopThread();

	started_ = true;

	for (int i = 0; i < numThreads_; ++i)
	{
		std::shared_ptr<EventLoopThread> t(new EventLoopThread(cb));
		threads_.push_back(t);
		loops_.push_back(t->startLoop());	// ����EventLoopThread�̣߳��ڽ����¼�ѭ��֮ǰ�������cb
	}
	if (numThreads_ == 0 && cb)
	{
		// ֻ��һ��EventLoop�������EventLoop�����¼�ѭ��֮ǰ������cb
		cb(baseLoop_);
	}
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
	baseLoop_->assertInLoopThread();
	EventLoop* loop = baseLoop_;

	// ���loops_Ϊ�գ���loopָ��baseLoop_
	// �����Ϊ�գ�����round-robin��RR���ֽУ��ĵ��ȷ�ʽѡ��һ��EventLoop
	if (!loops_.empty())
	{
		// round-robin
		loop = loops_[next_];
		++next_;
		if (static_cast<size_t>(next_) >= loops_.size())
		{
			next_ = 0;
		}
	}
	return loop;
}

