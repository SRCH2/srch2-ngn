//$Id: ResultsPostProcessor.h 3456 2013-06-26 02:11:13Z Jamshid $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */
#ifndef __WRAPPER_DATEANDTIMEHANDLER_H__
#define __WRAPPER_DATEANDTIMEHANDLER_H__


#include <string>
#include <cstdlib>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <time.h>
#include <limits.h>
#include <sstream>
#include <map>

#include "WrapperConstants.h"

using namespace std;

using boost::posix_time::time_input_facet;
using std::locale;

namespace bt = boost::posix_time;

namespace srch2 {
namespace httpwrapper {

class DateAndTimeHandler{
public:
    static time_t convertDateTimeStringToSecondsFromEpoch(const string & timeString);
    static string convertSecondsFromEpochToDateTimeString(register const time_t * secondsFromEpoch);
    static bool verifyDateTimeString(const string & timeString, DateTimeType dateTimeType);

private:

    const static locale localeInputsPointOfTime[] ;
    const static locale localeInputsDurationOfTime[] ;
    const static size_t localeFormatsPointOfTime ;
    const static size_t localeFormatsDurationOfTime ;

    const static map<string, time_t> DURATION_OF_TIME_CONSTANTS;

    static time_t convertPtimeToTimeT(boost::posix_time::ptime t);
    static map<string, time_t> initializeConstants();

};

}
}



#endif // __WRAPPER_DATEANDTIMEHANDLER_H__
