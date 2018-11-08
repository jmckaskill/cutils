#define _POSIX_C_SOURCE 199309L
#include "cutils/timer.h"
#include <stdint.h>

#ifdef WIN32
#include <windows.h>

void start_timer(struct timer *t) {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	t->a = li.QuadPart;
	t->b = 0;
}
double stop_timer(struct timer *t) {
	LARGE_INTEGER stop_time;
	QueryPerformanceCounter(&stop_time);
	uint64_t delta = stop_time.QuadPart - t->a;
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return (double)delta / (double)freq.QuadPart;
}

#elif defined __linux__
#include <time.h>

void start_timer(struct timer *t) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	t->a = ts.tv_sec;
	t->b = ts.tv_nsec;
}

double stop_timer(struct timer *t) {
	struct timespec stop_time;
	clock_gettime(CLOCK_MONOTONIC, &stop_time);
	int64_t delta_sec = stop_time.tv_sec - t->a;
	int64_t delta_ns = stop_time.tv_nsec - t->b;
	return (double)delta_sec + (double)delta_ns / 1e9;
}

#elif defined __APPLE__

#include <mach/mach_time.h>

void start_timer(struct timer *t) {
	t->a = mach_absolute_time();
}

double stop_timer(struct timer *t) {
	uint64_t end = mach_absolute_time();
	mach_timebase_info_data_t rate_nsec;
	mach_timebase_info(&rate_nsec);
	uint64_t delta = end - t->a;
	return (double) delta * rate_nsec.numer / (double) rate_nsec.denom / 1e9;
}

#else
#error
#endif

double restart_timer(struct timer *t) {
	double ret = stop_timer(t);
	start_timer(t);
	return ret;
}
