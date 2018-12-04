#include "cutils/hash.h"
#include "cutils/vector.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// use quadratic probing
// stepping function i*(i+1)/2
// table size a power of 2
// (i^2 + i)/2
// 1 - 1
// 2 - 3
// 3 - 6
// 4 - 4*5/2 = 10
// 5 - 5*6/2 = 15
// 6 - 6*7/2 = 21
// i*(i+1)/2 - i*(i-1)/2 = i*(i+1-i+1)/2 = i

struct table {
	hash_t h;
	uint8_t *keys;
	uint8_t *values;
};

void free_hash(hash_t *h, size_t valsz) {
	if (h->size) {
		struct table *t = (struct table*)h;
		xfree(h->alloc, h->flags);
		xfree(h->alloc, t->keys);
		h->flags = NULL;
		h->size = 0;
		t->keys = NULL;
		h->end = 0;
		h->num_used = 0;
		if (valsz) {
			xfree(h->alloc, t->values);
			t->values = NULL;
		}
	}
}

void clear_hash(hash_t *h) {
	h->size = 0;
	h->num_used = 0;
	memset(h->flags, 0, h->end >> 2);
}

size_t hash_memory(const hash_t *h, size_t keysz, size_t valsz) {
	return (h->end >> 2) // flags
		+ (keysz * h->end)
		+ (valsz * h->end);
}

size_t hash_blob(const void *data, size_t size) {
	const uint8_t *p = data;
	const uint8_t *e = p + size;
	size_t h = 0;
	while (p < e) {
		h = (h << 5) - h + (size_t)(*(p++));
	}
	return h;
}

static size_t hash_key(uint64_t key, const void *blob) {
	if (blob) {
		return hash_blob(blob, (size_t)key);
	} else {
		return (size_t)key;
	}
}

static size_t rehash_key(size_t keysz, const void *keys, size_t idx) {
	switch (keysz) {
	case 4:
		return (size_t) (((uint32_t*)keys)[idx]);
	case 8:
		return (size_t) (((uint64_t*)keys)[idx]);
	default:
		break;
	}
	blob_t b = ((blob_t*)keys)[idx];
	return hash_blob(b.data, b.size);
}

static int key_equals(size_t keysz, const void *keys, size_t idx, uint64_t key, const void *blob) {
	switch (keysz) {
	case 4:
		return ((uint32_t*)keys)[idx] == key;
	case 8:
		return ((uint64_t*)keys)[idx] == key;
	default:
		break;
	}
	blob_t b = ((blob_t*)keys)[idx];
	return b.size == key && !memcmp(b.data, blob, b.size);
}

static void set_key(size_t keysz, void *keys, size_t idx, uint64_t key, const void *blob) {
	switch (keysz) {
	case 4:
		((uint32_t*)keys)[idx] = (uint32_t)key;
		break;
	case 8:
		((uint64_t*)keys)[idx] = key;
		break;
	default: {
		blob_t *b = &((blob_t*)keys)[idx];
		b->size = (size_t)key;
		b->data = blob;
		break;
	}
	}
}

// flags is a bitfield with 2 bits for each entry
// 0 - has not been used
// 1 or 3 - used now
// 2 - removed (used previously)
// the addition of the removed state helps the iterator
// to know when to stop or to continue because the slot
// may be further along

#define USED 1
#define REMOVED 2

static size_t get_flags(uint32_t *flags, size_t idx) {
	return (flags[idx >> 4] >> (2 * (idx & 15))) & 3;
}

static void set_used(uint32_t *flags, size_t idx) {
	flags[idx >> 4] |= 1 << (2 * (idx & 15));
}

static void set_removed(uint32_t *flags, size_t idx) {
	flags[idx >> 4] &= 1 << (2 * (idx & 15));
	flags[idx >> 4] |= 2 << (2 * (idx & 15));
}

int next_hash(hash_t *h, size_t *pidx) {
	for (;;) {
		(*pidx)++;
		if (*pidx == h->end) {
			return 0;
		}
		size_t flag = get_flags(h->flags, *pidx);
		if (flag & USED) {
			return 1;
		}
	}
}

size_t find_hash(hash_t *h, size_t keysz, uint64_t key, const void *blob) {
	struct table *t = (struct table*)h;
	if (!h->end) {
		return 0;
	}
	size_t mask = h->end - 1;
	size_t hash = hash_key(key, blob) & mask;
	size_t first = hash;
	size_t step = 0;
	do {
		size_t flags = get_flags(h->flags, hash);
		if (!flags) {
			break;
		} else if ((flags & USED) && key_equals(keysz, t->keys, hash, key, blob)) {
			return hash;
		}
		hash = (hash + (++step)) & mask;
	} while (hash != first);

	return h->end;
}

static size_t roundup(size_t v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
#if SIZE_MAX == UINT64_MAX
	v |= v >> 32;
#endif
	v++;
	return v;
}

static int rehash_table(hash_t *h, size_t keysz, size_t valsz, size_t newcap) {
	struct table *t = (struct table*)h;
	newcap = roundup(newcap);
	if (newcap < 16) {
		newcap = 16;
	}
	allocator_t *a = h->alloc;
	uint8_t *new_vals = NULL;
	uint32_t *new_flags = xcalloc(a, newcap >> 4, 4);
	uint8_t *new_keys = xmalloc(a, keysz * newcap);
	if (!new_flags || !new_keys) {
		goto err;
	}
	if (valsz) {
		new_vals = xmalloc(a, valsz * newcap);
		if (!new_vals) {
			goto err;
		}
	}

	size_t mask = newcap - 1;
	for (size_t i = 0; i < h->end; i++) {
		size_t old_flag = get_flags(h->flags, i);
		if (old_flag & USED) {
			size_t hash = rehash_key(keysz, t->keys, i) & mask;
			size_t first = hash;
			size_t step = 0;
			for (;;) {
				size_t new_flag = get_flags(new_flags, hash);
				if (!new_flag) {
					memcpy(new_keys + (hash * keysz), t->keys + (i * keysz), keysz);
					set_used(new_flags, hash);
					if (valsz) {
						memcpy(new_vals + (hash * valsz), t->values + (i * valsz), valsz);
					}
					break;
				}
				hash = (hash + (++step)) & mask;
				if (hash == first) {
					goto err;
				}
			}
		}
	}

	if (valsz) {
		xfree(a, t->values);
		t->values = new_vals;
	}
	xfree(a, t->keys);
	xfree(a, h->flags);
	t->keys = new_keys;
	h->flags = new_flags;
	h->end = newcap;
	h->num_used = h->size;
	return 0;

err:
	xfree(a, new_flags);
	xfree(a, new_keys);
	xfree(a, new_keys);
	return -1;
}

int resize_hash(hash_t *h, size_t keysz, size_t valsz, size_t newsz) {
	size_t slots = newsz * 2;
	if (slots > h->end) {
		return rehash_table(h, keysz, valsz, slots);
	} else {
		return 0;
	}
}

size_t insert_hash(hash_t *h, size_t keysz, size_t valsz, bool *padded, uint64_t key, const void *blob) {
	struct table *t = (struct table*)h;
	if (h->size >= h->end * 3 / 4) {
		// grow the table
		if (rehash_table(h, keysz, valsz, h->end + 1)) {
			return h->end;
		}
	} else if (h->num_used >= h->end * 7 / 8) {
		// clear the removed entries
		if (rehash_table(h, keysz, valsz, h->end)) {
			return h->end;
		}
	}

	size_t mask = h->end - 1;
	size_t hash = hash_key(key, blob) & mask;
	size_t first = hash;
	size_t step = 0;
	size_t site = h->end;
	do {
		size_t flags = get_flags(h->flags, hash);
		if (!flags) {
			if (site == h->end) {
				h->num_used++;
				site = hash;
			}
			goto add_entry;
		} else if ((flags & USED) && key_equals(keysz, t->keys, hash, key, blob)) {
			*padded = false;
			return hash;
		} else if (flags == REMOVED) {
			site = hash;
		}
		hash = (hash + (++step)) & mask;
	} while (hash != first);

	if (site == h->end) {
		return h->end;
	}
add_entry:
	set_key(keysz, t->keys, site, key, blob);
	set_used(h->flags, site);
	h->size++;
	*padded = true;
	return site;
}

void remove_hash(hash_t *h, size_t idx) {
	if (idx < h->end) {
		set_removed(h->flags, idx);
	}
}


