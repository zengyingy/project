#include "Condition.h"

#include <errno.h>

// returns true if time out, false otherwise.
bool net::Condition::waitForSeconds(int seconds)
{
	struct timespec abstime;
	clock_gettime(CLOCK_REALTIME, &abstime);
	abstime.tv_sec += seconds;
	return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
}

