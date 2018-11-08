#include "cutils/heap.h"
#include "cutils/test.h"
#include "cutils/log.h"

struct int_node {
	struct heap_node hn;
	int value;
};

static int total_comparisons;
static int int_node_before(const struct heap_node *a, const struct heap_node *b) {
	struct int_node *ia = container_of(a, struct int_node, hn);
	struct int_node *ib = container_of(b, struct int_node, hn);
	total_comparisons++;
	return ia->value < ib->value;
}

static void log_node(log_t *log, const struct heap_node *n, int depth) {
	static const char spaces[] = "                          ";
	struct int_node *in = container_of(n, struct int_node, hn);
	struct int_node *pn = container_of(n->parent, struct int_node, hn);
	LOG(log, "%.*s%d (P %d)", depth*2, spaces, in->value, pn ? pn->value : 0);
	const struct heap_node *prev = n;
	for (const struct heap_node *c = n->left; c != NULL; c = c->right) {
		log_node(log, c, depth+1);
		struct int_node *cn = container_of(c, struct int_node, hn);
		EXPECT_PTREQ(prev, c->parent);
		EXPECT_GT(cn->value, in->value);
		prev = c;
	}
}

static void log_heap(log_t *log, const struct heap *h) {
	LOG(log, "heap %d", h->size);
	if (h->head) {
		log_node(log, h->head, 0);
	}
	LOG(log, "\n");
}

static void insert(log_t *log, struct heap *h, struct int_node *in) {
	LOG(log, "insert %d", in->value);
	heap_insert(h, &in->hn);
	log_heap(log, h);
}

static void update(log_t *log, struct heap *h, struct int_node *in) {
	LOG(log, "update %d", in->value);
	heap_update(h, &in->hn);
	log_heap(log, h);
}

static void remove(log_t *log, struct heap *h, struct int_node *in) {
	LOG(log, "remove %d", in->value);
	heap_remove(h, &in->hn);
	log_heap(log, h);
}

int main(int argc, const char *argv[]) {
	log_t *log = start_test(argc, argv);
	struct heap h = HEAP_INIT(&int_node_before);
	struct int_node n[30];
	for (int i = 0; i < sizeof(n)/sizeof(n[0]); i++) {
		n[i].value = i;
	}
	insert(log, &h, n+18);
	insert(log, &h, n+16);
	insert(log, &h, n+13);
	insert(log, &h, n+17);
	insert(log, &h, n+4);
	insert(log, &h, n+2);
	insert(log, &h, n+20);
	insert(log, &h, n+21);
	insert(log, &h, n+23);
	insert(log, &h, n+24);
	insert(log, &h, n+25);
	insert(log, &h, n+26);

	struct int_node u;
	u.value = 15;
	insert(log, &h, &u);
	remove(log, &h, container_of(h.head, struct int_node, hn));
	u.value = 31;
	update(log, &h, &u);

	remove(log, &h, n+13);
	while (h.size > 10) {
		remove(log, &h, container_of(h.head, struct int_node, hn));
	}
	insert(log, &h, n+27);
	insert(log, &h, n+28);
	insert(log, &h, n+29);
	insert(log, &h, n+1);
	insert(log, &h, n+5);
	insert(log, &h, n+6);
	insert(log, &h, n+7);
	insert(log, &h, n+8);
	insert(log, &h, n+9);
	insert(log, &h, n+10);
	while (h.size) {
		remove(log, &h, container_of(h.head, struct int_node, hn));
	}
	LOG(log, "total comparisons %d", total_comparisons);
	return finish_test();
}
