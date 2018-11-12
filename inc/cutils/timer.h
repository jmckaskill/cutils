#pragma once
#include <stdint.h>

struct timer {
	uint64_t a;
};

void start_timer(struct timer *t);
double restart_timer(struct timer *t);
double stop_timer(struct timer *t);

uint64_t monotonic_nanoseconds(void);
