#include <cutils/apc.h>
#include <assert.h>

static int compare_apc(const struct heap_node *a, const struct heap_node *b) {
	const apc_t *sa = container_of(a, struct apc, hn);
	const apc_t *sb = container_of(b, struct apc, hn);
	tickdiff_t diff = (tickdiff_t)(sa->wakeup - sb->wakeup);
	return diff < 0;
}

void init_dispatcher(dispatcher_t *d, tick_t now) {
	heap_init(&d->h, &compare_apc);
	d->dispatching = NULL;
	d->last_tick = now;
}

int dispatch_apcs(dispatcher_t *d, tick_t now, tickdiff_t sleep_granularity) {
	assert(d->h.before);
	assert(!d->dispatching);
	d->last_tick = now;
	while (d->h.head) {
		apc_t *a = container_of(d->h.head, apc_t, hn);
		tick_t wakeup = a->wakeup;
		tickdiff_t t = (tickdiff_t)(wakeup - now);
		if (t > sleep_granularity/2) {
			return (t + (sleep_granularity/2)) / sleep_granularity;
		}
		// Cancel the apc, but don't remove from the heap yet
		// This allows add_timed_apc to be efficient if called
		// from the callback which is very common for periodic timers;
		wakeup_fn fn = a->fn;
		a->fn = NULL;
		d->dispatching = a;
		fn(a, now);
		a = d->dispatching;
		d->dispatching = NULL;
		if (a && a->fn == NULL) {
			heap_remove(&d->h, &a->hn);
		}
	}
	return -1;
}

void add_apc(dispatcher_t *d, apc_t *a, wakeup_fn fn) {
	add_timed_apc(d, a, d->last_tick, fn);
}

void add_timed_apc(dispatcher_t *d, apc_t *a, tick_t wakeup, wakeup_fn fn) {
	assert(d->h.before && fn != NULL);
	tickdiff_t diff = (tickdiff_t)(wakeup - a->wakeup);
	a->wakeup = wakeup;
	if (!a->hn.parent && d->h.head != &a->hn) {
		// apc is not in the heap
		heap_insert(&d->h, &a->hn);
	} else if (diff > 0) {
		// apc is in the heap and can be pushed back
		heap_update(&d->h, &a->hn);
	} else if (diff < 0) {
		// apc is in the heap but must be pulled forward
		heap_remove(&d->h, &a->hn);
		heap_insert(&d->h, &a->hn);
	}
	a->fn = fn;
}

void move_apc(dispatcher_t *od, dispatcher_t *nd, apc_t *a) {
	assert(od->h.before && nd->h.before);
	heap_remove(&od->h, &a->hn);
	if (is_apc_active(a)) {
		heap_insert(&nd->h, &a->hn);
	}
}

void cancel_apc(dispatcher_t *d, apc_t *a) {
	if (d->dispatching == a) {
		d->dispatching = NULL;
	}
	heap_remove(&d->h, &a->hn);
	a->fn = NULL;
}


