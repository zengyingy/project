#include "InetAddress.h"
#include <strings.h> //bzero

using namespace net;

InetAddress::InetAddress(uint16_t port)
{
	bzero(&addr_, sizeof(addr_));
	addr_.sin6_family = AF_INET6;
	addr_.sin6_addr = in6addr_any;
	addr_.sin6_port = htons(port);
}

InetAddress::InetAddress(const string& ip, uint16_t port)
{
	bzero(&addr_, sizeof(addr_));
	addr_.sin6_family = AF_INET6;
	if (::inet_pton(AF_INET6, ip.c_str(), addr_.sin6_addr) <= 0)
	{
		LOG_SYSERR << "sockets::fromIpPort";
	}
	addr_.sin6_port = htons(port);
}

string InetAddress::toIp() const
{
	char buf[128];
	inet_ntop(AF_INET6, &addr_sin6_addr, buf, 128);
	return buf;
}

string InetAddress::toIpPort() const
{
	char buf[128];
	char host[128] = "INVALID";
	inet_ntop(AF_INET6, &addr_sin6_addr, host, 128);
	uint16_t port = ntohs(addr_.sin6_port);
	snprintf(buf, 128, "%s:%u", host, port);
	return buf;
}