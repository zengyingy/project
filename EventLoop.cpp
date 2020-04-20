#include "EventLoop.h"

#include "Logging.h"
#include "Channel.h"
#include "EPollPoller.h"
#include "TimerQueue.h"

#include <functional>

#include <signal.h>
#include <sys/eventfd.h>

using namespace net;

namespace
{
	// ��ǰ�߳�EventLoop����ָ��
	// �ֲ߳̾��洢
	__thread EventLoop* t_loopInThisThread = 0;

	const int kPollTimeMs = 10000;

	int createEventfd()
	{
		int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
		if (evtfd < 0)
		{
			LOG_SYSERR << "Failed in eventfd";
			abort();
		}
		return evtfd;
	}

#pragma GCC diagnostic ignored "-Wold-style-cast"
	class IgnoreSigPipe
	{
	public:
		IgnoreSigPipe()
		{
			::signal(SIGPIPE, SIG_IGN);
			LOG_TRACE << "Ignore SIGPIPE";
		}
	};
#pragma GCC diagnostic error "-Wold-style-cast"

	IgnoreSigPipe initObj;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}

EventLoop::EventLoop()
	: looping_(false),
	quit_(false),
	eventHandling_(false),
	callingPendingFunctors_(false),
	threadId_(CurrentThread::tid()),
	EPollPoller_(EPollPoller(this)),
	timerQueue_(new TimerQueue(this)),
	wakeupFd_(createEventfd()),
	wakeupChannel_(new Channel(this, wakeupFd_)),
	currentActiveChannel_(NULL)
{
	LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
	// �����ǰ�߳��Ѿ�������EventLoop������ֹ(LOG_FATAL)
	if (t_loopInThisThread)
	{
		LOG_FATAL << "Another EventLoop " << t_loopInThisThread
			<< " exists in this thread " << threadId_;
	}
	else
	{
		t_loopInThisThread = this;
	}
	wakeupChannel_->setReadCallback(
		std::bind(&EventLoop::handleRead, this));
	// we are always reading the wakeupfd
	wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
	::close(wakeupFd_);
	t_loopInThisThread = NULL;
}

// �¼�ѭ�����ú������ܿ��̵߳���
// ֻ���ڴ����ö�����߳��е���
void EventLoop::loop()
{
	assert(!looping_);
	// ���Ե�ǰ���ڴ����ö�����߳���
	assertInLoopThread();
	looping_ = true;
	quit_ = false;
	LOG_TRACE << "EventLoop " << this << " start looping";

	//::poll(NULL, 0, 5*1000);
	while (!quit_)
	{
		activeChannels_.clear();
		pollReturnTime_ = EPollPoller_->poll(kPollTimeMs, &activeChannels_);
		//++iteration_;
		if (Logger::logLevel() <= Logger::TRACE)
		{
			printActiveChannels();
		}
		// TODO sort channel by priority
		eventHandling_ = true;
		for (ChannelList::iterator it = activeChannels_.begin();
			it != activeChannels_.end(); ++it)
		{
			currentActiveChannel_ = *it;
			currentActiveChannel_->handleEvent(pollReturnTime_);
		}
		currentActiveChannel_ = NULL;
		eventHandling_ = false;
		doPendingFunctors();
	}

	LOG_TRACE << "EventLoop " << this << " stop looping";
	looping_ = false;
}

// �ú������Կ��̵߳���
void EventLoop::quit()
{
	quit_ = true;
	if (!isInLoopThread())
	{
		wakeup();
	}
}

// ��I/O�߳���ִ��ĳ���ص��������ú������Կ��̵߳���
void EventLoop::runInLoop(const Functor& cb)
{
	if (isInLoopThread())
	{
		// ����ǵ�ǰIO�̵߳���runInLoop����ͬ������cb
		cb();
	}
	else
	{
		// ����������̵߳���runInLoop�����첽�ؽ�cb��ӵ�����
		queueInLoop(cb);
	}
}

void EventLoop::queueInLoop(const Functor& cb)
{
	{
		MutexLockGuard lock(mutex_);
		pendingFunctors_.push_back(cb);
	}

	// ����queueInLoop���̲߳���IO�߳���Ҫ����
	// ���ߵ���queueInLoop���߳���IO�̣߳����Ҵ�ʱ���ڵ���pending functor����Ҫ����
	// ֻ��IO�̵߳��¼��ص��е���queueInLoop�Ų���Ҫ����
	if (!isInLoopThread() || callingPendingFunctors_)
	{
		wakeup();
	}
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
	return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
	Timestamp time(addTime(Timestamp::now(), delay));
	return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
	Timestamp time(addTime(Timestamp::now(), interval));
	return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
	return timerQueue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel* channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	EPollPoller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	if (eventHandling_)
	{
		assert(currentActiveChannel_ == channel ||
			std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
	}
	EPollPoller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
	LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
		<< " was created in threadId_ = " << threadId_
		<< ", current thread id = " << CurrentThread::tid();
}

void EventLoop::wakeup()
{
	uint64_t one = 1;
	//ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
	ssize_t n = ::write(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
	}
}

void EventLoop::handleRead()
{
	uint64_t one = 1;
	//ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
	ssize_t n = ::read(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
	}
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFunctors_ = true;

	{
		MutexLockGuard lock(mutex_);
		functors.swap(pendingFunctors_);
	}

	for (size_t i = 0; i < functors.size(); ++i)
	{
		functors[i]();
	}
	callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const
{
	for (ChannelList::const_iterator it = activeChannels_.begin();
		it != activeChannels_.end(); ++it)
	{
		const Channel* ch = *it;
		LOG_TRACE << "{" << ch->reventsToString() << "} ";
	}
}
