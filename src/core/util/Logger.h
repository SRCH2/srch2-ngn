//$Id$

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

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
	static inline void setOutputFile(FILE* file) {
		_out_file = file;
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
};

}
}

#endif /* _SRCH2_UTIL_LOG_H_ */
