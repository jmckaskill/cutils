#define _POSIX_C_SOURCE 199309L
#include "cutils/timer.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>

// computes (a * mul) / div using a 96 bit intermediate
static uint64_t muldiv64(uint32_t alo, uint32_t ahi, uint32_t mul, uint64_t div) {
	uint64_t plo = (uint64_t)alo * mul;
	uint64_t phi = (uint64_t)ahi * mul;
	phi += plo >> 32;
	plo &= ~UINT32_C(0);
	uint64_t dhi = phi / div;
	uint64_t rhi = phi % div;
	uint64_t dlo = ((rhi << 32) + plo) / div;
	return (dhi << 32) | (dlo & ~UINT32_C(0));
}

#ifdef WIN32
#include <windows.h>

uint64_t monotonic_ns(void) {
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	return muldiv64(count.LowPart, count.HighPart, 1000 * 1000 * 1000, freq.QuadPart);
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

uint64_t monotonic_ns(void) {
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	return ((uint64_t)(tv.tv_sec) * 1000 * 1000 * 1000) + tv.tv_nsec;
}

#elif defined __APPLE__
#include <mach/mach_time.h>
#include <sys/time.h>
#include <time.h>

uint64_t monotonic_ns(void) {
	struct mach_timebase_info timebase;
	mach_timebase_info(&timebase);
	uint64_t ticks = mach_absolute_time();
	return muldiv64((uint32_t)ticks, (uint32_t)(ticks >> 32), timebase.numer, timebase.denom);
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

