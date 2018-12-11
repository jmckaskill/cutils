#pragma once

#include <stdint.h>
#include <string.h>

#define ALIGN_DOWN(u, sz) ((u) &~ ((sz)-1))
#define ALIGN_UP(u, sz) ALIGN_DOWN(type, (u) + (sz) - 1, (sz))

#if defined _M_X64 || defined __amd64__ || defined __x86_64__ || defined __i386__ || defined _M_IX86 || defined __arm__ || defined __thumb__ || defined _M_ARM || defined __aarch64__
#define NATIVE_LITTLE_ENDIAN
#endif

union endian_float {
	uint32_t u;
	float d;
};
union endian_double {
	uint64_t u;
	double d;
};

static inline uint16_t little_16(const void *p) {
#if defined NATIVE_LITTLE_ENDIAN
	uint16_t ret;
	memcpy(&ret, p, 2);
	return ret;
#else
	const uint8_t *u = p;
	return ((uint16_t) u[0])
		| ((uint16_t) u[1] << 8);
#endif
}

static inline uint32_t little_24(const void *p) {
#ifdef NATIVE_LITTLE_ENDIAN
	uint32_t ret;
	memcpy(&ret, p, 3);
	return ret;
#else
	const uint8_t *u = p;
	return ((uint32_t) u[0])
		| ((uint32_t) u[1] << 8)
		| ((uint32_t) u[2] << 16);
#endif
}

static inline uint32_t little_32(const void *p) {
#if defined NATIVE_LITTLE_ENDIAN
	uint32_t ret;
	memcpy(&ret, p, 4);
	return ret;
#else
	const uint8_t *u = p;
	return ((uint32_t) u[0])
		| ((uint32_t) u[1] << 8)
		| ((uint32_t) u[2] << 16)
		| ((uint32_t) u[3] << 24);
#endif
}

static inline uint64_t little_48(const void *p) {
#ifdef NATIVE_LITTLE_ENDIAN
	uint64_t ret;
	memcpy(&ret, p, 6);
	return ret;
#else
	const uint8_t *u = p;
	return ((uint64_t) u[0])
		 | ((uint64_t) u[1] << 8)
		 | ((uint64_t) u[2] << 16)
		 | ((uint64_t) u[3] << 24)
		 | ((uint64_t) u[4] << 32)
		 | ((uint64_t) u[5] << 40);
#endif
}

static inline uint64_t little_64(const void *p) {
#if defined NATIVE_LITTLE_ENDIAN
	uint64_t ret;
	memcpy(&ret, p, 8);
	return ret;
#else
	const uint8_t *u = p;
	return ((uint64_t) u[0])
		| ((uint64_t) u[1] << 8)
		| ((uint64_t) u[2] << 16)
		| ((uint64_t) u[3] << 24)
		| ((uint64_t) u[4] << 32)
		| ((uint64_t) u[5] << 40)
		| ((uint64_t) u[6] << 48)
		| ((uint64_t) u[7] << 56);
#endif
}

static inline float little_float(const void *p) {
	union endian_float u;
	u.u = little_32(p);
	return u.d;
}
static inline double little_double(const void *p) {
	union endian_double u;
	u.u = little_64(p);
	return u.d;
}

#ifdef NATIVE_LITTLE_ENDIAN
static inline uint16_t little_16_aligned(const void *p) {return *(uint16_t*)p;}
static inline uint32_t little_32_aligned(const void *p) {return *(uint32_t*)p;}
static inline uint64_t little_64_aligned(const void *p) {return *(uint64_t*)p;}
static inline float little_float_aligned(const void *p) {
	union endian_float u;
	u.u = little_32_aligned(p);
	return u.d;
}
static inline double little_double_aligned(const void *p) {
	union endian_double u;
	u.u = little_64_aligned(p);
	return u.d;
}
#else
#define little_16_aligned(P) little_16(P)
#define little_32_aligned(P) little_32(P)
#define little_64_aligned(P) little_64(P)
#define little_float_aligned(P) little_float(P)
#define little_double_aligned(P) little_double(P)
#endif

static inline void *write_little_16(void *p, uint16_t v) {
	uint8_t *u = p;
#if defined NATIVE_LITTLE_ENDIAN
	memcpy(u, &v, 2);
#else
	u[0] = (uint8_t) (v);
	u[1] = (uint8_t) (v >> 8);
#endif
	return u + 2;
}
static inline void *write_little_24(void *p, uint32_t v) {
	uint8_t *u = p;
#ifdef NATIVE_LITTLE_ENDIAN
	memcpy(u, &v, 3);
#else
	u[0] = (uint8_t) (v);
	u[1] = (uint8_t) (v >> 8);
	u[2] = (uint8_t) (v >> 16);
#endif
	return u + 3;
}
static inline void *write_little_32(void *p, uint32_t v) {
	uint8_t *u = p;
#if defined NATIVE_LITTLE_ENDIAN
	memcpy(u, &v, 4);
#else
	u[0] = (uint8_t) (v);
	u[1] = (uint8_t) (v >> 8);
	u[2] = (uint8_t) (v >> 16);
	u[3] = (uint8_t) (v >> 24);
#endif
	return u + 4;
}
static inline void *write_little_48(void *p, uint64_t v) {
	uint8_t *u = p;
#ifdef NATIVE_LITTLE_ENDIAN
	memcpy(u, &v, 6);
#else
	u[0] = (uint8_t) (v);
	u[1] = (uint8_t) (v >> 8);
	u[2] = (uint8_t) (v >> 16);
	u[3] = (uint8_t) (v >> 24);
	u[4] = (uint8_t) (v >> 32);
	u[5] = (uint8_t) (v >> 40);
#endif
	return u + 6;
}
static inline void *write_little_64(void *p, uint64_t v) {
	uint8_t *u = p;
#if defined NATIVE_LITTLE_ENDIAN
	memcpy(u, &v, 8);
#else
	u[0] = (uint8_t) (v);
	u[1] = (uint8_t) (v >> 8);
	u[2] = (uint8_t) (v >> 16);
	u[3] = (uint8_t) (v >> 24);
	u[4] = (uint8_t) (v >> 32);
	u[5] = (uint8_t) (v >> 40);
	u[6] = (uint8_t) (v >> 48);
	u[7] = (uint8_t) (v >> 56);
#endif
	return u + 8;
}
static inline void *write_little_float(void *p, float f) {
	union endian_float u;
	u.d = f;
	return write_little_32(p, u.u);
}

static inline void *write_little_double(void *p, double f) {
	union endian_double u;
	u.d = f;
	return write_little_64(p, u.u);
}

#ifdef NATIVE_LITTLE_ENDIAN
static inline void *write_little_16_aligned(void *p, uint16_t v) {
	*(uint16_t*)p = v; return (uint16_t*)p + 1;
}
static inline void *write_little_32_aligned(void *p, uint32_t v) {
	*(uint32_t*)p = v; return (uint32_t*)p + 1;
}
static inline void *write_little_64_aligned(void *p, uint64_t v) {
	*(uint64_t*)p = v; return (uint64_t*)p + 1;
}
static inline void *write_little_float_aligned(void *p, float f) {
	union endian_float u;
	u.d = f;
	return write_little_32_aligned(p, u.u);
}
static inline void *write_little_double_aligned(void *p, double f) {
	union endian_double u;
	u.d = f;
	return write_little_64_aligned(p, u.u);
}
#else
#define write_little_16_aligned write_little_16
#define write_little_32_aligned write_little_32
#define write_little_64_aligned write_little_64
#define write_little_float_aligned write_little_float
#define write_little_double_aligned write_little_double
#endif


static inline uint16_t big_16(const void *p) {
	const uint8_t *u = p;
	return ((uint16_t)u[0] << 8)
         | ((uint16_t) u[1]);
}
static inline uint32_t big_24(const void *p) {
	const uint8_t *u = p;
	return ((uint32_t)u[0] << 16)
		| ((uint32_t)u[1] << 8)
		| ((uint32_t)u[2]);
}
static inline uint32_t big_32(const void *p) {
	const uint8_t *u = p;
	return ((uint32_t)u[0] << 24)
         | ((uint32_t) u[1] << 16)
         | ((uint32_t) u[2] << 8)
         | ((uint32_t) u[3]);
}
static inline uint64_t big_48(const void *p) {
	const uint8_t *u = p;
	return ((uint64_t)u[0] << 40)
         | ((uint64_t) u[1] << 32)
         | ((uint64_t) u[2] << 24)
         | ((uint64_t) u[3] << 16)
         | ((uint64_t) u[4] << 8)
         | ((uint64_t) u[5]);
}
static inline uint64_t big_64(const void *p) {
	const uint8_t *u = p;
	return ((uint64_t)u[0] << 56)
         | ((uint64_t) u[1] << 48)
         | ((uint64_t) u[2] << 40)
         | ((uint64_t) u[3] << 32)
         | ((uint64_t) u[4] << 24)
         | ((uint64_t) u[5] << 16)
         | ((uint64_t) u[6] << 8)
         | ((uint64_t) u[7]);
}

static inline float big_float(const void *p) {
	union endian_float u;
	u.u = big_32(p);
	return u.d;
}

static inline double big_double(const void *p) {
	union endian_double u;
	u.u = big_64(p);
	return u.d;
}

static inline void *write_big_16(void *p, uint16_t v) {
    uint8_t *u = p;
	u[0] = (uint8_t) (v >> 8);
    u[1] = (uint8_t) (v);
	return u + 2;
}
static inline void *write_big_32(void *p, uint32_t v) {
    uint8_t *u = p;
	u[0] = (uint8_t) (v >> 24);
    u[1] = (uint8_t) (v >> 16);
    u[2] = (uint8_t) (v >> 8);
    u[3] = (uint8_t) (v);
	return u + 4;
}
static inline void *write_big_24(void *p, uint32_t v) {
	uint8_t *u = p;
	u[0] = (uint8_t)(v >> 16);
	u[1] = (uint8_t)(v >> 8);
	u[2] = (uint8_t)(v);
	return u + 3;
}
static inline void *write_big_48(void *p, uint64_t v) {
    uint8_t *u = p;
	u[0] = (uint8_t) (v >> 40);
    u[1] = (uint8_t) (v >> 32);
    u[2] = (uint8_t) (v >> 24);
    u[3] = (uint8_t) (v >> 16);
    u[4] = (uint8_t) (v >> 8);
    u[5] = (uint8_t) (v);
	return u + 6;
}
static inline void *write_big_64(void *p, uint64_t v) {
	uint8_t *u = p;
    u[0] = (uint8_t) (v >> 56);
    u[1] = (uint8_t) (v >> 48);
    u[2] = (uint8_t) (v >> 40);
    u[3] = (uint8_t) (v >> 32);
    u[4] = (uint8_t) (v >> 24);
    u[5] = (uint8_t) (v >> 16);
    u[6] = (uint8_t) (v >> 8);
    u[7] = (uint8_t) (v);
	return u + 8;
}
static inline void *write_big_float(void *p, float f) {
	union endian_float u;
	u.d = f;
	return write_big_32(p, u.u);
}

static inline void *write_big_double(void *p, double f) {
	union endian_double u;
	u.d = f;
	return write_big_64(p, u.u);
}

// clzl = count leading zeros (long)
// These versions do not protect against a zero value
#if defined __GNUC__
static inline unsigned clzl(uint64_t v) {
#if defined __amd64__
	return __builtin_clzl(v);
#else
	uint32_t lo = (uint32_t)v;
	uint32_t hi = (uint32_t)(v >> 32);
	return hi ? __builtin_clz(hi) : (32 + __builtin_clz(lo));
#endif
}
#elif defined _MSC_VER
#include <intrin.h>
#if defined _M_X64
#pragma intrinsic(_BitScanReverse64)
static inline unsigned clzl(uint64_t v) {
	unsigned long ret;
	_BitScanReverse64(&ret, v);
	return 63 - ret;
}
#else
#pragma intrinsic(_BitScanReverse)
static inline unsigned clzl(uint64_t v) {
	unsigned long ret;
	if (_BitScanReverse(&ret, (uint32_t)(v >> 32))) {
		return 31 - ret;
	} else {
		_BitScanReverse(&ret, (uint32_t)(v));
		return 63 - ret;
	}
}
#endif
#else
static inline unsigned clzl(uint64_t v) {
	unsigned n = 0;
	int64_t x = (int64_t)v;
	while (!(x < 0)) {
		n++;
		x <<= 1;
	}
	return n;
}
#endif


// ctz = count trailing zeros
// These versions do not protect against a zero value.
#if defined __GNUC__
static inline uint8_t ctz(uint32_t v) {
	return (uint8_t)__builtin_ctz(v);
}
#elif defined _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
static inline uint8_t ctz(uint32_t v) {
	unsigned long ret;
	_BitScanForward(&ret, v);
	return (uint8_t)ret;
}
#else
static inline uint8_t ctz(uint32_t v) {
	uint8_t n = 0;
	while (!(v & 1)) {
		n++;
		v >>= 1;
	}
	return n;
}
#endif


