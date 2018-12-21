#pragma once

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#define ARRAYSZ(A) (sizeof(A) / sizeof((A)[0]))

#ifdef WIN32
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#if !defined __GLIBC__ && !defined _GNU_SOURCE
static inline void *memrchr(const void *s, int c, size_t n) {
	unsigned char *b = (unsigned char*) s;
	unsigned char *e = b + n - 1;
	while (e >= b) {
		if (*e == c) {
			return e;
		}
		e--;
	}
	return NULL;
}
#endif
#ifdef _MSC_VER
static inline void *memmem(const void *hay, size_t haysz, const void *needle, size_t needlesz) {
	while (needlesz <= haysz) {
		char *test = (char*)memchr(hay, *(unsigned char*)needle, haysz - needlesz + 1);
		if (!test) {
			return NULL;
		}
		if (!memcmp(test, needle, needlesz)) {
			return test;
		}
		test++;
		haysz = (char*)hay + haysz - test;
		hay = test;
	}
	return NULL;
}
#endif

// P should point to a structure of the form
// struct {
//	size_t len;
//	char c_str[...];
// };

typedef struct slice slice_t;

struct slice {
	const char *c_str;
	size_t len;
};

// returns zero on success, non-zero on failure
#define ca_setlen(P, SZ) 		(assert(sizeof((P)->c_str) != sizeof(char*)), (P)->len = (SZ), assert((P)->len < sizeof((P)->c_str)), (P)->c_str[(P)->len] = '\0')
#define ca_clear(P)             ca_setlen(P, 0)
#define ca_set2(P, SRC, SRCSZ) 	(assert(sizeof((P)->c_str) != sizeof(char*)), ca_set2_((P)->c_str, sizeof((P)->c_str), &(P)->len, (SRC), (SRCSZ)))
#define ca_set(P, SRC) 			(assert(sizeof((P)->c_str) != sizeof(char*)), ca_set2_((P)->c_str, sizeof((P)->c_str), &(P)->len, (SRC), strlen(SRC)))
#define ca_setstr(P, ADD) 		(assert(sizeof((P)->c_str) != sizeof(char*)), ca_set2_((P)->c_str, sizeof((P)->c_str), &(P)->len, (ADD).c_str, (ADD).len))
#define ca_add2(P, SRC, SRCSZ) 	(assert(sizeof((P)->c_str) != sizeof(char*)), ca_add2_((P)->c_str, sizeof((P)->c_str), &(P)->len, (SRC), (SRCSZ)))
#define ca_add(P, SRC) 			(assert(sizeof((P)->c_str) != sizeof(char*)), ca_add2_((P)->c_str, sizeof((P)->c_str), &(P)->len, (SRC), strlen(SRC)))
#define ca_addch(P, CH)         (assert(sizeof((P)->c_str) != sizeof(char*)), ((P)->len >= (sizeof((P)->c_str) - 1)) ? -1 : ((P)->c_str[(P)->len++] = (CH), (P)->c_str[(P)->len] = 0))
#define ca_addstr(P, ADD) 		(assert(sizeof((P)->c_str) != sizeof(char*)), ca_add2_((P)->c_str, sizeof((P)->c_str), &(P)->len, (ADD).c_str, (ADD).len))
#define ca_vaddf(P, FMT, AP) 	(assert(sizeof((P)->c_str) != sizeof(char*)), ca_vaddf_((P)->c_str, sizeof((P)->c_str), &(P)->len, (FMT), (AP)))
#define ca_addf(P, ...) 		(assert(sizeof((P)->c_str) != sizeof(char*)), ca_addf_((P)->c_str, sizeof((P)->c_str), &(P)->len, __VA_ARGS__))
#define ca_vsetf(P, FMT, AP) 	(ca_setlen(P, 0), ca_vaddf_((P)->c_str, sizeof((P)->c_str), &(P)->len, (FMT), (AP)))
#define ca_setf(P, ...) 		(ca_setlen(P, 0), ca_addf_((P)->c_str, sizeof((P)->c_str), &(P)->len, __VA_ARGS__))


// comparison functions can be used with str_t, pb_string or a char buffer like above

// can be used with fixed sized arrays as well as str_t
#define str_ends_with(P, TEST) 		(strlen(TEST) <= (P).len && !memcmp((P).c_str + (P).len - strlen(TEST), (TEST), strlen(TEST)))
#define str_iends_with(P, TEST) 	(strlen(TEST) <= (P).len && !strncasecmp((P).c_str + (P).len - strlen(TEST), (TEST), strlen(TEST)))
#define str_begins_with(P, TEST)	(strlen(TEST) <= (P).len && !memcmp((P).c_str, (TEST), strlen(TEST)))
#define str_ibegins_with(P, TEST)	(strlen(TEST) <= (P).len && !strncasecmp((P).c_str, (TEST), strlen(TEST)))

#define str_test(P, TEST) 	(strlen(TEST) == (P).len && !memcmp((P).c_str, (TEST), (P).len))
#define str_itest(P, TEST)	(strlen(TEST) == (P).len && !strncasecmp((P).c_str, (TEST), (P).len))

#define str_equals(A, B) ((A).len == (B).len && !memcmp((A).c_str, (B).c_str, (A).len))

#define str_find_char(P, CH)    ((char*) memchr((P).c_str, (CH), (P).len))
#define str_rfind_char(P, CH) 	((char*) memrchr((P).c_str, (CH), (P).len))

#define slice_all(P, STR) ((P)->len = (STR).len, (P)->c_str = (STR).c_str)
#define slice_left(P, STR, PTR) ((P)->len = (PTR) - (STR).c_str, (P)->c_str = (STR).c_str)
#define slice_right(P, STR, PTR) ((P)->len = (STR).c_str + (STR).len - (PTR), (P)->c_str = (PTR))
#define slice_string(P, STR) ((P)->len = sizeof(STR)-1, (P)->c_str = (STR))

// the following are the implementation functions

static inline int ca_set2_(char *buf, size_t bufsz, size_t *plen, const char *a, size_t asz) {
	if (asz < bufsz) {
		memcpy(buf, a, asz);
		*plen = asz;
		buf[asz] = '\0';
		return 0;
	} else {
		*plen = 0;
		buf[0] = '\0';
		return -1;
	}
}

static inline int ca_add2_(char *buf, size_t bufsz, size_t *plen, const char *a, size_t asz) {
	assert(*plen >= 0 && *plen < bufsz);
	if (*plen + asz < bufsz) {
		memcpy(buf + *plen, a, asz);
		*plen += asz;
		buf[*plen] = '\0';
		return 0;
	} else {
		return -1;
	}
}

static inline int ca_vaddf_(char *buf, size_t bufsz, size_t *plen, const char *fmt, va_list ap) {
	assert(*plen >= 0 && *plen < bufsz);
	size_t left = bufsz - *plen - 1;
	int ret = vsnprintf(buf + *plen, left, fmt, ap);
	if (ret < 0 || (size_t) ret >= left) {
		buf[*plen] = '\0';
		return -1;
	}
	*plen += ret;
	buf[*plen] = '\0';
	return 0;
}

static inline int ca_addf_(char *buf, size_t bufsz, size_t *plen, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	return ca_vaddf_(buf, bufsz, plen, fmt, ap);
}
