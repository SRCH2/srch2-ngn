//$Id: BimapleServerLogger.h 2647 2012-07-03 17:02:15Z oliverax $

#ifndef __BIMAPLESERVERLOGGER_H__
#define __BIMAPLESERVERLOGGER_H__

#include <stdio.h>
#include <string>

namespace bimaple
{
namespace httpwrapper
{


class BimapleServerLogger
{
public:
    BimapleServerLogger(std::string logFilePath)
    {
        this->verbosity = 0;  /* TODO set in conf.ini */

        this->logFilePath = logFilePath;
        this->logFile = (logFilePath == "") ? stdout : fopen(logFilePath.c_str(),"a");
    }

    ~BimapleServerLogger()
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

#endif // __BIMAPLESERVERLOGGER_H__
