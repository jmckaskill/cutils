#pragma once

#include "heap.h"
#include <stdint.h>
#include <stdbool.h>

// tick can be any resolution depending on the use case
// the app can trade off resolution for max timer length
// common resolutions include:
// 1 ns -> max length = 2.1 s
// 1 us -> max length = 35.7 min
// 1 ms -> max length = 24.8 days

typedef uint32_t tick_t;
typedef int32_t tickdiff_t;
typedef struct apc apc_t;
typedef struct dispatcher dispatcher_t;
typedef void(*wakeup_fn)(apc_t *s, tick_t now);

struct dispatcher {
	struct heap h;
	apc_t *dispatching;
	tick_t last_tick;
};

struct apc {
	struct heap_node hn;
	wakeup_fn fn;
	tick_t wakeup;
};

void init_dispatcher(dispatcher_t *s, tick_t now);
void add_apc(dispatcher_t *s, apc_t *w, wakeup_fn fn);
void add_timed_apc(dispatcher_t *s, apc_t *w, tick_t wakeup, wakeup_fn fn);
void cancel_apc(dispatcher_t *s, apc_t *w);
void move_apc(dispatcher_t *od, dispatcher_t *nd, apc_t *a);
static inline bool is_apc_active(apc_t *a) {return a->fn != NULL;}

int dispatch_apcs(dispatcher_t *set, tick_t now, int timeout_divisor);





