#ifndef SOCKET_H
#define SOCKET_H

#include <boost/noncopyable.hpp>

namespace net
{

	class InetAddress;

	///
	/// Wrapper of socket file descriptor.
	///
	/// It closes the sockfd when desctructs.
	/// It's thread safe, all operations are delagated to OS.
	class Socket : boost::noncopyable
	{
	public:
		explicit Socket(int sockfd)
			: sockfd_(sockfd)
		{ }

		// Socket(Socket&&) // move constructor in C++11
		~Socket();

		int fd() const { return sockfd_; }

		/// abort if address in use
		void bindAddress(const InetAddress& localaddr);
		/// abort if address in use
		void listen();

		/// On success, returns a non-negative integer that is
		/// a descriptor for the accepted socket, which has been
		/// set to non-blocking and close-on-exec. *peeraddr is assigned.
		/// On error, -1 is returned, and *peeraddr is untouched.
		int accept(InetAddress* peeraddr);

		void shutdownWrite();

		///
		/// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
		///
		// Nagle�㷨����һ���̶��ϱ�������ӵ��
		// TCP_NODELAYѡ����Խ���Nagle�㷨
		// ����Nagle�㷨�����Ա����������������ӳ٣�����ڱ�д���ӳٵ�����������Ҫ
		void setTcpNoDelay(bool on);

		///
		/// Enable/disable SO_REUSEADDR
		///
		void setReuseAddr(bool on);

		///
		/// Enable/disable SO_KEEPALIVE
		///
		// TCP keepalive��ָ����̽�������Ƿ���ڣ����Ӧ�ò��������Ļ������ѡ��Ǳ���Ҫ���õ�
		void setKeepAlive(bool on);

	private:
		const int sockfd_;
	};

}
#endif  // SOCKET_H
