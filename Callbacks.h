#ifndef NET_CALLBACKS_H
#define NET_CALLBACKS_H

#include <functional>
#include <memory>

#include "Timestamp.h"

/*
namespace net
{
	// Adapted from google-protobuf stubs/common.h
	// see License in muduo/base/Types.h
	template<typename To, typename From>
	inline std::shared_ptr<To> down_pointer_cast(const std::shared_ptr<From>& f) {
		if (false) {
			implicit_cast<From*, To*>(0);
		}

#ifndef NDEBUG
		assert(f == NULL || dynamic_cast<To*>(get_pointer(f)) != NULL);
#endif
		return ::boost::static_pointer_cast<To>(f);
	}
*/
namespace net
{

	// All client visible callbacks go here.

	class Buffer;

	class TcpConnection;
	typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

	typedef std::function<void()> TimerCallback;

	typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
	typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
	typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
	typedef std::function<void(const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

	// the data has been read to (buf, len)
	typedef std::function<void(const TcpConnectionPtr&,
		Buffer*,
		Timestamp)> MessageCallback;

	void defaultConnectionCallback(const TcpConnectionPtr& conn);
	void defaultMessageCallback(const TcpConnectionPtr& conn,
		Buffer* buffer,
		Timestamp receiveTime);

	/*typedef boost::function<void (const TcpConnectionPtr&,
	const char* data,
	ssize_t len)> MessageCallback;*/

}

#endif  // NET_CALLBACKS_H
