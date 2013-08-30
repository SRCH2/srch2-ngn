#include "ParserUtility.h"
#include <string>
#include <cstdlib>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>

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

std::vector<std::string> &split(std::string &s, std::string delimiter) {
    std::vector<std::string> result;
    std::string source = s;

    std::string::size_type lastpos = source.find(delimiter, 0);

    if (lastpos == std::string::npos) {
        result.push_back(source);
        return result;
    }
    if (lastpos != 0) {
        result.push_back(source.substr(0, lastpos));
        source = source.substr(lastpos, source.size() - lastpos);
    }
    // Skip delimiters at beginning.
    lastpos = lastpos + delimiter.size();
    // Find first "non-delimiter".
    std::string::size_type pos = source.find(delimiter, lastpos);

    while (std::string::npos != pos && std::string::npos != lastpos) {
        // Found a token, add it to the vector.
        result.push_back(source.substr(lastpos, pos - lastpos));
        // Skip delimiters.  Note the "not_of"
        lastpos = lastpos + delimiter.size();
        // Find next "non-delimiter"
        pos = source.find(delimiter, lastpos);
    }
    return result;

}

// source : http://stackoverflow.com/questions/2844817/how-do-i-check-if-a-c-string-is-an-int
bool isInteger(const std::string & s) {
    if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
        return false;

    char * p;
    strtol(s.c_str(), &p, 10);

    return (*p == 0);
}

bool isUnsigned(const std::string & s) {
    if (s.empty() || ((!isdigit(s[0])) && (s[0] != '+')))
        return false;
    char * p;
    strtol(s.c_str(), &p, 10);
    return (*p == 0);
}

bool isFloat(const std::string & s) {
    if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
        return false;

    char * p;
    strtof(s.c_str(), &p);

    return (*p == 0);
}

bool isTime(const std::string & s) {
    if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
        return false;

    char * p;
    strtol(s.c_str(), &p, 10);

    return (*p == 0);
}

time_t convertPtimeToTimeT(boost::posix_time::ptime t) {
    static boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    return (t - epoch).ticks()
            / boost::posix_time::time_duration::ticks_per_second();
}

// convert other types to string
template<class T>
string convertToStr(T value) {
    std::ostringstream o;
    if (!(o << value))
        return "";
    return o.str();
}

std::string convertTimeFormatToLong(std::string & timeString) {
    std::string stringValue = "";
    for (size_t i = 0; i < localeFormats; ++i) {
        std::istringstream ss(timeString);
        ss.imbue(localeInputs[i]);
        boost::posix_time::ptime this_time;
        ss >> this_time;

        if (this_time != boost::posix_time::not_a_date_time) {
            time_t value = srch2::httpwrapper::convertPtimeToTimeT(this_time);
            long valueLong = value;
            std::ostringstream o;
            o << valueLong;
            stringValue = o.str();
        }

    }
    return stringValue;

}
std::string convertLongToTimeFormat(std::string & timeLong) {
    static boost::posix_time::ptime empch;
}
void custom_evhttp_find_headers(const struct evkeyvalq *headers,
        const char *key, vector<string> &values) {
    struct evkeyval *header;
    int c = 0;
    TAILQ_FOREACH(header, headers, next)
    {
        if (evutil_ascii_strcasecmp(header->key, key) == 0) {
            ++c;
            values.push_back(header->value);
        }
    }
}
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
    case srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED:
        return isInteger(value);
    case srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT:
        return isFloat(value);
    case srch2::instantsearch::ATTRIBUTE_TYPE_TEXT:
        return true; // TEXT does not have any criteria ?????
    case srch2::instantsearch::ATTRIBUTE_TYPE_TIME:
        return isTime(value);
    }
    // flow never reaches here
    return false;
}

}
}
