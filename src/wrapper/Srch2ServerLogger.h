//$Id: Srch2ServerLogger.h 3456 2013-06-14 02:11:13Z jiaying $

#ifndef __SRCH2SERVERLOGGER_H__
#define __SRCH2SERVERLOGGER_H__

#include <stdio.h>
#include <string>

namespace srch2
{
namespace httpwrapper
{


class Srch2ServerLogger
{
public:
    Srch2ServerLogger(std::string logFilePath)
    {
        this->verbosity = 0;  /* TODO set in conf.ini */

        this->logFilePath = logFilePath;
        this->logFile = (logFilePath == "") ? stdout : fopen(logFilePath.c_str(),"a");
    }

    ~Srch2ServerLogger()
    {
        if (this->logFile && this->logFilePath != "")
            fclose(this->logFile);
    }

    void BMLogRaw(const /*uint8_h*/int level, const char *msg) const;

    void BMLog(const int level, const char *fmt, ...) const;

private:
    std::string logFilePath;
    FILE *logFile;
    int verbosity;
    static const unsigned MAX_LOGMSG_LEN = 500;
};



}}

#endif // __SRCH2SERVERLOGGER_H__
