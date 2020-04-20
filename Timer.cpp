#include "Timer.h"

using namespace net;

std::atomic_int64_t Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
	if (repeat_)
	{
		// ���¼�����һ����ʱʱ��
		expiration_ = addTime(now, interval_);
	}
	else
	{
		expiration_ = Timestamp::invalid();
	}
}
