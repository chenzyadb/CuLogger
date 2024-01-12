// CuLogger V2 by chenzyadb.
// Based on C++14 STL (MSVC).

#ifndef _CU_LOGGER_V2_H
#define _CU_LOGGER_V2_H

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <functional>

class LoggerExcept : public std::exception
{
	public:
		LoggerExcept(const std::string &message) : message_(message) { }

		const char* what() const noexcept override
		{
			return message_.c_str();
		}

	private:
		const std::string message_;
};

class CuLogger
{
	public:
		static constexpr int LOG_NONE = -1;
		static constexpr int LOG_ERROR = 0;
		static constexpr int LOG_WARNING = 1;
		static constexpr int LOG_INFO = 2;
		static constexpr int LOG_DEBUG = 3;

		static CuLogger* CreateLogger(const int &logLevel, const std::string &logPath)
		{
			if (instance_ != nullptr) {
				throw LoggerExcept("Logger already exist.");
			}
			std::call_once(flag_, [&]() {
				instance_ = new CuLogger(logLevel, logPath);
				if (instance_ == nullptr) {
					throw LoggerExcept("Failed to create logger.");
				}
			});
			return instance_;
		}

		static CuLogger* GetLogger()
		{
			if (instance_ == nullptr) {
				throw LoggerExcept("Logger has not been created.");
			}
			return instance_;
		}

		void ResetLogLevel(const int &level)
		{
			if (level < LOG_NONE || level > LOG_DEBUG) {
				throw LoggerExcept("Invaild log level.");
			}
			{
				std::unique_lock<std::mutex> lck(mtx_);
				logLevel_ = level;
			}
		}

		void Error(const char* format, ...)
		{
			std::unique_lock<std::mutex> lck(mtx_);
			va_list args{};
			va_start(args, format);
			size_t len = vsnprintf(nullptr, 0, format, args);
			va_end(args);
			if (len > 0) {
				auto size = len + 1;
				auto buffer = new char[size];
				memset(buffer, 0, size);
				va_start(args, format);
				vsnprintf(buffer, size, format, args);
				va_end(args);
				JoinLogQueue_(LOG_ERROR, buffer);
				delete[] buffer;
			}
		}

		void Warning(const char* format, ...)
		{
			std::unique_lock<std::mutex> lck(mtx_);
			va_list args{};
			va_start(args, format);
			size_t len = vsnprintf(nullptr, 0, format, args);
			va_end(args);
			if (len > 0) {
				auto size = len + 1;
				auto buffer = new char[size];
				memset(buffer, 0, size);
				va_start(args, format);
				vsnprintf(buffer, size, format, args);
				va_end(args);
				JoinLogQueue_(LOG_WARNING, buffer);
				delete[] buffer;
			}
		}

		void Info(const char* format, ...)
		{
			std::unique_lock<std::mutex> lck(mtx_);
			va_list args{};
			va_start(args, format);
			size_t len = vsnprintf(nullptr, 0, format, args);
			va_end(args);
			if (len > 0) {
				auto size = len + 1;
				auto buffer = new char[size];
				memset(buffer, 0, size);
				va_start(args, format);
				vsnprintf(buffer, size, format, args);
				va_end(args);
				JoinLogQueue_(LOG_INFO, buffer);
				delete[] buffer;
			}
		}

		void Debug(const char* format, ...)
		{
			std::unique_lock<std::mutex> lck(mtx_);
			va_list args{};
			va_start(args, format);
			size_t len = vsnprintf(nullptr, 0, format, args);
			va_end(args);
			if (len > 0) {
				auto size = len + 1;
				auto buffer = new char[size];
				memset(buffer, 0, size);
				va_start(args, format);
				vsnprintf(buffer, size, format, args);
				va_end(args);
				JoinLogQueue_(LOG_DEBUG, buffer);
				delete[] buffer;
			}
		}

		void Flush()
		{
			bool flushed = false;
			while (!flushed) {
				{
					std::unique_lock<std::mutex> lck(mtx_);
					flushed = queueFlushed_;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}

	private:
		CuLogger(const int &logLevel, const std::string &logPath) : 
			logPath_(logPath), 
			logLevel_(logLevel), 
			cv_(), 
			mtx_(), 
			logQueue_(), 
			queueFlushed_(true)
		{
			if (!CreateLog_(logPath_)) {
				throw LoggerExcept("Failed to create log file.");
			}
			std::thread thread_(std::bind(&CuLogger::MainLoop_, this));
			thread_.detach();
		}
		CuLogger() = delete;
		CuLogger(const CuLogger &other) = delete;
		CuLogger &operator=(const CuLogger &other) = delete;

		bool CreateLog_(const std::string &logPath)
		{
			auto fp = fopen(logPath.c_str(), "wt");
			if (fp) {
				fclose(fp);
				return true;
			}
			return false;
		}

		void MainLoop_()
		{
			auto fp = fopen(logPath_.c_str(), "at");
			if (!fp) {
				throw LoggerExcept("Failed to open log file.");
			}
			std::vector<std::string> writeQueue{};
			for (;;) {
				{
					std::unique_lock<std::mutex> lck(mtx_);
					queueFlushed_ = logQueue_.empty();
					while (logQueue_.empty()) {
						cv_.wait(lck);
					}
					writeQueue = logQueue_;
					logQueue_.clear();
				}
				if (!writeQueue.empty()) {
					for (const auto &log : writeQueue) {
						fputs(log.c_str(), fp);
					}
					fflush(fp);
					writeQueue.clear();
				}
			}
		}

		std::string GetTimeInfo_()
		{
			auto now = std::chrono::system_clock::now();
			auto nowTime = std::chrono::system_clock::to_time_t(now);
			auto localTime = std::localtime(std::addressof(nowTime));
			char buffer[16] = { 0 };
			snprintf(buffer, sizeof(buffer), "%02d-%02d %02d:%02d:%02d", 
					 localTime->tm_mon + 1, localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
			std::string timeInfo(buffer);
			return timeInfo;
		}

		void JoinLogQueue_(const int &level, const std::string &logText)
		{
			static const std::unordered_map<int, std::string> levelStrMap{
				{LOG_NONE, ""}, {LOG_ERROR, " [E] "}, {LOG_WARNING, " [W] "}, {LOG_INFO, " [I] "}, {LOG_DEBUG, " [D] "}
			};
			if (level <= logLevel_) { 
				logQueue_.emplace_back(GetTimeInfo_() + levelStrMap.at(level) + logText + "\n");
				cv_.notify_all();
			}
		}

		static CuLogger* instance_;
		static std::once_flag flag_;
		std::string logPath_;
		int logLevel_;
		std::condition_variable cv_;
		std::mutex mtx_;
		std::vector<std::string> logQueue_;
		bool queueFlushed_;
};

#endif
