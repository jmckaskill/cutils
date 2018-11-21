#define _POSIX_C_SOURCE 199309L
#include "cutils/timer.h"
#include <stdint.h>
#include <stddef.h>

#ifdef WIN32
#include <windows.h>

void start_timer(struct timer *t) {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	t->a = li.QuadPart;
}
double stop_timer(struct timer *t) {
	LARGE_INTEGER stop_time;
	QueryPerformanceCounter(&stop_time);
	uint64_t delta = stop_time.QuadPart - t->a;
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return (double)delta / (double)freq.QuadPart;
}
uint64_t monotonic_ns(void) {
	uint64_t ms = GetTickCount64();
	return ms * 1000 * 1000;
}

uint64_t utc_us(void) {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	LARGE_INTEGER li;
	li.HighPart = ft.dwHighDateTime;
	li.LowPart = ft.dwLowDateTime;
	return (li.QuadPart / 10) - UINT64_C(11644473600000000);
}

#elif defined __linux__
#include <time.h>
#include <sys/time.h>

void start_timer(struct timer *t) {
	t->a = monotonic_ns();
}

double stop_timer(struct timer *t) {
	uint64_t delta = monotonic_ns() - t->a;
	return (double)delta / 1e9;
}

uint64_t monotonic_ns(void) {
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	return ((uint64_t)(tv.tv_sec) * 1000 * 1000 * 1000) + tv.tv_nsec;
}

#elif defined __APPLE__
#include <mach/mach_time.h>
#include <sys/time.h>
#include <time.h>

static mach_timebase_info_data_t g_timebase_info;

void start_timer(struct timer *t) {
	t->a = mach_absolute_time();
}

double stop_timer(struct timer *t) {
	uint64_t end = mach_absolute_time();
	if (g_timebase_info.denom == 0) {
		mach_timebase_info(&g_timebase_info);
	}
	uint64_t delta = end - t->a;
	return (double) delta * g_timebase_info.numer / (double) g_timebase_info.denom / 1e9;
}
uint64_t monotonic_ns(void) {
	uint64_t ticks = mach_absolute_time();
	if (g_timebase_info.denom == 0) {
		mach_timebase_info(&g_timebase_info);
	}
	double ns = ((double)ticks * g_timebase_info.numer) / g_timebase_info.denom;
	return (uint64_t)ns;
}


#else
#error
#endif

#ifndef WIN32
uint64_t utc_us(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint64_t sec = (uint64_t)tv.tv_sec * 1000 * 1000;
	uint64_t us = (uint64_t)tv.tv_usec;
	return sec + us;
}
#endif

double restart_timer(struct timer *t) {
	double ret = stop_timer(t);
	start_timer(t);
	return ret;
}

