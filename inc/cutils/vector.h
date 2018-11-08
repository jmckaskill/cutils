#pragma once
#include <stdlib.h>
#include <string.h>

typedef struct allocator allocator_t;

struct allocator {
	void *(*calloc)(allocator_t*, size_t num, size_t size);
	void *(*realloc)(allocator_t*, void *, size_t);
	void (*free)(allocator_t*, void*);
};

static inline void *xcalloc(allocator_t *a, size_t num, size_t size) {
	return a ? a->calloc(a, num, size) : calloc(num, size);
}
static inline void *xrealloc(allocator_t *a, void *p, size_t size) {
	return a ? a->realloc(a, p, size) : realloc(p, size);
}
static inline void *xmalloc(allocator_t *a, size_t size) {
	return a ? a->realloc(a, NULL, size) : malloc(size);
}
static inline void xfree(allocator_t *a, void *p) {
	a ? a->free(a, p) : free(p);
}

struct vector {
	void *data;
	size_t size, cap;
};

void *grow_vector(allocator_t *a, struct vector *v, size_t objsz, size_t num);
void *grow_append_vector(allocator_t *a, struct vector *v, size_t objsz, size_t num);

static inline void *append_vector(allocator_t *a, struct vector *v, size_t objsz, size_t num) {
	size_t newsz = v->size + num;
	if (newsz <= v->cap) {
		char *ret = (char*)v->data + objsz * v->size;
		v->size = newsz;
		return ret;
	} else {
		return grow_append_vector(a, v, objsz, num);
	}
}

static inline void *append_vector_zeroed(allocator_t *a, struct vector *v, size_t objsz, size_t num) {
	void *ret = append_vector(a, v, objsz, num);
	if (ret) {
		memset(ret, 0, objsz * num);
	}
	return ret;
}

#define FIRST(V) ((V)->v[0])
#define LAST(V) ((V)->v[(V)->size - 1])
#define APPEND2(V,N) append_vector(NULL, (struct vector*) (V), sizeof((V)->v[0]), (N))
#define APPEND(V) APPEND2((V), 1)
#define APPEND2_ZERO(V,N) append_vector_zeroed(NULL, (struct vector*) (V), sizeof((V)->v[0]), (N))
#define APPEND_ZERO(V) APPEND2_ZERO((V), 1)
#define APPEND_ALLOC(V,A) append_vector((A), (struct vector*)(V), sizeof((V)->v[0]), 1)
#define GROW_VECTOR(V, N) grow_vector(NULL, (struct vector*) (V), sizeof((V)->v[0]), (N))
#define VECTOR_MEMORY(V) ((V)->cap * sizeof((V)->v[0]))
#define SORT_VECTOR(V,FN) qsort((V)->v, (V)->size, sizeof((V)->v[0]), (FN))
#define BSEARCH_VECTOR(V,KEY,FN) bsearch(&(KEY),(V)->v,(V)->size, sizeof((V)->v[0]), (FN))

#define NEW(TYPE) ((TYPE*)calloc(1, sizeof(TYPE)))

struct bitset {
	unsigned char *v;
	size_t bytes, cap;
};

int set_bitset_size(allocator_t *a, struct bitset *v, size_t size);

static inline void set_bitset(struct bitset *v, size_t idx) {
	v->v[idx >> 3] |= (1U << (idx & 7U));
}
static inline void clear_bitset(struct bitset *v, size_t idx) {
	v->v[idx >> 3] &= ~(1U << (idx & 7U));
}
static inline int test_bitset(struct bitset *v, size_t idx) {
	return (v->v[idx >> 3] & (1U << (idx & 7U))) != 0;
}

