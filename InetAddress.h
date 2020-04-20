#ifndef InetAddress_h
#define InetAddrerss_h

#include<netinet/inh>
#include<string>

using std::string;

//写好socketops类改变字节转换函数

namespace net 
{

	class InetAddress
	{
	public:
		//ip为in6addr_any
		explicit InetAddress(uint16_t port);

		InetAddress(const string& ip, uint16_t port);

		InetAddress(const struct sockaddr_in6& addr) :addr_(addr) {}

		string toIp() const;
		string toIpPort() const;

		const struct sockaddr_in6& getSockAddrInet() const { return addr_; }
		void setSockAddrInet(const struct sockaddr_in6& addr) { addr_ = addr; }

		//返回的是结构体
		struct in6_addr ipNetEndian() const { return addr_.sin6_addr; }
		uint16_t portNetEndian() const { return addr_.sin6_port; }

	private:
		struct sockaddr_in6 addr_;
	};

}
#endif // !InetAddress_h
