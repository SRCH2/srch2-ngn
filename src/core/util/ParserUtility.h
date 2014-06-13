#ifndef __WRAPPER_PARSERUTILITY_H__
#define __WRAPPER_PARSERUTILITY_H__

#include <string>
#include <cstdlib>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sys/queue.h>
#include "boost/regex.hpp"
#include <boost/algorithm/string.hpp>
#include "src/core/util/Logger.h"
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


//void custom_evhttp_find_headers(const struct evkeyvalq *headers,
//        const char *key, vector<string> &values);
/*
 * parses using the given regex
 */
bool doParse(string &input, const boost::regex &re, string &output);

//void custom_evhttp_find_headers(const struct evkeyvalq *headers,
//        const char *key, vector<string> &values);

bool validateValueWithType(srch2::instantsearch::FilterType type,
        string & value);
}
}

#endif // __WRAPPER_PARSERUTILITY_H__
