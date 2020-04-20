#include "ThreadPool.h"

#include <functional>

#include <assert.h>
#include <stdio.h>

using namespace net;

ThreadPool::ThreadPool(const std::string& name)
	: mutex_(),
	cond_(mutex_),
	name_(name),
	running_(false)
{
}

ThreadPool::~ThreadPool()
{
	if (running_)
	{
		stop();
	}
}

void ThreadPool::start(int numThreads)
{
	assert(threads_.empty());
	running_ = true;
	threads_.reserve(numThreads);
	for (int i = 0; i < numThreads; ++i)
	{
		char id[32];
		snprintf(id, sizeof id, "%d", i);
		threads_.push_back(std::shared_ptr<Thread>(new Thread(
			std::bind(&ThreadPool::runInThread, this), name_ + id)));
		threads_[i]->start();
	}
}

void ThreadPool::stop()
{
	{
		MutexLockGuard lock(mutex_);
		running_ = false;
		cond_.notifyAll();
	}
	for (auto t : threads_)
	{
		t->join();
	}
}

void ThreadPool::run(const Task& task)
{
	if (threads_.empty())
	{
		task();
	}
	else
	{
		MutexLockGuard lock(mutex_);
		queue_.push_back(task);
		cond_.notify();
	}
}

ThreadPool::Task ThreadPool::take()
{
	MutexLockGuard lock(mutex_);
	// always use a while-loop, due to spurious wakeup
	while (queue_.empty() && running_)
	{
		cond_.wait();
	}
	Task task;
	if (!queue_.empty())
	{
		task = queue_.front();
		queue_.pop_front();
	}
	return task;
}

void ThreadPool::runInThread()
{

	while (running_)
	{
		Task task(take());
		if (task)
		{
			task();
		}
	}
	
}

