#pragma once

#include "heap.h"
#include <stdint.h>

// tick can be any resolution depending on the use case
// the app can trade off resolution for max timer length
// common resolutions include:
// 1 ns -> max length = 2.1 s
// 1 us -> max length = 35.7 min
// 1 ms -> max length = 24.8 days

typedef unsigned long tick_t;
typedef long tickdiff_t;
typedef struct wakeup wakeup_t;
typedef struct stopwatch stopwatch_t;
typedef void(*wakeup_fn)(wakeup_t *s, tick_t now);

struct stopwatch {
	struct heap h;
};

struct wakeup {
	struct heap_node hn;
	wakeup_fn fn;
	tick_t wakeup;
};

void add_wakeup(stopwatch_t *set, wakeup_t *s, tick_t wakeup, wakeup_fn fn);
void rm_wakeup(stopwatch_t *set, wakeup_t *s);

void init_stopwatch(stopwatch_t *set);
tickdiff_t dispatch_stopwatch(stopwatch_t *set, tick_t now);





