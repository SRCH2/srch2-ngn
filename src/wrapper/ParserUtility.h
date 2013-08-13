#ifndef _WRAPPER_PARSERUTILITY_H_
#define _WRAPPER_PARSERUTILITY_H_

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

using boost::posix_time::time_input_facet;
using std::locale;

using namespace std;
using srch2::util::Logger;
namespace srch2 {
namespace httpwrapper {

// trim from start
std::string &ltrim(std::string &s);

// trim from end
std::string &rtrim(std::string &s);

// trim from both ends
std::string &trim(std::string &s);

std::string &removeWhiteSpace(std::string &s);

std::vector<std::string> &split(std::string &s, std::string delimiter);

bool isInteger(const std::string & s);

bool isUnsigned(const std::string & s);

bool isFloat(const std::string & s);

bool isTime(const std::string & s);

const locale localeInputs[] = { locale(locale::classic(),
        new time_input_facet("%m/%d/%Y")), locale(locale::classic(),
        new time_input_facet("%Y-%m-%d %H:%M:%S")), locale(locale::classic(),
        new time_input_facet("%Y%m%d%H%M%S")), locale(locale::classic(),
        new time_input_facet("%Y%m%d%H%M")), locale(locale::classic(),
        new time_input_facet("%Y%m%d")) };
const size_t localeFormats = sizeof(localeInputs) / sizeof(localeInputs[0]);

time_t convertPtimeToTimeT(boost::posix_time::ptime t);
void custom_evhttp_find_headers(const struct evkeyvalq *headers,
        const char *key, vector<string> &values);
/*
 * parses using the given regex
 */
bool doParse(string &input, const boost::regex &re, string &output);
}
}

#endif // _WRAPPER_PARSERUTILITY_H_
