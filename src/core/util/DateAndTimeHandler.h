#ifndef __WRAPPER_DATEANDTIMEHANDLER_H__
#define __WRAPPER_DATEANDTIMEHANDLER_H__

#include "instantsearch/Constants.h"
#include "instantsearch/TypedValue.h"

#include <string>
#include <cstdlib>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <time.h>
#include <limits.h>
#include <sstream>
#include <vector>

using namespace std;

using boost::posix_time::time_input_facet;
using std::locale;
namespace bt = boost::posix_time;

namespace srch2 {
namespace instantsearch {

class DateAndTimeHandler{
public:
    static time_t convertDateTimeStringToSecondsFromEpoch(const string & timeString);
    static string convertSecondsFromEpochToDateTimeString(register const time_t * secondsFromEpoch);
    static bool verifyDateTimeString(const string & timeString, DateTimeType dateTimeType);
    static time_t convertPtimeToSecondsFromEpoch(boost::posix_time::ptime t);
    static boost::posix_time::ptime convertSecondsFromEpochToPTime(time_t t);
    static TimeDuration convertDurationTimeStringToTimeDurationObject(const string & timeString);

private:
    const static string regexInputsPointOfTime[];
    const static locale localeInputsPointOfTime[] ;
    const static string regexInputsDurationOfTime[];
    const static locale localeInputsDurationOfTime[] ;
    const static size_t localeFormatsPointOfTime ;
    const static size_t localeFormatsDurationOfTime ;
    const static vector<string> DURATION_OF_TIME_CONSTANTS;

    static vector<string> initializeConstants();

};


}
}



#endif // __WRAPPER_DATEANDTIMEHANDLER_H__
