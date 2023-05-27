// CuLogger V1 by chenzyadb.

#ifndef _CU_LOGGER_H
#define _CU_LOGGER_H

#if defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <mutex>
#include <string>

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
				throw std::runtime_error("Logger already exist.");
			}
			std::call_once(flag_, CuLogger::CreateInstance_);
			if (logLevel >= LOG_NONE && logLevel <= LOG_DEBUG) {
				logLevel_ = logLevel;
			}
			if (logLevel_ != LOG_NONE) {
				if (CreateLog_(logPath)) {
					logPath_ = logPath;
				} else {
					logLevel_ = LOG_NONE;
				}
			}
		}

		static CuLogger* GetLogger()
		{
			return instance_;
		}

		void ResetLogLevel(const int &logLevel)
		{
			if (logLevel >= LOG_NONE && logLevel <= LOG_DEBUG) {
				logLevel_ = logLevel;
			}
		}

		void Error(const char* format, ...) const
		{
			std::string logText = "";
			{
				va_list arg;
				va_start(arg, format);
				int size = vsnprintf(nullptr, 0, format, arg);
				va_end(arg);
				if (size > 0) {
					logText.resize((size_t)size + 1);
					va_start(arg, format);
					vsprintf(&logText[0], format, arg);
					va_end(arg);
				}
				logText.resize(strlen(logText.c_str()));
			}
			if (logLevel_ >= LOG_ERROR) {
				logText = GetTimeInfo_() + " [E] " + logText + "\n";
				WriteLog_(logText);
			}
		}

		void Warning(const char* format, ...) const
		{
			std::string logText = "";
			{
				va_list arg;
				va_start(arg, format);
				int size = vsnprintf(nullptr, 0, format, arg);
				va_end(arg);
				if (size > 0) {
					logText.resize((size_t)size + 1);
					va_start(arg, format);
					vsprintf(&logText[0], format, arg);
					va_end(arg);
				}
				logText.resize(strlen(logText.c_str()));
			}
			if (logLevel_ >= LOG_WARNING) {
				logText = GetTimeInfo_() + " [W] " + logText + "\n";
				WriteLog_(logText);
			}
		}

		void Info(const char* format, ...) const
		{
			std::string logText = "";
			{
				va_list arg;
				va_start(arg, format);
				int size = vsnprintf(nullptr, 0, format, arg);
				va_end(arg);
				if (size > 0) {
					logText.resize((size_t)size + 1);
					va_start(arg, format);
					vsprintf(&logText[0], format, arg);
					va_end(arg);
				}
				logText.resize(strlen(logText.c_str()));
			}
			if (logLevel_ >= LOG_INFO) {
				logText = GetTimeInfo_() + " [I] " + logText + "\n";
				WriteLog_(logText);
			}
		}

		void Debug(const char* format, ...) const
		{
			std::string logText = "";
			{
				va_list arg;
				va_start(arg, format);
				int size = vsnprintf(nullptr, 0, format, arg);
				va_end(arg);
				if (size > 0) {
					logText.resize((size_t)size + 1);
					va_start(arg, format);
					vsprintf(&logText[0], format, arg);
					va_end(arg);
				}
				logText.resize(strlen(logText.c_str()));
			}
			if (logLevel_ >= LOG_DEBUG) {
				logText = GetTimeInfo_() + " [D] " + logText + "\n";
				WriteLog_(logText);
			}
		}

	private:
		CuLogger() = default;
		CuLogger(const CuLogger&) = delete;
		CuLogger &operator=(const CuLogger&) = delete;

		static void CreateInstance_()
		{
			instance_ = new CuLogger();
		}

		static bool CreateLog_(const std::string &logPath)
		{
			bool created = false;
			FILE* fp = fopen(logPath.c_str(), "w");
			if (fp) {
				fputs("", fp);
				fflush(fp);
				fclose(fp);
				created = true;
			}

			return created;
		}

		void WriteLog_(const std::string &text) const
		{
			FILE* fp = fopen(logPath_.c_str(), "a");
			if (fp) {
				fputs(text.c_str(), fp);
				fflush(fp);
				fclose(fp);
			}
		}

		std::string GetTimeInfo_() const
		{
			char timeInfo[16] = { 0 };
			time_t time_stamp = time(nullptr);
			struct tm* time_p = localtime(&time_stamp);
			snprintf(timeInfo, sizeof(timeInfo), "%02d-%02d %02d:%02d:%02d",
					time_p->tm_mon + 1, time_p->tm_mday, time_p->tm_hour, time_p->tm_min, time_p->tm_sec);

			return timeInfo;
		}

		static CuLogger* instance_;
		static std::once_flag flag_;
		static std::string logPath_;
		static int logLevel_;
};

CuLogger* CuLogger::instance_ = nullptr;
std::once_flag CuLogger::flag_;
std::string CuLogger::logPath_ = CuLogger::LOG_PATH_NONE;
int CuLogger::logLevel_ = CuLogger::LOG_NONE;

#endif
