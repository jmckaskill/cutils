#include <cutils/apc.h>
#include <assert.h>

static int compare_stopwatch(const struct heap_node *a, const struct heap_node *b) {
	const apc_t *sa = container_of(a, struct apc, hn);
	const apc_t *sb = container_of(b, struct apc, hn);
	tickdiff_t diff = (tickdiff_t)(sa->wakeup - sb->wakeup);
	return diff < 0;
}

void init_dispatcher(dispatcher_t *s, tick_t now) {
	heap_init(&s->h, &compare_stopwatch);
	s->dispatching = NULL;
	s->last_tick = now;
}

int dispatch_apcs(dispatcher_t *s, tick_t now, int divisor) {
	assert(s->h.before);
	while (s->h.head) {
		apc_t *w = container_of(s->h.head, apc_t, hn);
		tick_t wakeup = w->wakeup;
		tickdiff_t d = (tickdiff_t)(wakeup - now);
		if (d > 0) {
			return (d + divisor - 1) / divisor;
		}
		s->dispatching = w;
		w->fn(w, now);
		if (s->dispatching == w) {
			cancel_apc(s, w);
		}
	}
	return -1;
}

void add_apc(dispatcher_t *d, apc_t *a, wakeup_fn fn) {
	add_timed_apc(d, a, d->last_tick, fn);
}

void add_timed_apc(dispatcher_t *d, apc_t *a, tick_t wakeup, wakeup_fn fn) {
	assert(d->h.before);
	if (d->dispatching == a) {
		d->dispatching = NULL;
	}
	if (a->fn && (tickdiff_t)(wakeup - a->wakeup) >= 0) {
		a->wakeup = wakeup;
		heap_update(&d->h, &a->hn);
	} else {
		heap_remove(&d->h, &a->hn);
		a->wakeup = wakeup;
		heap_insert(&d->h, &a->hn);
	}
	a->fn = fn;
}

void move_apc(dispatcher_t *od, dispatcher_t *nd, apc_t *a) {
	assert(od->h.before && nd->h.before);
	if (od->dispatching == a) {
		nd->dispatching = NULL;
	}
	heap_remove(&od->h, &a->hn);
	heap_insert(&nd->h, &a->hn);
}

void cancel_apc(dispatcher_t *d, apc_t *a) {
	if (a->fn != NULL) {
		if (d->dispatching == a) {
			d->dispatching = NULL;
		}
		heap_remove(&d->h, &a->hn);
		a->fn = NULL;
	}
}


