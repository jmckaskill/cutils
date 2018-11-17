#pragma once
#include <stdint.h>

struct timer {
	uint64_t a;
};

void start_timer(struct timer *t);
double restart_timer(struct timer *t);
double stop_timer(struct timer *t);

uint64_t monotonic_ns(void);
uint64_t utc_us(int *tzoffmin);
