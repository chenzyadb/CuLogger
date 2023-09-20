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
		static constexpr char LOG_PATH_NONE[] = "NONE";
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
			std::call_once(flag_, CuLogger::CreateInstance_);
			if (logLevel >= LOG_NONE && logLevel <= LOG_DEBUG) {
				logLevel_ = logLevel;
			}
			if (logLevel_ != LOG_NONE) {
				if (CreateLog_(logPath)) {
					logPath_ = logPath;
				} else {
					throw LoggerExcept("Failed to create log file.");
				}
			}
		}

		static CuLogger* GetLogger()
		{
			if (instance_ == nullptr) {
				throw LoggerExcept("Logger has not been created.");
			}

			return instance_;
		}

		void ResetLogLevel(const int &logLevel)
		{
			if (logLevel >= LOG_NONE && logLevel <= LOG_DEBUG) {
				logLevel_ = logLevel;
			}
		}

		void Error(const char* format, ...)
		{
			std::string logInfo = "";
			{
				va_list arg;
				va_start(arg, format);
				int size = vsnprintf(nullptr, 0, format, arg);
				va_end(arg);
				if (size > 0) {
					logInfo.resize((size_t)size + 1);
					va_start(arg, format);
					vsnprintf(&logInfo[0], logInfo.size(), format, arg);
					va_end(arg);
				}
				logInfo.resize(strlen(logInfo.c_str()));
			}
			if (logLevel_ >= LOG_ERROR) {
				auto log = GetTimeInfo_() + " [E] " + logInfo + "\n";
				{
					std::unique_lock<std::mutex> lck(mtx_);
					logQueue_.emplace_back(log);
					unblocked_ = true;
					cv_.notify_all();
				}
			}
		}

		void Warning(const char* format, ...)
		{
			std::string logInfo = "";
			{
				va_list arg;
				va_start(arg, format);
				int size = vsnprintf(nullptr, 0, format, arg);
				va_end(arg);
				if (size > 0) {
					logInfo.resize((size_t)size + 1);
					va_start(arg, format);
					vsnprintf(&logInfo[0], logInfo.size(), format, arg);
					va_end(arg);
				}
				logInfo.resize(strlen(logInfo.c_str()));
			}
			if (logLevel_ >= LOG_WARNING) {
				auto log = GetTimeInfo_() + " [W] " + logInfo + "\n";
				{
					std::unique_lock<std::mutex> lck(mtx_);
					logQueue_.emplace_back(log);
					unblocked_ = true;
					cv_.notify_all();
				}
			}
		}

		void Info(const char* format, ...)
		{
			std::string logInfo = "";
			{
				va_list arg;
				va_start(arg, format);
				int size = vsnprintf(nullptr, 0, format, arg);
				va_end(arg);
				if (size > 0) {
					logInfo.resize((size_t)size + 1);
					va_start(arg, format);
					vsnprintf(&logInfo[0], logInfo.size(), format, arg);
					va_end(arg);
				}
				logInfo.resize(strlen(logInfo.c_str()));
			}
			if (logLevel_ >= LOG_INFO) {
				auto log = GetTimeInfo_() + " [I] " + logInfo + "\n";
				{
					std::unique_lock<std::mutex> lck(mtx_);
					logQueue_.emplace_back(log);
					unblocked_ = true;
					cv_.notify_all();
				}
			}
		}

		void Debug(const char* format, ...)
		{
			std::string logInfo = "";
			{
				va_list arg;
				va_start(arg, format);
				int size = vsnprintf(nullptr, 0, format, arg);
				va_end(arg);
				if (size > 0) {
					logInfo.resize((size_t)size + 1);
					va_start(arg, format);
					vsnprintf(&logInfo[0], logInfo.size(), format, arg);
					va_end(arg);
				}
				logInfo.resize(strlen(logInfo.c_str()));
			}
			if (logLevel_ >= LOG_DEBUG) {
				auto log = GetTimeInfo_() + " [D] " + logInfo + "\n";
				{
					std::unique_lock<std::mutex> lck(mtx_);
					logQueue_.emplace_back(log);
					unblocked_ = true;
					cv_.notify_all();
				}
			}
		}

	private:
		CuLogger()
		{
			thread_ = std::thread(std::bind(&CuLogger::LoggerMain_, this));
			thread_.detach();
		}
		CuLogger(const CuLogger&) = delete;
		CuLogger &operator=(const CuLogger&) = delete;

		static void CreateInstance_()
		{
			instance_ = new CuLogger();
		}

		static bool CreateLog_(const std::string &logPath)
		{
			FILE* fp = fopen(logPath.c_str(), "w");
			if (fp) {
				fclose(fp);
				return true;
			}
			return false;
		}

		void LoggerMain_()
		{
			FILE* fp = fopen(logPath_.c_str(), "a");
			if (!fp) {
				throw LoggerExcept("Failed to open log file.");
			}
			for (;;) {
				std::vector<std::string> workQueue{};
				{
					std::unique_lock<std::mutex> lck(mtx_);
					while (!unblocked_) {
						cv_.wait(lck);
					}
					unblocked_ = false;
					if (!logQueue_.empty()) {
						workQueue = logQueue_;
						logQueue_.clear();
					}
				}
				if (!workQueue.empty()) {
					for (const auto &log : workQueue) {
						fputs(log.c_str(), fp);
					}
					fflush(fp);
				}
			}
			fclose(fp);
		}

		std::string GetTimeInfo_()
		{
			std::string timeInfo = std::string(16, '\0');
			time_t time_stamp = time(nullptr);
			struct tm time = *localtime(&time_stamp);
			snprintf(&timeInfo[0], timeInfo.size(), "%02d-%02d %02d:%02d:%02d",
					time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
			timeInfo.resize(strlen(timeInfo.c_str()));

			return timeInfo;
		}

		static CuLogger* instance_;
		static std::once_flag flag_;
		static std::string logPath_;
		static int logLevel_;
		static std::thread thread_;
		static std::condition_variable cv_;
		static std::mutex mtx_;
		static bool unblocked_;
		static std::vector<std::string> logQueue_;
};

#endif
