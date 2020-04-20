#include "Thread.h"
#include "CurrentThread.h"

#include <exception>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace net
{
	namespace CurrentThread
	{
		// __thread修饰的变量是线程局部存储的。
		__thread int t_cachedTid = 0;		// 线程真实pid（tid）的缓存，
											// 是为了减少::syscall(SYS_gettid)系统调用的次数
											// 提高获取tid的效率
		__thread char t_tidString[32];	// 这是tid的字符串表示形式
		__thread const char* t_threadName = "unknown";
		const bool sameType = boost::is_same<int, pid_t>::value;
		BOOST_STATIC_ASSERT(sameType);
	}

	namespace detail
	{

		pid_t gettid()
		{
			return static_cast<pid_t>(::syscall(SYS_gettid));
		}

		void afterFork()
		{
			net::CurrentThread::t_cachedTid = 0;
			net::CurrentThread::t_threadName = "main";
			CurrentThread::tid();
			// no need to call pthread_atfork(NULL, NULL, &afterFork);
		}

		class ThreadNameInitializer
		{
		public:
			ThreadNameInitializer()
			{
				net::CurrentThread::t_threadName = "main";
				CurrentThread::tid();
				pthread_atfork(NULL, NULL, &afterFork);
			}
		};

		ThreadNameInitializer init;
	}
}

using namespace net;

void CurrentThread::cacheTid()
{
	if (t_cachedTid == 0)
	{
		t_cachedTid = detail::gettid();
		int n = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
		assert(n == 6); (void)n;
	}
}

bool CurrentThread::isMainThread()
{
	return tid() == ::getpid();
}

std::atomic_int64_t Thread::numCreated_;

Thread::Thread(const ThreadFunc& func, const string& n)
	: started_(false),
	pthreadId_(0),
	tid_(0),
	func_(func),
	name_(n)
{
	++numCreated_;
}

Thread::~Thread()
{
	// no join
}

void Thread::start()
{
	assert(!started_);
	started_ = true;
	errno = pthread_create(&pthreadId_, NULL, &startThread, this);
	if (errno != 0)
	{
		//LOG_SYSFATAL << "Failed in pthread_create";
	}
}

int Thread::join()
{
	assert(started_);
	return pthread_join(pthreadId_, NULL);
}

void* Thread::startThread(void* obj)
{
	Thread* thread = static_cast<Thread*>(obj);
	thread->runInThread();
	return NULL;
}

//除去了异常处理
void Thread::runInThread()
{
	tid_ = CurrentThread::tid();
	net::CurrentThread::t_threadName = name_.c_str();
	func_();
	net::CurrentThread::t_threadName = "finished";
}

