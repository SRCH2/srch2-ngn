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
