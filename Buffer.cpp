#include "Buffer.h"

#include "SocketsOps.h"

#include <errno.h>
#include <sys/uio.h>

using namespace net;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

// ���ջ�ϵĿռ䣬�����ڴ�ʹ�ù�������ڴ�ʹ����
// �����5K�����ӣ�ÿ�����Ӿͷ���64K+64K�Ļ������Ļ�����ռ��640M�ڴ棬
// �������ʱ����Щ��������ʹ���ʺܵ�
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
	// saved an ioctl()/FIONREAD call to tell how much to read
	// ��ʡһ��ioctlϵͳ���ã���ȡ�ж��ٿɶ����ݣ�
	char extrabuf[65536];
	struct iovec vec[2];
	const size_t writable = writableBytes();
	// ��һ�黺����
	vec[0].iov_base = begin() + writerIndex_;
	vec[0].iov_len = writable;
	// �ڶ��黺����
	vec[1].iov_base = extrabuf;
	vec[1].iov_len = sizeof extrabuf;
	const ssize_t n = sockets::readv(fd, vec, 2);
	if (n < 0)
	{
		*savedErrno = errno;
	}
	else if (static_cast<size_t>(n) <= writable)	//��һ�黺�����㹻����
	{
		writerIndex_ += n;
	}
	else		// ��ǰ���������������ɣ�������ݱ����յ��˵ڶ��黺����extrabuf������append��buffer
	{
		writerIndex_ = buffer_.size();
		append(extrabuf, n - writable);
	}
	// if (n == writable + sizeof extrabuf)
	// {
	//   goto line_30;
	// }
	return n;
}

