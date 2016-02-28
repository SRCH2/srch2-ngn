/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
