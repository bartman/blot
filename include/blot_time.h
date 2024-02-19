/* blot: various time utilities */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <time.h>

#define BLOT_MSEC_PER_SEC 1000ull
#define BLOT_USEC_PER_SEC 1000000ull
#define BLOT_NSEC_PER_SEC 1000000000ull

#define BLOT_USEC_PER_MSEC 1000ull
#define BLOT_NSEC_PER_MSEC 1000000ull

#define BLOT_NSEC_PER_USEC 1000ull

static inline double blot_double_time(void)
{
	struct timespec ts;
	double sec;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	sec = (double)ts.tv_sec
	    + (double)ts.tv_nsec / BLOT_NSEC_PER_SEC;

	return sec;
}

