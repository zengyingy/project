#include "TcpConnection.h"

#include "Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"

#include <functional>

#include <errno.h>
#include <stdio.h>

using namespace net;

/*
void muduo::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
	LOG_TRACE << conn->localAddress().toIpPort() << " -> "
		<< conn->peerAddress().toIpPort() << " is "
		<< (conn->connected() ? "UP" : "DOWN");
}

void muduo::net::defaultMessageCallback(const TcpConnectionPtr&,
	Buffer* buf,
	Timestamp)
{
	buf->retrieveAll();
}
*/
TcpConnection::TcpConnection(EventLoop* loop,
	const string& nameArg,
	int sockfd,
	const InetAddress& localAddr,
	const InetAddress& peerAddr)
	: loop_(CHECK_NOTNULL(loop)),
	name_(nameArg),
	state_(kConnecting),
	socket_(new Socket(sockfd)),
	channel_(new Channel(loop, sockfd)),
	localAddr_(localAddr),
	peerAddr_(peerAddr),
	highWaterMark_(64 * 1024 * 1024)
{
	// ͨ���ɶ��¼�������ʱ�򣬻ص�TcpConnection::handleRead��_1���¼�����ʱ��
	channel_->setReadCallback(
		std::bind(&TcpConnection::handleRead, this, _1));
	// ͨ����д�¼�������ʱ�򣬻ص�TcpConnection::handleWrite
	channel_->setWriteCallback(
		std::bind(&TcpConnection::handleWrite, this));
	// ���ӹرգ��ص�TcpConnection::handleClose
	channel_->setCloseCallback(
		std::bind(&TcpConnection::handleClose, this));
	// �������󣬻ص�TcpConnection::handleError
	channel_->setErrorCallback(
		std::bind(&TcpConnection::handleError, this));
	LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
		<< " fd=" << sockfd;
	socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
	LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
		<< " fd=" << channel_->fd();
}

// �̰߳�ȫ�����Կ��̵߳���
void TcpConnection::send(const void* data, size_t len)
{
	if (state_ == kConnected)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(data, len);
		}
		else
		{
			string message(static_cast<const char*>(data), len);
			loop_->runInLoop(
				std::bind(&TcpConnection::sendInLoop,
					this,
					message));
		}
	}
}
/*
// �̰߳�ȫ�����Կ��̵߳���
void TcpConnection::send(const StringPiece& message)
{
	if (state_ == kConnected)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(message);
		}
		else
		{
			loop_->runInLoop(
				boost::bind(&TcpConnection::sendInLoop,
					this,
					message.as_string()));
			//std::forward<string>(message)));
		}
	}
}
*/
// �̰߳�ȫ�����Կ��̵߳���
void TcpConnection::send(Buffer* buf)
{
	if (state_ == kConnected)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(buf->peek(), buf->readableBytes());
			buf->retrieveAll();
		}
		else
		{
			loop_->runInLoop(
				std::bind(&TcpConnection::sendInLoop,
					this,
					buf->retrieveAllAsString()));
			//std::forward<string>(message)));
		}
	}
}
/*
void TcpConnection::sendInLoop(const StringPiece& message)
{
	sendInLoop(message.data(), message.size());
}
*/
void TcpConnection::sendInLoop(const void* data, size_t len)
{
	/*
	loop_->assertInLoopThread();
	sockets::write(channel_->fd(), data, len);
	*/

	loop_->assertInLoopThread();
	ssize_t nwrote = 0;
	size_t remaining = len;
	bool error = false;
	if (state_ == kDisconnected)
	{
		LOG_WARN << "disconnected, give up writing";
		return;
	}
	// if no thing in output queue, try writing directly
	// ͨ��û�й�ע��д�¼����ҷ��ͻ�����û�����ݣ�ֱ��write
	if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
	{
		nwrote = sockets::write(channel_->fd(), data, len);
		if (nwrote >= 0)
		{
			remaining = len - nwrote;
			// д���ˣ��ص�writeCompleteCallback_
			if (remaining == 0 && writeCompleteCallback_)
			{
				loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
			}
		}
		else // nwrote < 0
		{
			nwrote = 0;
			if (errno != EWOULDBLOCK)
			{
				LOG_SYSERR << "TcpConnection::sendInLoop";
				if (errno == EPIPE) // FIXME: any others?
				{
					error = true;
				}
			}
		}
	}

	assert(remaining <= len);
	// û�д��󣬲��һ���δд������ݣ�˵���ں˷��ͻ���������Ҫ��δд���������ӵ�output buffer�У�
	if (!error && remaining > 0)
	{
		LOG_TRACE << "I am going to write more data";
		size_t oldLen = outputBuffer_.readableBytes();
		// �������highWaterMark_����ˮλ�꣩���ص�highWaterMarkCallback_
		if (oldLen + remaining >= highWaterMark_
			&& oldLen < highWaterMark_
			&& highWaterMarkCallback_)
		{
			loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
		}
		outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
		if (!channel_->isWriting())
		{
			channel_->enableWriting();		// ��עPOLLOUT�¼�
		}
	}
}

void TcpConnection::shutdown()
{
	// FIXME: use compare and swap
	if (state_ == kConnected)
	{
		setState(kDisconnecting);
		// FIXME: shared_from_this()?
		loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
	}
}

void TcpConnection::shutdownInLoop()
{
	loop_->assertInLoopThread();
	if (!channel_->isWriting())
	{
		// we are not writing
		socket_->shutdownWrite();
	}
}

void TcpConnection::setTcpNoDelay(bool on)
{
	socket_->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished()
{
	loop_->assertInLoopThread();
	assert(state_ == kConnecting);
	setState(kConnected);
	LOG_TRACE << "[3] usecount=" << shared_from_this().use_count();
	channel_->tie(shared_from_this());
	channel_->enableReading();	// TcpConnection����Ӧ��ͨ�����뵽Poller��ע

	connectionCallback_(shared_from_this());
	LOG_TRACE << "[4] usecount=" << shared_from_this().use_count();
}

void TcpConnection::connectDestroyed()
{
	loop_->assertInLoopThread();
	if (state_ == kConnected)
	{
		setState(kDisconnected);
		channel_->disableAll();

		connectionCallback_(shared_from_this());
	}
	channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
	/*
	loop_->assertInLoopThread();
	int savedErrno = 0;
	ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
	if (n > 0)
	{
	messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
	}
	else if (n == 0)
	{
	handleClose();
	}
	else
	{
	errno = savedErrno;
	LOG_SYSERR << "TcpConnection::handleRead";
	handleError();
	}
	*/

	/*
	loop_->assertInLoopThread();
	int savedErrno = 0;
	char buf[65536];
	ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
	if (n > 0)
	{
	messageCallback_(shared_from_this(), buf, n);
	}
	else if (n == 0)
	{
	handleClose();
	}
	else
	{
	errno = savedErrno;
	LOG_SYSERR << "TcpConnection::handleRead";
	handleError();
	}
	*/
	while (true) {
		loop_->assertInLoopThread();
		int savedErrno = 0;
		ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
		if (n > 0)
		{
			messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
		}
		else if (n == 0)
		{
			handleClose();
			break;
		}
		else
		{
			errno = savedErrno;
			LOG_SYSERR << "TcpConnection::handleRead";
			handleError();
			break;
		}
	}
}

// �ں˷��ͻ������пռ��ˣ��ص��ú���
void TcpConnection::handleWrite()
{
	loop_->assertInLoopThread();
	if (channel_->isWriting())
	{
		ssize_t n = sockets::write(channel_->fd(),
			outputBuffer_.peek(),
			outputBuffer_.readableBytes());
		if (n > 0)
		{
			outputBuffer_.retrieve(n);
			if (outputBuffer_.readableBytes() == 0)	 // ���ͻ����������
			{
				channel_->disableWriting();		// ֹͣ��עPOLLOUT�¼����������busy loop
				if (writeCompleteCallback_)		// �ص�writeCompleteCallback_
				{
					// Ӧ�ò㷢�ͻ���������գ��ͻص���writeCompleteCallback_
					loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
				}
				if (state_ == kDisconnecting)	// ���ͻ���������ղ�������״̬��kDisconnecting, Ҫ�ر�����
				{
					shutdownInLoop();		// �ر�����
				}
			}
			else
			{
				LOG_TRACE << "I am going to write more data";
			}
		}
		else
		{
			LOG_SYSERR << "TcpConnection::handleWrite";
			// if (state_ == kDisconnecting)
			// {
			//   shutdownInLoop();
			// }
		}
	}
	else
	{
		LOG_TRACE << "Connection fd = " << channel_->fd()
			<< " is down, no more writing";
	}
}

void TcpConnection::handleClose()
{
	loop_->assertInLoopThread();
	LOG_TRACE << "fd = " << channel_->fd() << " state = " << state_;
	assert(state_ == kConnected || state_ == kDisconnecting);
	// we don't close fd, leave it to dtor, so we can find leaks easily.
	setState(kDisconnected);
	channel_->disableAll();

	TcpConnectionPtr guardThis(shared_from_this());
	connectionCallback_(guardThis);		// ��һ�У����Բ�����
	LOG_TRACE << "[7] usecount=" << guardThis.use_count();
	// must be the last line
	closeCallback_(guardThis);	// ����TcpServer::removeConnection
	LOG_TRACE << "[11] usecount=" << guardThis.use_count();
}

void TcpConnection::handleError()
{
	int err = sockets::getSocketError(channel_->fd());
	LOG_ERROR << "TcpConnection::handleError [" << name_
		<< "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
