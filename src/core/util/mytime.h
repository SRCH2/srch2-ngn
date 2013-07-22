/*
 * mytime.h
 *
 *  Created on: Jun 11, 2013
 *      Author: sina
 */

#ifndef MYTIME_H_
#define MYTIME_H_

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#else
#include <time.h>
#endif


#ifdef __MACH__
#define CLOCK_REALTIME 1

inline int clock_gettime(int clk_id, struct timespec *tp){
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	tp->tv_sec = mts.tv_sec;
	tp->tv_nsec = mts.tv_nsec;
	return 1;
}

#endif

#endif /* MYTIME_H_ */
