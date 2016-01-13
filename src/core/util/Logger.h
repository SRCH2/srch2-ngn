
#ifndef __SRCH2_UTIL_LOG_H__
#define __SRCH2_UTIL_LOG_H__

#include <cstdio>

namespace srch2 {
namespace util {

class Logger {
public:
	typedef enum LogLevel {
		SRCH2_LOG_SILENT = 0,
		SRCH2_LOG_ERROR = 1,
		SRCH2_LOG_WARN = 2,
		SRCH2_LOG_INFO = 3,
		SRCH2_LOG_DEBUG = 4
	} LogLevel;

	static const int kMaxLengthOfMessage = 1024;
private:
	static LogLevel _logLevel;
	static FILE* _out_file;

	static char* formatCurrentTime(char* buffer, unsigned size);
	static char* formatLogString(char* buffer, const char* prefix);
	static void writeToFile(FILE* out, const char* str);
public:
        // Logger now owns FILE * - it may close it (for example, upon /resetLogger request)
	static inline void setOutputFile(FILE* file) {
		if (file != NULL){
			_out_file = file;
		}else{
			Logger::warn("The given logger file is NULL");
		}
	}
	static inline void setLogLevel(LogLevel logLevel) {
		_logLevel = logLevel;
	}
	static inline int getLogLevel() {
		return _logLevel;
	}

	static void console(const char *format, ...);
	static void error(const char *format, ...);
	static void warn(const char *format, ...);
	static void info(const char *format, ...);
	static void debug(const char *format, ...);
	static FILE* swapLoggerFile(FILE * newLogger);

        static void close() { if (_out_file != NULL) { fclose(_out_file);} }
};

}
}

#endif /* _SRCH2_UTIL_LOG_H_ */
