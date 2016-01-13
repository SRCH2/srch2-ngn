
#ifndef ASSERT_H_
#define ASSERT_H_

#ifndef ANDROID
#include <execinfo.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "Logger.h"

using namespace srch2::util;

namespace srch2
{
namespace instantsearch
{

inline void print_trace (void)
{
#ifndef ANDROID
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace (array, 10);
    strings = backtrace_symbols (array, size);

    Logger::error("Obtained %d stack frames.\n", (int) size);

    for (i = 0; i < size; i++)
    	Logger::error ("%s\n", strings[i]);

    free (strings);
#endif
}

#ifndef ASSERT_LEVEL
#define ASSERT_LEVEL 1
#endif

#if ASSERT_LEVEL > 0
#define ASSERT(cond) do {\
        if (cond) {\
        } else {\
            Logger::error("Assert failed in file=%s and line %d\n", __FILE__, __LINE__); \
            print_trace();\
            abort();\
        }\
} while(0)
#else

#define ASSERT(cond) (void)0

#endif
}}

#endif /* ASSERT_H_ */
