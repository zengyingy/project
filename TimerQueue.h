#ifndef NET_TIMERQUEUE_H
#define NET_TIMERQUEUE_H

#include <set>
#include <vector>
#include <queue>
#include <memory>

#include <boost/noncopyable.hpp>

#include "Mutex.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include "Channel.h"

namespace net
{

	class EventLoop;
	class Timer;
	class TimerId;

	///
	/// A best efforts timer queue.
	/// No guarantee that the callback will be on time.
	///
	class TimerQueue : boost::noncopyable
	{
	public:

		TimerQueue(EventLoop* loop);
		~TimerQueue();

		///
		/// Schedules the callback to be run at given time,
		/// repeats if @c interval > 0.0.
		///
		/// Must be thread safe. Usually be called from other threads.
		// һ�����̰߳�ȫ�ģ����Կ��̵߳��á�ͨ������±������̵߳��á�
		TimerId addTimer(const TimerCallback& cb,
			Timestamp when,
			double interval);

		void cancel(TimerId timerId);

	private:

		// FIXME: use unique_ptr<Timer> instead of raw pointers.
		// unique_ptr��C++ 11��׼��һ����������Ȩ������ָ��
		// �޷��õ�ָ��ͬһ���������unique_ptrָ��
		// �����Խ����ƶ��������ƶ���ֵ������������Ȩ�����ƶ�����һ�����󣨶��ǿ������죩
		typedef std::pair<Timestamp, Timer*> Entry;
		//typedef std::set<Entry> TimerList;

		struct TimerCmp
		{
			bool operator()(Entry &a, Entry &b) const
			{
				return a.second->expiration() > b.second->expiration();
			}
		};

		typedef std::priority_queue<Entry,std::deque<Entry>,TimerCmp> TimerList;
		typedef std::pair<Timer*, int64_t> ActiveTimer;
		//typedef std::set<ActiveTimer> ActiveTimerSet;
		typedef std::priority_queue<ActiveTimer> ActiveTimerqueue;

		// ���³�Ա����ֻ��������������I/O�߳��е��ã�������ؼ�����
		// ����������ɱ��֮һ��������������Ҫ������������
		void addTimerInLoop(Timer* timer);
		void cancelInLoop(TimerId timerId);
		// called when timerfd alarms
		void handleRead();
		// move out all expired timers
		// ���س�ʱ�Ķ�ʱ���б�
		std::vector<Entry> getExpired(Timestamp now);
		void reset(const std::vector<Entry>& expired, Timestamp now);

		bool insert(Timer* timer);

		EventLoop* loop_;		// ����EventLoop
		const int timerfd_;
		Channel timerfdChannel_;
		// Timer list sorted by expiration
		TimerList timers_;	// timers_�ǰ�����ʱ������

							// for cancel()
							// timers_��activeTimers_���������ͬ������
							// timers_�ǰ�����ʱ������activeTimers_�ǰ������ַ����
		//ActiveTimerSet activeTimers_;
		//ActiveTimerqueue activeTimers_;
		//ActiveTimerqueue cancelingTimers_;
		bool callingExpiredTimers_; /* atomic */
		//ActiveTimerSet cancelingTimers_;	// ������Ǳ�ȡ���Ķ�ʱ��
	};

}
#endif  // NET_TIMERQUEUE_H
