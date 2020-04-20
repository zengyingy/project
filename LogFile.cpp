#include "LogFile.h"
#include "Logging.h"
//#include <muduo/base/ProcessInfo.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace net;

// not thread safe
class LogFile::File : boost::noncopyable
{
public:
	explicit File(const std::string& filename)
		: fp_(::fopen(filename.data(), "ae")),
		writtenBytes_(0)
	{
		assert(fp_);
		::setbuffer(fp_, buffer_, sizeof buffer_);
		// posix_fadvise POSIX_FADV_DONTNEED ?
	}

	~File()
	{
		::fclose(fp_);
	}

	void append(const char* logline, const size_t len)
	{
		size_t n = write(logline, len);
		size_t remain = len - n;
		// remain>0��ʾûд�꣬��Ҫ����дֱ��д��
		while (remain > 0)
		{
			size_t x = write(logline + n, remain);
			if (x == 0)
			{
				int err = ferror(fp_);
				if (err)
				{
					fprintf(stderr, "LogFile::File::append() failed %s\n", strerror_tl(err));
				}
				break;
			}
			n += x;
			remain = len - n; // remain -= x
		}

		writtenBytes_ += len;
	}

	void flush()
	{
		::fflush(fp_);
	}

	size_t writtenBytes() const { return writtenBytes_; }

private:

	size_t write(const char* logline, size_t len)
	{
#undef fwrite_unlocked
		return ::fwrite_unlocked(logline, 1, len, fp_);
	}

	FILE* fp_;
	char buffer_[64 * 1024];
	size_t writtenBytes_;
};

LogFile::LogFile(const std::string& basename,
	size_t rollSize,
	bool threadSafe,
	int flushInterval)
	: basename_(basename),
	rollSize_(rollSize),
	flushInterval_(flushInterval),
	count_(0),
	mutex_(threadSafe ? new MutexLock : NULL),
	startOfPeriod_(0),
	lastRoll_(0),
	lastFlush_(0)
{
	assert(basename.find('/') == string::npos);
	rollFile();
}

LogFile::~LogFile()
{
}

void LogFile::append(const char* logline, int len)
{
	if (mutex_)
	{
		MutexLockGuard lock(*mutex_);
		append_unlocked(logline, len);
	}
	else
	{
		append_unlocked(logline, len);
	}
}

void LogFile::flush()
{
	if (mutex_)
	{
		MutexLockGuard lock(*mutex_);
		file_->flush();
	}
	else
	{
		file_->flush();
	}
}

void LogFile::append_unlocked(const char* logline, int len)
{
	file_->append(logline, len);

	if (file_->writtenBytes() > rollSize_)
	{
		rollFile();
	}
	else
	{
		if (count_ > kCheckTimeRoll_)
		{
			count_ = 0;
			time_t now = ::time(NULL);
			time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
			if (thisPeriod_ != startOfPeriod_)
			{
				rollFile();
			}
			else if (now - lastFlush_ > flushInterval_)
			{
				lastFlush_ = now;
				file_->flush();
			}
		}
		else
		{
			++count_;
		}
	}
}

void LogFile::rollFile()
{
	time_t now = 0;
	std::string filename = getLogFileName(basename_, &now);
	// ע�⣬�����ȳ�kRollPerSeconds_ ���kRollPerSeconds_��ʾ
	// ������kRollPerSeconds_��������Ҳ����ʱ�������������㡣
	time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

	if (now > lastRoll_)
	{
		lastRoll_ = now;
		lastFlush_ = now;
		startOfPeriod_ = start;
		file_.reset(new File(filename));
	}
}

std::string LogFile::hostname()
{
	char buf[64] = "unknownhost";
	buf[sizeof(buf) - 1] = '\0';
	::gethostname(buf, sizeof buf);
	return buf;
}

std::string LogFile::getLogFileName(const std::string& basename, time_t* now)
{
	std::string filename;
	filename.reserve(basename.size() + 64);
	filename = basename;

	char timebuf[32];
	char pidbuf[32];
	struct tm tm;
	*now = time(NULL);
	gmtime_r(now, &tm); // FIXME: localtime_r ?
	strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
	filename += timebuf;
	filename += hostname();
	snprintf(pidbuf, sizeof pidbuf, ".%d", ::getpid());
	filename += pidbuf;
	filename += ".log";

	return filename;
}

