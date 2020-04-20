#ifndef InetAddress_h
#define InetAddrerss_h

#include<netinet/inh>
#include<string>

using std::string;

//д��socketops��ı��ֽ�ת������

namespace net 
{

	class InetAddress
	{
	public:
		//ipΪin6addr_any
		explicit InetAddress(uint16_t port);

		InetAddress(const string& ip, uint16_t port);

		InetAddress(const struct sockaddr_in6& addr) :addr_(addr) {}

		string toIp() const;
		string toIpPort() const;

		const struct sockaddr_in6& getSockAddrInet() const { return addr_; }
		void setSockAddrInet(const struct sockaddr_in6& addr) { addr_ = addr; }

		//���ص��ǽṹ��
		struct in6_addr ipNetEndian() const { return addr_.sin6_addr; }
		uint16_t portNetEndian() const { return addr_.sin6_port; }

	private:
		struct sockaddr_in6 addr_;
	};

}
#endif // !InetAddress_h
