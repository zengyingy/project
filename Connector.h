#ifndef NET_CONNECTOR_H
#define NET_CONNECTOR_H

#include "InetAddress.h"

#include <memory>
#include <functional>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace net
{

	class Channel;
	class EventLoop;

	// �����������ӣ������Զ���������
	class Connector : boost::noncopyable,
		public std::enable_shared_from_this<Connector>
	{
	public:
		typedef std::function<void(int sockfd)> NewConnectionCallback;

		Connector(EventLoop* loop, const InetAddress& serverAddr);
		~Connector();

		void setNewConnectionCallback(const NewConnectionCallback& cb)
		{
			newConnectionCallback_ = cb;
		}

		void start();  // can be called in any thread
		void restart();  // must be called in loop thread
		void stop();  // can be called in any thread

		const InetAddress& serverAddress() const { return serverAddr_; }

	private:
		enum States { kDisconnected, kConnecting, kConnected };
		static const int kMaxRetryDelayMs = 30 * 1000;			// 30�룬��������ӳ�ʱ��
		static const int kInitRetryDelayMs = 500;				// 0.5�룬��ʼ״̬�����Ӳ��ϣ�0.5�������

		void setState(States s) { state_ = s; }
		void startInLoop();
		void stopInLoop();
		void connect();
		void connecting(int sockfd);
		void handleWrite();
		void handleError();
		void retry(int sockfd);
		int removeAndResetChannel();
		void resetChannel();

		EventLoop* loop_;			// ����EventLoop
		InetAddress serverAddr_;	// �������˵�ַ
		bool connect_; // atomic
		States state_;  // FIXME: use atomic variable
		boost::scoped_ptr<Channel> channel_;	// Connector����Ӧ��Channel
		NewConnectionCallback newConnectionCallback_;		// ���ӳɹ��ص�������
		int retryDelayMs_;		// �����ӳ�ʱ�䣨��λ�����룩
	};

}

#endif  // NET_CONNECTOR_H
