#include "Logging.h"
#include "Channel.h"
#include "EventLoop.h"

#include <sstream>

#include "EPollPoller.h"

using namespace net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI | EPOLLET;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd__)
	: loop_(loop),
	fd_(fd__),
	events_(0),
	revents_(0),
	index_(-1),
	logHup_(true),
	tied_(false),
	eventHandling_(false)
{
}

Channel::~Channel()
{
	assert(!eventHandling_);
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
	tie_ = obj;
	tied_ = true;
}

void Channel::update()
{
	loop_->updateChannel(this);
}

// 调用这个函数之前确保调用disableAll
void Channel::remove()
{
	assert(isNoneEvent());
	loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
	std::shared_ptr<void> guard;
	if (tied_)
	{
		guard = tie_.lock();
		if (guard)
		{
			LOG_TRACE << "[6] usecount=" << guard.use_count();
			handleEventWithGuard(receiveTime);
			LOG_TRACE << "[12] usecount=" << guard.use_count();
		}
	}
	else
	{
		handleEventWithGuard(receiveTime);
	}
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
	eventHandling_ = true;
	/*
	if (revents_ & POLLHUP)
	{
	LOG_TRACE << "1111111111111111";
	}
	if (revents_ & POLLIN)
	{
	LOG_TRACE << "2222222222222222";
	}
	*/
	if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
	{
		if (logHup_)
		{
			LOG_WARN << "Channel::handle_event() POLLHUP";
		}
		if (closeCallback_) closeCallback_();
	}

	if (revents_ & EPOLLNVAL)
	{
		LOG_WARN << "Channel::handle_event() POLLNVAL";
	}

	if (revents_ & (EPOLLERR | EPOLLNVAL))
	{
		if (errorCallback_) errorCallback_();
	}
	if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
	{
		if (readCallback_) readCallback_(receiveTime);
	}
	if (revents_ & EPOLLOUT)
	{
		if (writeCallback_) writeCallback_();
	}
	eventHandling_ = false;
}

std::string Channel::reventsToString() const
{
	std::ostringstream oss;
	oss << fd_ << ": ";
	if (revents_ & EPOLLIN)
		oss << "IN ";
	if (revents_ & EPOLLPRI)
		oss << "PRI ";
	if (revents_ & EPOLLOUT)
		oss << "OUT ";
	if (revents_ & EPOLLHUP)
		oss << "HUP ";
	if (revents_ & EPOLLRDHUP)
		oss << "RDHUP ";
	if (revents_ & EPOLLERR)
		oss << "ERR ";
	if (revents_ & EPOLLNVAL)
		oss << "NVAL ";

	return oss.str().c_str();
}
