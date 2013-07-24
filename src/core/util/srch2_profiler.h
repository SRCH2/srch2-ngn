#ifndef __CORE_UTIL_SRCH2_PROFILER_H__
#define __CORE_UTIL_SRCH2_PROFILER_H__
#include <gperftools/profiler.h>
void sProfilerStart(const char* fname)
{
#ifdef ENABLE_PROFILER
    ProfilerStart(fname);
#endif
}
void sProfilerStop()
{
#ifdef ENABLE_PROFILER
    ProfilerStop();
#endif
}

#endif
