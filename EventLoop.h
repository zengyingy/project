#ifndef NET_EVENTLOOP_H
#define NET_EVENTLOOP_H

#include <vector>
#include <functional>
#include <memory>

#include <boost/noncopyable.hpp>

#include "Mutex.h"
#include "Thread.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include "TimerId.h"

namespace net
{

	class Channel;
	class EPollPoller;
	class TimerQueue;
	///
	/// Reactor, at most one per thread.
	///
	/// This is an interface class, so don't expose too much details.
	class EventLoop : boost::noncopyable
	{
	public:
		typedef std::function<void()> Functor;

		EventLoop();
		~EventLoop();  // force out-line dtor, for scoped_ptr members.

					   ///
					   /// Loops forever.
					   ///
					   /// Must be called in the same thread as creation of the object.
					   ///
		void loop();

		void quit();

		///
		/// Time when poll returns, usually means data arrivial.
		///
		Timestamp pollReturnTime() const { return pollReturnTime_; }

		/// Runs callback immediately in the loop thread.
		/// It wakes up the loop, and run the cb.
		/// If in the same loop thread, cb is run within the function.
		/// Safe to call from other threads.
		void runInLoop(const Functor& cb);
		/// Queues callback in the loop thread.
		/// Runs after finish pooling.
		/// Safe to call from other threads.
		void queueInLoop(const Functor& cb);

		// timers

		///
		/// Runs callback at 'time'.
		/// Safe to call from other threads.
		///
		TimerId runAt(const Timestamp& time, const TimerCallback& cb);
		///
		/// Runs callback after @c delay seconds.
		/// Safe to call from other threads.
		///
		TimerId runAfter(double delay, const TimerCallback& cb);
		///
		/// Runs callback every @c interval seconds.
		/// Safe to call from other threads.
		///
		TimerId runEvery(double interval, const TimerCallback& cb);
		///
		/// Cancels the timer.
		/// Safe to call from other threads.
		///
		void cancel(TimerId timerId);

		// internal usage
		void wakeup();
		void updateChannel(Channel* channel);		// ��Poller����ӻ��߸���ͨ��
		void removeChannel(Channel* channel);		// ��Poller���Ƴ�ͨ��

		void assertInLoopThread()
		{
			if (!isInLoopThread())
			{
				abortNotInLoopThread();
			}
		}
		bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

		bool eventHandling() const { return eventHandling_; }

		static EventLoop* getEventLoopOfCurrentThread();

	private:
		void abortNotInLoopThread();
		void handleRead();  // waked up
		void doPendingFunctors();

		void printActiveChannels() const; // DEBUG

		typedef std::vector<Channel*> ChannelList;

		bool looping_; /* atomic */
		bool quit_; /* atomic */
		bool eventHandling_; /* atomic */
		bool callingPendingFunctors_; /* atomic */
		const pid_t threadId_;		// ��ǰ���������߳�ID
		Timestamp pollReturnTime_;
		boost::scoped_ptr<EPollPoller> EPollPoller_;
		boost::scoped_ptr<TimerQueue> timerQueue_;
		int wakeupFd_;				// ����eventfd
									// unlike in TimerQueue, which is an internal class,
									// we don't expose Channel to client.
		boost::scoped_ptr<Channel> wakeupChannel_;	// ��ͨ����������poller_������
		ChannelList activeChannels_;		// Poller���صĻͨ��
		Channel* currentActiveChannel_;	// ��ǰ���ڴ���Ļͨ��
		MutexLock mutex_;
		std::vector<Functor> pendingFunctors_; // @BuardedBy mutex_
	};

}
#endif  // NET_EVENTLOOP_H
