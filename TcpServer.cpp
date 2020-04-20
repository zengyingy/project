#include "TcpServer.h"

#include "Logging.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "SocketsOps.h"

#include <stdio.h>  // snprintf

using namespace net;

TcpServer::TcpServer(EventLoop* loop,
	const InetAddress& listenAddr,
	const string& nameArg)
	: loop_(CHECK_NOTNULL(loop)),
	hostport_(listenAddr.toIpPort()),
	name_(nameArg),
	acceptor_(new Acceptor(loop, listenAddr)),
	threadPool_(new EventLoopThreadPool(loop)),
	connectionCallback_(defaultConnectionCallback),
	messageCallback_(defaultMessageCallback),
	started_(false),
	nextConnId_(1)
{
	// Acceptor::handleRead�����л�ص���TcpServer::newConnection
	// _1��Ӧ����socket�ļ���������_2��Ӧ���ǶԵȷ��ĵ�ַ(InetAddress)
	acceptor_->setNewConnectionCallback(
		std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
	loop_->assertInLoopThread();
	LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

	for (ConnectionMap::iterator it(connections_.begin());
		it != connections_.end(); ++it)
	{
		TcpConnectionPtr conn = it->second;
		it->second.reset();		// �ͷŵ�ǰ�����ƵĶ������ü�����һ
		conn->getLoop()->runInLoop(
			std::bind(&TcpConnection::connectDestroyed, conn));
		conn.reset();			// �ͷŵ�ǰ�����ƵĶ������ü�����һ
	}
}

void TcpServer::setThreadNum(int numThreads)
{
	assert(0 <= numThreads);
	threadPool_->setThreadNum(numThreads);
}

// �ú�����ε������޺���
// �ú������Կ��̵߳���
void TcpServer::start()
{
	if (!started_)
	{
		started_ = true;
		threadPool_->start(threadInitCallback_);
	}

	if (!acceptor_->listenning())
	{
		// get_pointer����ԭ��ָ��
		loop_->runInLoop(
			std::bind(&Acceptor::listen, &acceptor_));
	}
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
	loop_->assertInLoopThread();
	// �����ֽеķ�ʽѡ��һ��EventLoop
	EventLoop* ioLoop = threadPool_->getNextLoop();
	char buf[32];
	snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_);
	++nextConnId_;
	string connName = name_ + buf;

	LOG_INFO << "TcpServer::newConnection [" << name_
		<< "] - new connection [" << connName
		<< "] from " << peerAddr.toIpPort();
	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	/*TcpConnectionPtr conn(new TcpConnection(loop_,
	connName,
	sockfd,
	localAddr,
	peerAddr));*/

	TcpConnectionPtr conn(new TcpConnection(ioLoop,
		connName,
		sockfd,
		localAddr,
		peerAddr));

	LOG_TRACE << "[1] usecount=" << conn.use_count();
	connections_[connName] = conn;
	LOG_TRACE << "[2] usecount=" << conn.use_count();
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);

	conn->setCloseCallback(
		std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

	// conn->connectEstablished();
	ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
	LOG_TRACE << "[5] usecount=" << conn.use_count();

}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	/*
	loop_->assertInLoopThread();
	LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
	<< "] - connection " << conn->name();


	LOG_TRACE << "[8] usecount=" << conn.use_count();
	size_t n = connections_.erase(conn->name());
	LOG_TRACE << "[9] usecount=" << conn.use_count();

	(void)n;
	assert(n == 1);

	loop_->queueInLoop(
	boost::bind(&TcpConnection::connectDestroyed, conn));
	LOG_TRACE << "[10] usecount=" << conn.use_count();
	*/

	loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));

}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
		<< "] - connection " << conn->name();


	LOG_TRACE << "[8] usecount=" << conn.use_count();
	size_t n = connections_.erase(conn->name());
	LOG_TRACE << "[9] usecount=" << conn.use_count();

	(void)n;
	assert(n == 1);

	EventLoop* ioLoop = conn->getLoop();
	ioLoop->queueInLoop(
		std::bind(&TcpConnection::connectDestroyed, conn));

	//loop_->queueInLoop(
	//    boost::bind(&TcpConnection::connectDestroyed, conn));
	LOG_TRACE << "[10] usecount=" << conn.use_count();



}
