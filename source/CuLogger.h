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
#include <ctime>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
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

		static void CreateLogger(const int &logLevel, const std::string &logPath)
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
			if (logLevel_ >= LOG_ERROR) {
				va_list args;
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
					std::string logText(buffer);
					delete[] buffer;
					logText = GetTimeInfo_() + " [E] " + logText + "\n";
					logQueue_.emplace_back(logText);
					cv_.notify_all();
				}
			}
		}

		void Warning(const char* format, ...)
		{
			std::unique_lock<std::mutex> lck(mtx_);
			if (logLevel_ >= LOG_WARNING) {
				va_list args;
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
					std::string logText(buffer);
					delete[] buffer;
					logText = GetTimeInfo_() + " [W] " + logText + "\n";
					logQueue_.emplace_back(logText);
					cv_.notify_all();
				}
			}
		}

		void Info(const char* format, ...)
		{
			std::unique_lock<std::mutex> lck(mtx_);
			if (logLevel_ >= LOG_INFO) {
				va_list args;
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
					std::string logText(buffer);
					delete[] buffer;
					logText = GetTimeInfo_() + " [I] " + logText + "\n";
					logQueue_.emplace_back(logText);
					cv_.notify_all();
				}
			}
		}

		void Debug(const char* format, ...)
		{
			std::unique_lock<std::mutex> lck(mtx_);
			if (logLevel_ >= LOG_DEBUG) {
				va_list args;
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
					std::string logText(buffer);
					delete[] buffer;
					logText = GetTimeInfo_() + " [D] " + logText + "\n";
					logQueue_.emplace_back(logText);
					cv_.notify_all();
				}
			}
		}

		void Flush()
		{
			bool flushed = false;
			while (!flushed) {
				std::unique_lock<std::mutex> lck(mtx_);
				flushed = queueFlushed_;
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
			std::thread thread_(std::bind(&CuLogger::LoggerMain_, this));
			thread_.detach();
		}
		CuLogger() = delete;
		CuLogger(const CuLogger &other) = delete;
		CuLogger &operator=(const CuLogger &other) = delete;

		bool CreateLog_(const std::string &logPath)
		{
			auto fp = fopen(logPath.c_str(), "w");
			if (fp) {
				fclose(fp);
				return true;
			}
			return false;
		}

		void LoggerMain_()
		{
			auto fp = fopen(logPath_.c_str(), "a");
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
			fclose(fp);
		}

		std::string GetTimeInfo_()
		{
			char buffer[16] = { 0 };
			auto time_stamp = time(nullptr);
			auto tm_ptr = localtime(std::addressof(time_stamp));
			int len = snprintf(buffer, sizeof(buffer), "%02d-%02d %02d:%02d:%02d",
				tm_ptr->tm_mon + 1, tm_ptr->tm_mday, tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec);
			std::string timeInfo(buffer);
			return timeInfo;
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
