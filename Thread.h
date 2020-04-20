#ifndef BASE_THREAD_H
#define BASE_THREAD_H

//使用C++ 11标准，原子操作、互斥锁等。
#include <functional>
#include <atomic>
#include <string>
#include <boost/noncopyable.hpp>
#include <pthread.h>

using std::string;

class Thread : boost::noncopyable
{
public:
	typedef std::function<void()> ThreadFunc;

	explicit Thread(const ThreadFunc&, const string& name = string());
	~Thread();

	void start();
	int join(); // return pthread_join()

	bool started() const { return started_; }
	// pthread_t pthreadId() const { return pthreadId_; }
	pid_t tid() const { return tid_; }
	const string& name() const { return name_; }

	static int numCreated() { return numCreated_; }

private:
	static void* startThread(void* thread);
	void runInThread();

	bool       started_;
	pthread_t  pthreadId_;
	pid_t      tid_;
	ThreadFunc func_;
	string     name_;

	static std::atomic_int64_t numCreated_;
};

#endif // BASE_THREAD_H
