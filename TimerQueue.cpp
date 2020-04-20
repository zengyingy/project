#define __STDC_LIMIT_MACROS
#include "TimerQueue.h"

#include <functional>

#include "Logging.h"
#include "EventLoop.h"
#include "Timer.h"
#include "TimerId.h"

#include <functional>

#include <sys/timerfd.h>

namespace net
{
	namespace detail
	{

		// ������ʱ��
		int createTimerfd()
		{
			int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
				TFD_NONBLOCK | TFD_CLOEXEC);
			if (timerfd < 0)
			{
				LOG_SYSFATAL << "Failed in timerfd_create";
			}
			return timerfd;
		}

		// ���㳬ʱʱ���뵱ǰʱ���ʱ���
		struct timespec howMuchTimeFromNow(Timestamp when)
		{
			int64_t microseconds = when.microSecondsSinceEpoch()
				- Timestamp::now().microSecondsSinceEpoch();
			if (microseconds < 100)
			{
				microseconds = 100;
			}
			struct timespec ts;
			ts.tv_sec = static_cast<time_t>(
				microseconds / Timestamp::kMicroSecondsPerSecond);
			ts.tv_nsec = static_cast<long>(
				(microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
			return ts;
		}

		// �����ʱ��������һֱ����
		void readTimerfd(int timerfd, Timestamp now)
		{
			uint64_t howmany;
			ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
			LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
			if (n != sizeof howmany)
			{
				LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
			}
		}

		// ���ö�ʱ���ĳ�ʱʱ��
		void resetTimerfd(int timerfd, Timestamp expiration)
		{
			// wake up loop by timerfd_settime()
			struct itimerspec newValue;
			struct itimerspec oldValue;
			bzero(&newValue, sizeof newValue);
			bzero(&oldValue, sizeof oldValue);
			newValue.it_value = howMuchTimeFromNow(expiration);
			int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
			if (ret)
			{
				LOG_SYSERR << "timerfd_settime()";
			}
		}

	}
}

using namespace net;
using namespace net::detail;

TimerQueue::TimerQueue(EventLoop* loop)
	: loop_(loop),
	timerfd_(createTimerfd()),
	timerfdChannel_(loop, timerfd_),
	timers_(),
	callingExpiredTimers_(false)
{
	timerfdChannel_.setReadCallback(
		std::bind(&TimerQueue::handleRead, this));
	// we are always reading the timerfd, we disarm it with timerfd_settime.
	timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
	::close(timerfd_);
	while (!timers_.empty())
	{
		Entry it = timers_.top();
		timers_.pop();
		delete it.second;
	}
	// do not remove channel, since we're in EventLoop::dtor();
/*
	for (TimerList::iterator it = timers_.begin();
		it != timers_.end(); ++it)
	{
		delete it->second;
	}
*/
}

TimerId TimerQueue::addTimer(const TimerCallback& cb,
	Timestamp when,
	double interval)
{
	Timer* timer = new Timer(cb, when, interval);

	loop_->runInLoop(
		std::bind(&TimerQueue::addTimerInLoop, this, timer));

	//addTimerInLoop(timer);
	return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
	loop_->runInLoop(
		std::bind(&TimerQueue::cancelInLoop, this, timerId));
	//cancelInLoop(timerId);
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
	loop_->assertInLoopThread();
	// ����һ����ʱ�����п��ܻ�ʹ�����絽�ڵĶ�ʱ�������ı�
	bool earliestChanged = insert(timer);

	if (earliestChanged)
	{
		// ���ö�ʱ���ĳ�ʱʱ��(timerfd_settime)
		resetTimerfd(timerfd_, timer->expiration());
	}
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
	/*
	loop_->assertInLoopThread();
	assert(timers_.size() == activeTimers_.size());
	ActiveTimer timer(timerId.timer_, timerId.sequence_);
	// ���Ҹö�ʱ��
	ActiveTimerSet::iterator it = activeTimers_.find(timer);
	if (it != activeTimers_.end())
	{
		size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
		assert(n == 1); (void)n;
		delete it->first; // FIXME: no delete please,�������unique_ptr,����Ͳ���Ҫ�ֶ�ɾ����
		activeTimers_.erase(it);
	}
	else if (callingExpiredTimers_)
	{
		// �Ѿ����ڣ��������ڵ��ûص������Ķ�ʱ��
		cancelingTimers_.insert(timer);
	}
	assert(timers_.size() == activeTimers_.size());
	*/
}

void TimerQueue::handleRead()
{
	loop_->assertInLoopThread();
	Timestamp now(Timestamp::now());
	readTimerfd(timerfd_, now);		// ������¼�������һֱ����

									// ��ȡ��ʱ��֮ǰ���еĶ�ʱ���б�(����ʱ��ʱ���б�)
	std::vector<Entry> expired = getExpired(now);
	
	//callingExpiredTimers_ = true;
	//cancelingTimers_.clear();
	// safe to callback outside critical section
	for (std::vector<Entry>::iterator it = expired.begin();
		it != expired.end(); ++it)
	{
		// ����ص���ʱ��������
		it->second->run();
	}
	callingExpiredTimers_ = false;

	// ����һ���Զ�ʱ������Ҫ����
	reset(expired, now);
}

// rvo
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
	std::vector<Entry> expired;
	Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
	while (!timers_.empty())
	{
		Entry temp = timers.top();
		if (temp.first < sentry.first)
		{
			expired.push_back(temp);
			timers.pop();
		}
		else
		{
			break;
		}
	}
	return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
	/*
	Timestamp nextExpire;

	for (std::vector<Entry>::const_iterator it = expired.begin();
		it != expired.end(); ++it)
	{
		ActiveTimer timer(it->second, it->second->sequence());
		// ������ظ��Ķ�ʱ��������δȡ����ʱ�����������ö�ʱ��
		if (it->second->repeat()
			&& cancelingTimers_.find(timer) == cancelingTimers_.end())
		{
			it->second->restart(now);
			insert(it->second);
		}
		else
		{
			// һ���Զ�ʱ�������ѱ�ȡ���Ķ�ʱ���ǲ������õģ����ɾ���ö�ʱ��
			// FIXME move to a free list
			delete it->second; // FIXME: no delete please
		}
	}

	if (!timers_.empty())
	{
		// ��ȡ���絽�ڵĶ�ʱ����ʱʱ��
		nextExpire = timers_.begin()->second->expiration();
	}

	if (nextExpire.valid())
	{
		// ���ö�ʱ���ĳ�ʱʱ��(timerfd_settime)
		resetTimerfd(timerfd_, nextExpire);
	}
	*/
}

bool TimerQueue::insert(Timer* timer)
{
	loop_->assertInLoopThread();
	// ���絽��ʱ���Ƿ�ı�
	bool earliestChanged = false;
	Timestamp when = timer->expiration();
	// ���timers_Ϊ�ջ���whenС��timers_�е����絽��ʱ��
	if (timers_.empty() || when < timers_.top().first)
	{
		earliestChanged = true;
	}
	{
		// ���뵽timers_��
		timers_.push(Entry(when, timer));
	}
	return earliestChanged;
}

