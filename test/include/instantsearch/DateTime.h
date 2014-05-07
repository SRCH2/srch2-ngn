
// $Id: Score.h 2013-06-19 02:11:13Z Jamshid $

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

#ifndef __INSTANTSEARCH_DATETIME_H__
#define __INSTANTSEARCH_DATETIME_H__


#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <vector>

#include "instantsearch/Constants.h"

using namespace std;

namespace srch2
{
namespace instantsearch
{


/*
 * This class handles different kinds of duration in time.
 */
class TimeDuration{
public:
	boost::posix_time::time_duration secondMinuteHourDuration;
	boost::gregorian::date_duration dayWeekDuration;
	boost::gregorian::months monthDuration;
	boost::gregorian::years yearDuration;

        TimeDuration(): dayWeekDuration(0), monthDuration(0),yearDuration(0){}

	boost::posix_time::ptime operator+(const boost::posix_time::ptime & pTime) const;
	long operator+(const long pTime) const;
	TimeDuration operator+(const TimeDuration & timeDuration) const;

	string toString() const;

	unsigned getNumberOfBytes() {
		return sizeof(yearDuration) + sizeof(monthDuration) + sizeof(dayWeekDuration)
				+ sizeof(secondMinuteHourDuration);
	}
};


}
}


#endif // __INSTANTSEARCH_DATETIME_H__
