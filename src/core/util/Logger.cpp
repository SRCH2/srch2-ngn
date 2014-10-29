//$Id$
#include "Logger.h"

#include <cstdarg>
#include <cstring>
#include <ctime>
#include <string>
#include <sstream>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef ANDROID
#include <android/log.h>
#endif

namespace srch2 {
namespace util {

#ifdef SRCH2_SHIPPING
Logger::LogLevel Logger::_logLevel = Logger::SRCH2_LOG_SILENT;
#else
Logger::LogLevel Logger::_logLevel = Logger::SRCH2_LOG_INFO;
#endif

FILE* Logger::_out_file = stdout;

char* Logger::formatCurrentTime(char* buffer, unsigned size) {
	time_t now = time(0);
	struct tm tstruct;
	tstruct = *localtime(&now);
	strftime(buffer, size, "%F %X", &tstruct);
	return buffer;
}

char* Logger::formatLogString(char* buffer, const char* prefix) {
	char timebuffer[32];
	sprintf(buffer, "%s ", formatCurrentTime(timebuffer, 32));
	if (prefix != NULL) {
		sprintf(buffer + strlen(buffer), "%s\t", prefix);
	}

	return buffer;
}

void Logger::writeToFile(FILE* out, const char* str) {
#ifdef ANDROID
	if (out == stderr || out == stdout) {
		__android_log_print(ANDROID_LOG_DEBUG, "SRCH2", str);
		return;
	}
#endif
	fprintf(out, "%s\n", str);
	fflush(out);

}

void Logger::console(const char *format, ...) {
	char buffer[kMaxLengthOfMessage] = { 0 };
	va_list args;
	va_start(args, format);
	formatLogString(buffer, "INFO");
	vsnprintf(buffer + strlen(buffer), kMaxLengthOfMessage - strlen(buffer),
			format, args);
	va_end(args);
	writeToFile(_out_file, buffer);
	if (_out_file != stdout && _out_file != stderr) {
		writeToFile(stdout, buffer);
	}
}

void Logger::error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[kMaxLengthOfMessage] = { 0 };
	formatLogString(buffer, "ERROR");
	vsnprintf(buffer + strlen(buffer), kMaxLengthOfMessage - strlen(buffer),
			format, args);
	va_end(args);
	if (_logLevel >= Logger::SRCH2_LOG_ERROR) {
		writeToFile(_out_file, buffer);
	}
	if (_out_file != stdout && _out_file != stderr) {
		writeToFile(stderr, buffer);
	}
}

void Logger::warn(const char *format, ...) {
	if (_logLevel >= Logger::SRCH2_LOG_WARN) {
		va_list args;
		va_start(args, format);
		char buffer[kMaxLengthOfMessage] = { 0 };
		formatLogString(buffer, "WARN");
		vsnprintf(buffer + strlen(buffer), kMaxLengthOfMessage - strlen(buffer),
				format, args);
		va_end(args);
		writeToFile(_out_file, buffer);
	}
}

void Logger::info(const char *format, ...) {
	if (_logLevel >= Logger::SRCH2_LOG_INFO) {
		va_list args;
		va_start(args, format);
		char buffer[kMaxLengthOfMessage] = { 0 };
		formatLogString(buffer, "INFO");
		vsnprintf(buffer + strlen(buffer), kMaxLengthOfMessage - strlen(buffer),
				format, args);
		va_end(args);
		writeToFile(_out_file, buffer);
	}
}

// make the static FILE * point to the new logger file and return the old FILE*
FILE* Logger::swapLoggerFile(FILE * newLogger) {
	// TODO: concurrent control
	if(newLogger == NULL) {
		Logger::error("The given logger file is NULL!!!");
		return NULL;
	}
	FILE* oldLogger = _out_file;
	_out_file = newLogger;
	return oldLogger;
}

void Logger::debug(const char *format, ...) {
#ifndef NDEBUG
    // NDEBUG is defined when we have run cmake BUILD_RELEASE=ON ..
	if (_logLevel >= Logger::SRCH2_LOG_DEBUG) {
		va_list args;
		va_start(args, format);
		char buffer[kMaxLengthOfMessage] = { 0 };
		formatLogString(buffer, "DEBUG");
		vsnprintf(buffer + strlen(buffer), kMaxLengthOfMessage - strlen(buffer),
				format, args);
		va_end(args);
		writeToFile(_out_file, buffer);
	}
#endif
}
void Logger::sharding(ShardingLogLevel logLevel, const char *format, ...){
#ifndef NDEBUG
    // NDEBUG is defined when we have run cmake BUILD_RELEASE=ON ..
	if (_logLevel >= Logger::SRCH2_LOG_DEBUG) {
		va_list args;
		va_start(args, format);
		char buffer[kMaxLengthOfMessage] = { 0 };
		std::stringstream ss;
		ss << "SHARDING-";
		switch (logLevel) {
			case Error:
				ss << "ERROR";
				break;
			case Step:
				ss << "STEP";
				break;
			case Detail:
				ss << "DETAIL";
				break;
			case Info:
				ss << "INFO";
				break;
			case FuncLine:
				ss << "F/L";
				break;
			default:
				break;
		}
		ss << "\t";
		ss << "P" << getpid() << "/T" << (unsigned long)pthread_self() << " || ";
		formatLogString(buffer, ss.str().c_str());
		vsnprintf(buffer + strlen(buffer), kMaxLengthOfMessage - strlen(buffer),
				format, args);
		va_end(args);
		writeToFile(_out_file, buffer);
	}
#endif
}


}
}

