#include "TcpClient.h"

#include "Logging.h"
#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <functional>

#include <stdio.h>  // snprintf

using namespace net;

// TcpClient::TcpClient(EventLoop* loop)
//   : loop_(loop)
// {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//   : loop_(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }


namespace net
{
	namespace detail
	{

		void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
		{
			loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
		}

		void removeConnector(const ConnectorPtr& connector)
		{
			//connector->
		}

	}
}

TcpClient::TcpClient(EventLoop* loop,
	const InetAddress& serverAddr,
	const string& name)
	: loop_(CHECK_NOTNULL(loop)),
	connector_(new Connector(loop, serverAddr)),
	name_(name),
	connectionCallback_(defaultConnectionCallback),
	//messageCallback_(defaultMessageCallback),
	retry_(false),
	connect_(true),
	nextConnId_(1)
{
	// �������ӳɹ��ص�����
	connector_->setNewConnectionCallback(
		std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
	// FIXME setConnectFailedCallback
	LOG_INFO << "TcpClient::TcpClient[" << name_
		<< "] - connector ";
}

TcpClient::~TcpClient()
{
	LOG_INFO << "TcpClient::~TcpClient[" << name_
		<< "] - connector ";
	TcpConnectionPtr conn;
	{
		MutexLockGuard lock(mutex_);
		conn = connection_;
	}
	if (conn)
	{
		// FIXME: not 100% safe, if we are in different thread

		// ��������TcpConnection�е�closeCallback_Ϊdetail::removeConnection
		CloseCallback cb = std::bind(&detail::removeConnection, loop_, std::placeholders::_1);
		loop_->runInLoop(
			std::bind(&TcpConnection::setCloseCallback, conn, cb));
	}
	else
	{
		// ���������˵��connector����δ����״̬����connector_ֹͣ
		connector_->stop();
		// FIXME: HACK
		loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
	}
}

void TcpClient::connect()
{
	// FIXME: check state
	LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
		<< connector_->serverAddress().toIpPort();
	connect_ = true;
	connector_->start();	// ��������
}

// ���������ѽ���������£��ر�����
void TcpClient::disconnect()
{
	connect_ = false;

	{
		MutexLockGuard lock(mutex_);
		if (connection_)
		{
			connection_->shutdown();
		}
	}
}

// ֹͣconnector_
void TcpClient::stop()
{
	connect_ = false;
	connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
	loop_->assertInLoopThread();
	InetAddress peerAddr(sockets::getPeerAddr(sockfd));
	char buf[32];
	snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
	++nextConnId_;
	string connName = name_ + buf;

	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	TcpConnectionPtr conn(new TcpConnection(loop_,
		connName,
		sockfd,
		localAddr,
		peerAddr));

	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(
		std::bind(&TcpClient::removeConnection, this, std::placeholders::_1)); // FIXME: unsafe
	{
		MutexLockGuard lock(mutex_);
		connection_ = conn;		// ����TcpConnection
	}
	conn->connectEstablished();		// ����ص�connectionCallback_
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	assert(loop_ == conn->getLoop());

	{
		MutexLockGuard lock(mutex_);
		assert(connection_ == conn);
		connection_.reset();
	}

	loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
	if (retry_ && connect_)
	{
		LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to "
			<< connector_->serverAddress().toIpPort();
		// �����������ָ���ӽ����ɹ�֮�󱻶Ͽ�������
		connector_->restart();
	}
}

