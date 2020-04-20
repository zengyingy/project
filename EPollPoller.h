#ifndef NET_POLLER_EPOLLPOLLER_H
#define NET_POLLER_EPOLLPOLLER_H

#include <map>
#include <vector>

#include "Timestamp.h"
#include "EventLoop.h"

struct epoll_event;

namespace net
{
	class Channel;
	///
	/// IO Multiplexing with epoll(4).
	///
	class EPollPoller : boost::noncopyable
	{
	public:
		typedef std::vector<Channel*> ChannelList;

	public:
		EPollPoller(EventLoop* loop);
		~EPollPoller();

		Timestamp poll(int timeoutMs, ChannelList* activeChannels);
		void updateChannel(Channel* channel);
		void removeChannel(Channel* channel);

		void assertInLoopThread()
		{
			ownerLoop_->assertInLoopThread();
		}

	private:
		static const int kInitEventListSize = 16;

		void fillActiveChannels(int numEvents,
			ChannelList* activeChannels) const;
		void update(int operation, Channel* channel);

		typedef std::vector<struct epoll_event> EventList;
		typedef std::map<int, Channel*> ChannelMap;

		int epollfd_;
		EventList events_;
		ChannelMap channels_;

	private:
		EventLoop * ownerLoop_;	// PollerÀ˘ ÙEventLoop
	};

}

#endif  // NET_POLLER_EPOLLPOLLER_H