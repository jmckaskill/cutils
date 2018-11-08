#include "cutils/vector.h"
#include <stdlib.h>
#include <string.h>

#define TIER0 0x100 // 256B
#define TIER1 0x1000 // 4 KB
#define TIER2 0x10000 // 64 KB
#define TIER3 0x100000 // 1 MB
#define TIER4 0x1000000 // 16 MB

void *grow_vector(allocator_t *a, struct vector *v, size_t objsz, size_t num) {
	size_t newcap = v->size + num;
	if (newcap > v->cap) {
		size_t want = newcap * objsz;
		unsigned mem;
		if (want < TIER1) {
			mem = (want + TIER0 - 1) &~(TIER0 - 1);
		} else if (want < TIER2) {
			mem = (want + TIER1 - 1) &~(TIER1 - 1);
		} else if (want < TIER3) {
			mem = (want + TIER2 - 1) &~(TIER2 - 1);
		} else if (want < TIER4) {
			mem = (want + TIER3 - 1) &~(TIER3 - 1);
		} else {
			mem = (want + TIER4 - 1) &~(TIER4 - 1);
		}
		char *newdata = xrealloc(a, v->data, mem);
		if (!newdata) {
			return NULL;
		}
		v->cap = mem / objsz;
		v->data = newdata;
	}

	return (char*)v->data + (objsz * v->size);
}

void *grow_append_vector(allocator_t *a, struct vector *v, size_t objsz, size_t num) {
	void *ret = grow_vector(a, v, objsz, num);
	if (ret) {
		v->size += num;
	}
	return ret;
}

int set_bitset_size(allocator_t *a, struct bitset *v, size_t size) {
	size_t bytes = (size + 7) / 8;
	if (bytes > v->cap && grow_vector(a, (struct vector*) v, 1, bytes) == NULL) {
		return -1;
	}
	if (bytes > v->bytes) {
		memset(v->v + v->bytes, 0, bytes - v->bytes);
	}
	v->bytes = bytes;
	return 0;
}


