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
#ifndef __WRAPPER_PARSERUTILITY_H__
#define __WRAPPER_PARSERUTILITY_H__

#include <string>
#include <cstdlib>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <event.h>
#include <evhttp.h>
#include <event2/http.h>
#include <sys/queue.h>
#include "boost/regex.hpp"
#include <boost/algorithm/string.hpp>
#include "util/Logger.h"
#include "WrapperConstants.h"
using boost::posix_time::time_input_facet;
using std::locale;

using namespace std;
using srch2::util::Logger;
namespace srch2 {
namespace httpwrapper {

/*
 * TODO : put these functions in a class and make them static
 */

// trim from start
std::string &ltrim(std::string &s);

// trim from end
std::string &rtrim(std::string &s);

// trim from both ends
std::string &trim(std::string &s);

std::string &removeWhiteSpace(std::string &s);

bool isInteger(const std::string & s);

bool isUnsigned(const std::string & s);

bool isFloat(const std::string & s);

bool isTime(const std::string & s);


void custom_evhttp_find_headers(const struct evkeyvalq *headers,
        const char *key, vector<string> &values);
/*
 * parses using the given regex
 */
bool doParse(string &input, const boost::regex &re, string &output);

void custom_evhttp_find_headers(const struct evkeyvalq *headers,
        const char *key, vector<string> &values);

bool validateValueWithType(srch2::instantsearch::FilterType type,
        string & value);
}
}

#endif // __WRAPPER_PARSERUTILITY_H__
