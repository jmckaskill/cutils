#pragma once
#include <stdint.h>

struct timer {
	uint64_t a, b;
};

void start_timer(struct timer *t);
double restart_timer(struct timer *t);
double stop_timer(struct timer *t);
