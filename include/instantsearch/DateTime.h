
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
