#pragma once
#include <stdint.h>
#include <stddef.h>

// Requires a minimum of (3/2 x size) bytes in dest
// size is number of bytes in source
size_t UTF16LE_to_UTF8(void *dest, const void* src, size_t size);

static inline size_t UTF16LE_to_UTF8_size(size_t size) {
	return size * 3 / 2;
}

// Requires a minimum of (2 x size) bytes in dest
// size is number of bytes in source
size_t UTF8_to_UTF16LE(void* dest, const void* src, size_t size);

static inline size_t UTF8_to_UTF16LE_size(size_t size) {
	return size * 2;
}

