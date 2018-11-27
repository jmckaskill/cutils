#include <cutils/apc.h>

static int compare_stopwatch(const struct heap_node *a, const struct heap_node *b) {
	const apc_t *sa = container_of(a, struct apc, hn);
	const apc_t *sb = container_of(b, struct apc, hn);
	tickdiff_t diff = (tickdiff_t)(sa->wakeup - sb->wakeup);
	return diff < 0;
}

static void init_dispatcher(dispatcher_t *s) {
	heap_init(&s->h, &compare_stopwatch);
	s->apcs.left = &s->apcs;
	s->apcs.right = &s->apcs;
	s->dispatching = NULL;
}

int dispatch_apcs(dispatcher_t *s, tick_t now, int divisor) {
	if (!s->h.before) {
		init_dispatcher(s);
	}
	while (s->apcs.left != &s->apcs) {
		apc_t *w = container_of(s->apcs.left, apc_t, hn);
		s->dispatching = w;
		w->fn(w, now);
		if (s->dispatching == w) {
			cancel_apc(s, w);
		}
	}

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
	if (!d->h.before) {
		init_dispatcher(d);
	}
	if (d->dispatching == a) {
		d->dispatching = NULL;
	}
	if (a->hn.parent || d->h.head == &a->hn) {
		heap_remove(&d->h, &a->hn);
	} else if (!a->hn.left) {
		a->hn.right = &d->apcs;
		a->hn.left = d->apcs.left;
		a->hn.left->right = &a->hn;
		a->hn.right->left = &a->hn;
		a->hn.parent = NULL;
	}
	a->fn = fn;
}

void add_timed_apc(dispatcher_t *d, apc_t *a, tick_t wakeup, wakeup_fn fn) {
	if (!d->h.before) {
		init_dispatcher(d);
	}
	if (a->fn && (a->hn.parent || d->h.head == &a->hn) && (tickdiff_t)(wakeup - a->wakeup) >= 0) {
		if (d->dispatching == a) {
			d->dispatching = NULL;
		}
		a->wakeup = wakeup;
		heap_update(&d->h, &a->hn);
	} else {
		cancel_apc(d, a);
		a->wakeup = wakeup;
		heap_insert(&d->h, &a->hn);
	}
	a->fn = fn;
}

void move_apc(dispatcher_t *od, dispatcher_t *nd, apc_t *a) {
	if (!od->h.before) {
		return;
	}
	if (!nd->h.before) {
		init_dispatcher(nd);
	}
	if (od->dispatching == a) {
		nd->dispatching = NULL;
	}
	if (a->hn.parent || od->h.head == &a->hn) {
		heap_remove(&od->h, &a->hn);
		heap_insert(&nd->h, &a->hn);
	} else if (a->hn.left) {
		a->hn.left->right = a->hn.right;
		a->hn.right->left = a->hn.left;
		a->hn.right = &nd->apcs;
		a->hn.left = nd->apcs.left;
		a->hn.right->left = &a->hn;
		a->hn.left->right = &a->hn;
	}
}

void cancel_apc(dispatcher_t *d, apc_t *a) {
	if (a->fn != NULL) {
		if (!d->h.before) {
			init_dispatcher(d);
		}
		if (d->dispatching == a) {
			d->dispatching = NULL;
		}
		if (a->hn.parent || d->h.head == &a->hn) {
			heap_remove(&d->h, &a->hn);
		} else if (a->hn.left) {
			a->hn.left->right = a->hn.right;
			a->hn.right->left = a->hn.left;
		}
		a->fn = NULL;
	}
}


