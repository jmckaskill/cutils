#include <cutils/stopwatch.h>

static int compare_stopwatch(const struct heap_node *a, const struct heap_node *b) {
	const wakeup_t *sa = container_of(a, struct wakeup, hn);
	const wakeup_t *sb = container_of(b, struct wakeup, hn);
	tickdiff_t diff = (tickdiff_t)(sa->wakeup - sb->wakeup);
	return diff < 0;
}

void init_stopwatch(stopwatch_t *w) {
	heap_init(&w->h, &compare_stopwatch);
}

tickdiff_t dispatch_stopwatch(stopwatch_t *s, tick_t now) {
	while (s->h.head) {
		wakeup_t *w = container_of(s->h.head, wakeup_t, hn);
		tick_t wakeup = w->wakeup;
		tickdiff_t d = (tickdiff_t)(wakeup - now);
		if (d > 0) {
			return d;
		}
		w->fn(w, now);
		if (w->wakeup == wakeup) {
			rm_wakeup(s, w);
		}
	}

	return (tickdiff_t)-1;
}

void add_wakeup(stopwatch_t *s, wakeup_t *w, tick_t wakeup, wakeup_fn fn) {
	if (w->fn == fn && wakeup > w->wakeup) {
		w->wakeup = wakeup;
		heap_update(&s->h, &w->hn);
	} else {
		if (w->fn) {
			rm_wakeup(s, w);
		}
		w->fn = fn;
		w->wakeup = wakeup;
		heap_insert(&s->h, &w->hn);
	}
}

void rm_wakeup(stopwatch_t *s, wakeup_t *w) {
	if (w->fn) {
		w->fn = NULL;
		heap_remove(&s->h, &w->hn);
	}
}


