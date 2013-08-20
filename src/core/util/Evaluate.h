//$Id$

#ifndef __CORE_UTIL_EVALUATE_H__
#define __CORE_UTIL_EVALUATE_H___

#include <ctime>

namespace srch2{
namespace util{

/**
 * Record the current time
 */
inline void setStartTime( timespec* startTime){
    clock_gettime(CLOCK_REALTIME, startTime);
}

/**
 * @return: the time passed from startTime to "now" in milliseconds
 */
inline double getTimeSpan(const timespec &startTime){
    timespec endTime;
    clock_gettime(CLOCK_REALTIME, &endTime);
    return (double)((endTime.tv_sec - (startTime).tv_sec) * 1000.0)
        + (double)(endTime.tv_nsec - (startTime).tv_nsec) / 1000000.0;
}

/**
 * @return: the RAM usage at current time, the value is in KB
 */
int getRAMUsageValue();

}
}

#endif
