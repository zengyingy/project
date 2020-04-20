#include "EventLoopThread.h"
#include "EventLoop.h"

#include <functional>

using namespace net;


EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
	: loop_(NULL),
	exiting_(false),
	thread_(std::bind(&EventLoopThread::threadFunc, this)),
	mutex_(),
	cond_(mutex_),
	callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
	exiting_ = true;
	loop_->quit();		// 退出IO线程，让IO线程的loop循环退出，从而退出了IO线程
	thread_.join();
}

EventLoop* EventLoopThread::startLoop()
{
	assert(!thread_.started());
	thread_.start();

	//主线程等待新的线程执行threadFunc函数，该函数作为参数传递给了Thread对象，该函数创建EventLoop对象，创建成功就notify，并loop。
	{
		MutexLockGuard lock(mutex_);
		while (loop_ == NULL)
		{
			cond_.wait();
		}
	}

	return loop_;
}

void EventLoopThread::threadFunc()
{
	EventLoop loop;

	if (callback_)
	{
		callback_(&loop);
	}

	{
		MutexLockGuard lock(mutex_);
		// loop_指针指向了一个栈上的对象，threadFunc函数退出之后，这个指针就失效了
		// threadFunc函数退出，就意味着线程退出了，EventLoopThread对象也就没有存在的价值了。
		// 因而不会有什么大的问题
		loop_ = &loop;
		cond_.notify();
	}

	loop.loop();
	//assert(exiting_);
}

