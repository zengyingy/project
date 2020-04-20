#ifndef BASE_LOGFILE_H
#define BASE_LOGFILE_H

#include "Mutex.h"
//#include <muduo/base/Types.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <string>

class LogFile : boost::noncopyable
{
public:
	LogFile(const std::string& basename,
		size_t rollSize,
		bool threadSafe = true,
		int flushInterval = 3);
	~LogFile();

	void append(const char* logline, int len);
	void flush();

private:
	void append_unlocked(const char* logline, int len);

	static std::string hostname();

	static std::string getLogFileName(const std::string& basename, time_t* now);
	void rollFile();

	const std::string basename_;		// 日志文件basename
	const size_t rollSize_;		// 日志文件达到rolSize_换一个新文件
	const int flushInterval_;		// 日志写入间隔时间

	int count_;

	boost::scoped_ptr<MutexLock> mutex_;
	time_t startOfPeriod_;	// 开始记录日志时间（调整至零点的时间）
	time_t lastRoll_;			// 上一次滚动日志文件时间
	time_t lastFlush_;		// 上一次日志写入文件时间
	class File;
	boost::scoped_ptr<File> file_;

	const static int kCheckTimeRoll_ = 1024;
	const static int kRollPerSeconds_ = 60 * 60 * 24;
};

#endif  // BASE_LOGFILE_H
