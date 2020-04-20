#ifndef NET_ACCEPTOR_H
#define NET_ACCEPTOR_H

#include <functional>

#include <boost/noncopyable.hpp>

#include "Channel.h"
#include "Socket.h"


namespace net
{

	class EventLoop;
	class InetAddress;

	///
	/// Acceptor of incoming TCP connections.
	///
	class Acceptor : boost::noncopyable
	{
	public:
		typedef std::function<void(int sockfd,
			const InetAddress&)> NewConnectionCallback;

		Acceptor(EventLoop* loop, const InetAddress& listenAddr);
		~Acceptor();

		void setNewConnectionCallback(const NewConnectionCallback& cb)
		{
			newConnectionCallback_ = cb;
		}

		bool listenning() const { return listenning_; }
		void listen();

	private:
		void handleRead(Timestamp s=Timestamp());

		EventLoop* loop_;
		Socket acceptSocket_;
		Channel acceptChannel_;
		NewConnectionCallback newConnectionCallback_;
		bool listenning_;
		int idleFd_;
	};

}

#endif  // NET_ACCEPTOR_H
