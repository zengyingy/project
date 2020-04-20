#ifndef NET_TIMER_H
#define NET_TIMER_H

#include <atomic>

#include <boost/noncopyable.hpp>

#include "Timestamp.h"
#include "Callbacks.h"

namespace net
{
	///
	/// Internal class for timer event.
	///
	using namespace net;

	class Timer : boost::noncopyable
	{
	public:
		Timer(const TimerCallback& cb, Timestamp when, double interval)
			: callback_(cb),
			expiration_(when),
			interval_(interval),
			repeat_(interval > 0.0),
			sequence_(++s_numCreated_)
		{ }

		void run() const
		{
			callback_();
		}

		Timestamp expiration() const { return expiration_; }
		bool repeat() const { return repeat_; }
		int64_t sequence() const { return sequence_; }

		void restart(Timestamp now);

		static int64_t numCreated() { return s_numCreated_; }

	private:
		const TimerCallback callback_;		// ��ʱ���ص�����
		Timestamp expiration_;				// ��һ�εĳ�ʱʱ��
		const double interval_;				// ��ʱʱ�����������һ���Զ�ʱ������ֵΪ0
		const bool repeat_;					// �Ƿ��ظ�
		const int64_t sequence_;				// ��ʱ�����

		static std::atomic_int64_t s_numCreated_;		// ��ʱ����������ǰ�Ѿ������Ķ�ʱ������
	};
}
#endif  // NET_TIMER_H
