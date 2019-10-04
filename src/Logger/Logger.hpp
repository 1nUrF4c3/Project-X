#pragma once

#include <fstream>
#include <cstdarg>
#include <ctime>
#include <cstdio>

#include <Config.h>

namespace xlog
{
	namespace LogLevel
	{
		enum e
		{
			fatal,
			error,
			critical,
			warning,
			normal,
			verbose
		};
	}

	static const char* sLogLevel[] = { "*FATAL*", "*ERROR*", "*CRITICAL*", "*WARNING*", "*NORMAL*", "*VERBOSE*" };

	class cLogger
	{
	public:
		~cLogger()
		{
			_output << std::endl;
		}

		static cLogger& Instance()
		{
			static cLogger log;
			return log;
		}

		bool DoLog(LogLevel::e level, const char* fmt, ...)
		{
			va_list alist;
			bool result = false;

			va_start(alist, fmt);
			result = DoLogV(level, fmt, alist);
			va_end(alist);

			return result;
		}

		bool DoLogV(LogLevel::e level, const char* fmt, va_list vargs)
		{
			char varbuf[2048] = { NULL };
			char message[4096] = { NULL };
			char timebuf[256] = { NULL };

			auto t = std::time(nullptr);
			tm stm;
			localtime_s(&stm, &t);
			std::strftime(timebuf, _countof(timebuf), "%Y-%m-%d %H:%M:%S", &stm);

			vsprintf_s(varbuf, _countof(varbuf), fmt, vargs);
			sprintf_s(message, _countof(message), "%s %-12s %s", timebuf, sLogLevel[level], varbuf);

			_output << message << std::endl;

			return true;
		}

	private:

		cLogger()
		{
#ifdef USE64
			_output.open("Project-X64.log", std::ios::out | std::ios::app);
#else
			_output.open("Project-X32.log", std::ios::out | std::ios::app);
#endif
		}

		cLogger(const cLogger&) = delete;
		cLogger& operator = (const cLogger&) = delete;

		std::ofstream _output;
	};


	inline bool Fatal(const char* fmt, ...)
	{
		va_list alist;
		bool result = false;

		va_start(alist, fmt);
		result = cLogger::Instance().DoLogV(LogLevel::fatal, fmt, alist);
		va_end(alist);

		return result;
	}

	inline bool Error(const char* fmt, ...)
	{
		va_list alist;
		bool result = false;

		va_start(alist, fmt);
		result = cLogger::Instance().DoLogV(LogLevel::error, fmt, alist);
		va_end(alist);

		return result;
	}

	inline bool Critical(const char* fmt, ...)
	{
		va_list alist;
		bool result = false;

		va_start(alist, fmt);
		result = cLogger::Instance().DoLogV(LogLevel::critical, fmt, alist);
		va_end(alist);

		return result;
	}

	inline bool Warning(const char* fmt, ...)
	{
		va_list alist;
		bool result = false;

		va_start(alist, fmt);
		result = cLogger::Instance().DoLogV(LogLevel::warning, fmt, alist);
		va_end(alist);

		return result;
	}

	inline bool Normal(const char* fmt, ...)
	{
		va_list alist;
		bool result = false;

		va_start(alist, fmt);
		result = cLogger::Instance().DoLogV(LogLevel::normal, fmt, alist);
		va_end(alist);

		return result;
	}

	inline bool Verbose(const char* fmt, ...)
	{
		va_list alist;
		bool result = false;

		va_start(alist, fmt);
		result = cLogger::Instance().DoLogV(LogLevel::verbose, fmt, alist);
		va_end(alist);

		return result;
	}
}
