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
#include "ParserUtility.h"
#include <string>
#include <cstdlib>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "util/DateAndTimeHandler.h"

using boost::posix_time::time_input_facet;
using std::locale;

using namespace std;

namespace srch2 {
namespace httpwrapper {

// trim from start
std::string &ltrim(std::string &s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                    std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
std::string &rtrim(std::string &s) {
    s.erase(
            std::find_if(s.rbegin(), s.rend(),
                    std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
            s.end());
    return s;
}

// trim from both ends
std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

std::string &removeWhiteSpace(std::string &s) {
    s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
    return s;
}

// source : http://stackoverflow.com/questions/2844817/how-do-i-check-if-a-c-string-is-an-int
bool isInteger(const std::string & s) {
    if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
        return false;

    char * p;
    int temp = strtol(s.c_str(), &p, 10);

    return (*p == 0);
}

bool isUnsigned(const std::string & s) {
    if (s.empty() || ((!isdigit(s[0])) && (s[0] != '+')))
        return false;
    char * p;
    int temp = strtol(s.c_str(), &p, 10);
    return (*p == 0);
}

bool isFloat(const std::string & s) {
    if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+') && (s[0] != '.')))
        return false;

    char * p;
    int temp = static_cast<float>(strtod(s.c_str(), &p));

    return (*p == 0);
}

bool isTime(const std::string & s) {
	return (srch2is::DateAndTimeHandler::verifyDateTimeString(s,srch2is::DateTimeTypeNow) ||
			srch2is::DateAndTimeHandler::verifyDateTimeString(s,srch2is::DateTimeTypePointOfTime) ||
			srch2is::DateAndTimeHandler::verifyDateTimeString(s,srch2is::DateTimeTypeDurationOfTime) );
}

// convert other types to string
template<class T>
string convertToStr(T value) {
    std::ostringstream o;
    if (!(o << value))
        return "";
    return o.str();
}

//void custom_evhttp_find_headers(const struct evkeyvalq *headers,
//        const char *key, vector<string> &values) {
//    struct evkeyval *header;
//    int c = 0;
//    TAILQ_FOREACH(header, headers, next)
//    {
//        if (evutil_ascii_strcasecmp(header->key, key) == 0) {
//            ++c;
//            values.push_back(header->value);
//        }
//    }
//}
bool doParse(string &input, const boost::regex &re, string &output) {
    boost::smatch matches;
    boost::regex_search(input, matches, re);
    if (matches[0].matched) {
        output = input.substr(matches.position(), matches.length());
        boost::algorithm::trim(output);
        input = input.substr(matches.position() + matches.length());
        boost::algorithm::trim(input);
        // string logMsg = "remove " + output + ", input modified to: "
        //        + input;
        Logger::debug("Remove %s,input modified to: %s", output.c_str(),
                input.c_str());
        //Logger::debug(logMsg.c_str());
        return true;
    } else {
        return false;
    }
}

bool validateValueWithType(srch2::instantsearch::FilterType type,
        string & value) {
    switch (type) {
    case srch2::instantsearch::ATTRIBUTE_TYPE_INT:
    case srch2::instantsearch::ATTRIBUTE_TYPE_LONG:
        return isInteger(value);
    case srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT:
    case srch2::instantsearch::ATTRIBUTE_TYPE_DOUBLE:
        return isFloat(value);
    case srch2::instantsearch::ATTRIBUTE_TYPE_TEXT:
        return true;
    case srch2::instantsearch::ATTRIBUTE_TYPE_TIME:
    case srch2::instantsearch::ATTRIBUTE_TYPE_DURATION:
        return isTime(value);
	default:
		break;
    }
    return false;
}

}
}
