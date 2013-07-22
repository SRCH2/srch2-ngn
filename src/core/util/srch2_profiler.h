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
