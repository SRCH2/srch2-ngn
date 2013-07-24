#ifndef __CORE_UTIL_SRCH2_PROFILER_H__
#define __CORE_UTIL_SRCH2_PROFILER_H__
#include <gperftools/profiler.h>

void sProfilerStart(const char* fname);
void sProfilerStop();

#endif
