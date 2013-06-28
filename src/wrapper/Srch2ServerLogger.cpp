//$Id: Srch2ServerLogger.cpp 3456 2013-06-14 02:11:13Z jiaying $

#include "Srch2ServerLogger.h"
#include <syslog.h>
#include <stdarg.h>

namespace srch2
{
namespace httpwrapper
{

// Inspired by Redis.c code. - Vijay
/*============================ Utility functions ============================ */

/* Low level logging. To use only for very big messages, otherwise
 * redisLog() is to prefer. */
void Srch2ServerLogger::BMLogRaw(const /*uint8_h*/int level, const char *msg) const
{
//	const int syslogLevelMap[] = { LOG_DEBUG, LOG_INFO, LOG_NOTICE, LOG_WARNING };
//	const char *c = ".-*#";
	time_t now = time(NULL);
	char buf[64];
//	int rawmode = (level & BM_LOG_RAW);

	//level &= 0xff; // clear flags
//	if (level < this->verbosity) return;

//	if (rawmode) {
//		fprintf(fp,"%s",msg);
//	} else {
		strftime(buf,sizeof(buf),"%x %X",gmtime(&now));
		//fprintf(fp,"[%d] %s %c %s\n",(int)getpid(),buf,c[level],msg);
		fprintf(this->logFile, "%s %s\n", buf, msg);
//	}
	fflush(this->logFile);

//	if (this->syslog_enabled) syslog(syslogLevelMap[level], "%s", msg);
}

/* Like redisLogRaw() but with printf-alike support. This is the funciton that
 * is used across the code. The raw version is only used in order to dump
 * the INFO output on crash. */
void Srch2ServerLogger::BMLog(const int level, const char *fmt, ...) const
{
	va_list ap;
	char msg[MAX_LOGMSG_LEN];

	if ((level&0xff) < this->verbosity) return;

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	if (this->logFile)
	    BMLogRaw(level, msg);
}


}
}
