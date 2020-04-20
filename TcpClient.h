#ifndef NET_TCPCLIENT_H
#define NET_TCPCLIENT_H

#include <boost/noncopyable.hpp>

#include "Mutex.h"
#include "TcpConnection.h"

namespace net
{

	class Connector;
	typedef std::shared_ptr<Connector> ConnectorPtr;

	class TcpClient : boost::noncopyable
	{
	public:
		// TcpClient(EventLoop* loop);
		// TcpClient(EventLoop* loop, const string& host, uint16_t port);
		TcpClient(EventLoop* loop,
			const InetAddress& serverAddr,
			const string& name);
		~TcpClient();  // force out-line dtor, for scoped_ptr members.

		void connect();
		void disconnect();
		void stop();

		TcpConnectionPtr connection() const
		{
			MutexLockGuard lock(mutex_);
			return connection_;
		}

		//bool retry() const;
		void enableRetry() { retry_ = true; }

		/// Set connection callback.
		/// Not thread safe.
		void setConnectionCallback(const ConnectionCallback& cb)
		{
			connectionCallback_ = cb;
		}

		/// Set message callback.
		/// Not thread safe.
		void setMessageCallback(const MessageCallback& cb)
		{
			messageCallback_ = cb;
		}

		/// Set write complete callback.
		/// Not thread safe.
		void setWriteCompleteCallback(const WriteCompleteCallback& cb)
		{
			writeCompleteCallback_ = cb;
		}

	private:
		/// Not thread safe, but in loop
		void newConnection(int sockfd);
		/// Not thread safe, but in loop
		void removeConnection(const TcpConnectionPtr& conn);

		EventLoop* loop_;
		ConnectorPtr connector_;	// ����������������
		const string name_;		// ����
		ConnectionCallback connectionCallback_;		// ���ӽ����ص�����
		MessageCallback messageCallback_;				// ��Ϣ�����ص�����
		WriteCompleteCallback writeCompleteCallback_;	// ���ݷ�����ϻص�����
		bool retry_;   // ��������ָ���ӽ���֮���ֶϿ���ʱ���Ƿ�����
		bool connect_; // atomic
					   // always in loop thread
		int nextConnId_;			// name_ + nextConnId_���ڱ�ʶһ������
		mutable MutexLock mutex_;
		TcpConnectionPtr connection_; // Connector���ӳɹ��Ժ󣬵õ�һ��TcpConnection
	};

}

#endif  // NET_TCPCLIENT_H
