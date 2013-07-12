//$Id$
#include "Logger.h"

#include <cstdarg>
#include <cstring>
#include <ctime>

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
	strftime(buffer, size, "%x %X", &tstruct);
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
	__android_log_print(ANDROID_LOG_DEBUG, prefix, str);
#else
	fprintf(out, "%s\n", str);
#endif
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

void Logger::debug(const char *format, ...) {
#ifndef SRCH2_SHIPPING
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

}
}

