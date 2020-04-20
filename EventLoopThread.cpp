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
	loop_->quit();		// �˳�IO�̣߳���IO�̵߳�loopѭ���˳����Ӷ��˳���IO�߳�
	thread_.join();
}

EventLoop* EventLoopThread::startLoop()
{
	assert(!thread_.started());
	thread_.start();

	//���̵߳ȴ��µ��߳�ִ��threadFunc�������ú�����Ϊ�������ݸ���Thread���󣬸ú�������EventLoop���󣬴����ɹ���notify����loop��
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
		// loop_ָ��ָ����һ��ջ�ϵĶ���threadFunc�����˳�֮�����ָ���ʧЧ��
		// threadFunc�����˳�������ζ���߳��˳��ˣ�EventLoopThread����Ҳ��û�д��ڵļ�ֵ�ˡ�
		// ���������ʲô�������
		loop_ = &loop;
		cond_.notify();
	}

	loop.loop();
	//assert(exiting_);
}

