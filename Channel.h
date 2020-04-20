#ifndef NET_CHANNEL_H
#define NET_CHANNEL_H

#include <functional>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>

#include "Timestamp.h"


namespace net
{

	class EventLoop;

	///
	/// A selectable I/O channel.
	///
	/// This class doesn't own the file descriptor.
	/// The file descriptor could be a socket,
	/// an eventfd, a timerfd, or a signalfd
	class Channel : boost::noncopyable
	{
	public:
		typedef std::function<void()> EventCallback;
		typedef std::function<void(Timestamp)> ReadEventCallback;

		Channel(EventLoop* loop, int fd);
		~Channel();

		void handleEvent(Timestamp receiveTime);
		void setReadCallback(const ReadEventCallback& cb)
		{
			readCallback_ = cb;
		}
		void setWriteCallback(const EventCallback& cb)
		{
			writeCallback_ = cb;
		}
		void setCloseCallback(const EventCallback& cb)
		{
			closeCallback_ = cb;
		}
		void setErrorCallback(const EventCallback& cb)
		{
			errorCallback_ = cb;
		}

		/// Tie this channel to the owner object managed by shared_ptr,
		/// prevent the owner object being destroyed in handleEvent.
		void tie(const std::shared_ptr<void>&);

		int fd() const { return fd_; }
		int events() const { return events_; }
		void set_revents(int revt) { revents_ = revt; } // used by pollers
														// int revents() const { return revents_; }
		bool isNoneEvent() const { return events_ == kNoneEvent; }

		void enableReading() { events_ |= kReadEvent; update(); }
		// void disableReading() { events_ &= ~kReadEvent; update(); }
		void enableWriting() { events_ |= kWriteEvent; update(); }
		void disableWriting() { events_ &= ~kWriteEvent; update(); }
		void disableAll() { events_ = kNoneEvent; update(); }
		bool isWriting() const { return events_ & kWriteEvent; }

		// for Poller
		int index() { return index_; }
		void set_index(int idx) { index_ = idx; }

		// for debug
		std::string reventsToString() const;

		void doNotLogHup() { logHup_ = false; }

		EventLoop* ownerLoop() { return loop_; }
		void remove();

	private:
		void update();
		void handleEventWithGuard(Timestamp receiveTime);

		static const int kNoneEvent;
		static const int kReadEvent;
		static const int kWriteEvent;

		EventLoop* loop_;			// ����EventLoop
		const int  fd_;			// �ļ�����������������رո��ļ�������
		int        events_;		// ��ע���¼�
		int        revents_;		// poll/epoll���ص��¼�
		int        index_;		// used by Poller.��ʾ��poll���¼������е����
		bool       logHup_;		// for POLLHUP

		std::weak_ptr<void> tie_;
		bool tied_;
		bool eventHandling_;		// �Ƿ��ڴ����¼���
		ReadEventCallback readCallback_;
		EventCallback writeCallback_;
		EventCallback closeCallback_;
		EventCallback errorCallback_;
	};

}
#endif  // NET_CHANNEL_H
