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
