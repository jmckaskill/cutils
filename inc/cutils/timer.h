#pragma once
#include <stdint.h>


uint64_t monotonic_ns(void);
uint64_t utc_us(void);

struct timer {
	uint64_t start;
};
static inline void start_timer(struct timer *t) {
	t->start = monotonic_ns();
}
static inline double stop_timer(struct timer *t) {
	return (double)(monotonic_ns() - t->start) / 1e9;
}
static inline double restart_timer(struct timer *t) {
	double ret = stop_timer(t);
	start_timer(t);
	return ret;
}
