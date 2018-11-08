#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct allocator allocator_t;
typedef struct hash_table hash_t;
typedef struct hash_blob blob_t;

struct hash_table {
	size_t size, end, num_used;
	uint32_t *flags;
	allocator_t *alloc;
};

struct hash_blob {
	const void *data;
	size_t size;
#if SIZE_MAX == UINT32_MAX
	char dummy;
#endif
};

void free_hash(hash_t *h, size_t valsz);
void clear_hash(hash_t *h);
size_t hash_memory(const hash_t *h, size_t keysz, size_t valsz);
void remove_hash(hash_t *h, size_t idx);
int resize_hash(hash_t *h, size_t keysz, size_t valsz, size_t newsz);
size_t find_hash(hash_t *h, size_t keysz, uint64_t key, const void *blob);
size_t insert_hash(hash_t *h, size_t keysz, size_t valsz, int *padded, uint64_t key, const void *blob);
int next_hash(hash_t *h, size_t *pidx);

#define CLEAR_HASH(H) clear_hash(&(H)->h)
#define REMOVE_HASH(H, IDX) remove_hash(&(H)->h, (IDX))

// returns the next iterator in a while loop, idx should be initialized to SIZE_MAX
#define NEXT_HASH(H, PIDX) next_hash(&(H)->h, (PIDX))

#define FREE_HASH(H) free_hash(&(H)->h, sizeof((H)->values[0]))
#define HASH_MEMORY(H) hash_memory(&(H)->h, sizeof((H)->keys[0]), sizeof((H)->values[0]))
#define RESIZE_HASH(H, SZ) resize_hash(&(H)->h, sizeof((H)->keys[0]), sizeof((H)->values[0]), (SZ))
#define FIND_HASH(H, KEY) find_hash(&(H)->h, sizeof((H)->keys[0]), (KEY), NULL)
#define FIND_BLOB_HASH(H, KEY, SZ) find_hash(&(H)->h, sizeof((H)->keys[0]), (SZ), (KEY))
#define INSERT_HASH(H, KEY, PADDED) insert_hash(&(H)->h, sizeof((H)->keys[0]), sizeof((H)->values[0]), (PADDED), (KEY), NULL)
#define INSERT_BLOB_HASH(H, KEY, SZ, PADDED) insert_hash(&(H)->h, sizeof((H)->keys[0]), sizeof((H)->values[0]), (PADDED), (SZ), (KEY))

#define FREE_SET(H) free_hash(&(H)->h, 0)
#define SET_MEMORY(H) hash_memory(&(H)->h, sizeof((H)->keys[0]), 0)
#define RESIZE_SET(H, SZ) resize_hash(&(H)->h, sizeof((H)->keys[0]), 0, (SZ))
#define FIND_SET(H, KEY) find_hash(&(H)->h, sizeof((H)->keys[0]), (KEY), NULL)
#define FIND_BLOB_SET(H, KEY, SZ) find_hash(&(H)->h, sizeof((H)->keys[0]), (SZ), (KEY))
#define INSERT_SET(H, KEY, PADDED) insert_hash(&(H)->h, sizeof((H)->keys[0]), 0, (PADDED), (KEY), NULL)
#define INSERT_BLOB_SET(H, KEY, SZ, PADDED) insert_hash(&(H)->h, sizeof((H)->keys[0]), 0, (PADDED), (SZ), (KEY))

