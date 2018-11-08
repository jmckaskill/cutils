#pragma once
#include <stddef.h>

typedef struct heap_node heap_node;
typedef struct heap heap;

struct heap_node {
	struct heap_node *left;
	struct heap_node *right;
	struct heap_node *parent;
};

// returns non zero if a should come out of the heap before b
typedef int (*heap_before)(const struct heap_node *a, const struct heap_node *b);

struct heap {
	struct heap_node *head;
	int size;
	heap_before before;
};

#define HEAP_INIT(BEFORE) {NULL, 0, BEFORE}

static inline void heap_init(struct heap *h, heap_before before) {
	h->head = NULL;
	h->size = 0;
	h->before = before;
}

void heap_insert(struct heap *h, struct heap_node *n);
void heap_remove(struct heap *h, struct heap_node *n);

// heap_update is called after updating a given node's data that
// may move it further behind. This is more efficient then removing
// and reinserting. To move a node further ahead, you need
// to remove and reinsert it.
void heap_update(struct heap *h, struct heap_node *n);


#ifndef container_of
#define container_of(ptr, type, member) ((type*) ((char*) (ptr) - offsetof(type, member)))
#endif



