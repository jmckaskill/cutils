#pragma once

#include "cutils/char-array.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// see cutils/char-array.h for string comparison functions
typedef struct str str_t;

struct str {
	size_t cap;
	size_t len;
	char *c_str;
};
extern char str_initbuf[];


static inline void str_setlen(str_t *s, size_t len) {
	assert(0 <= len && len <= s->cap);
	s->len = len;
	s->c_str[len] = '\0';
}
static inline void str_setend(str_t *s, const char *end) {
	assert(s->c_str <= end && end <= s->c_str + s->cap);
	str_setlen(s, (size_t)(end - s->c_str));
}
static inline void str_clear(str_t *s) {
	s->len = 0;
	s->c_str[0] = 0;
}
static inline char *str_release(str_t *s) {
	char *p = s->c_str;
	s->cap = 0;
	s->len = 0;
	s->c_str = str_initbuf;
	return p;
}

void str_destroy(str_t *s);
int str_read_file(str_t *s, const char *fn);
void str_fread_all(str_t *s, FILE *f);
void str_grow(str_t *s, size_t cap);
void str_add2(str_t *s, const char *a, size_t len);
void str_replace_all(str_t *s, const char *find, const char *replacement);


#ifdef __GNUC__
__attribute__((format (printf,2,3)))
#endif
int str_addf(str_t *s, const char *fmt, ...);

#ifdef __GNUC__
__attribute__((format (printf,2,0)))
#endif
int str_vaddf(str_t *s, const char *fmt, va_list ap);

// defined as a macro to also support ca_* char arrays
#define str_addstr(P, STR) str_add2(P, (STR).c_str, (STR).len);

static inline void str_add(str_t *s, const char *a) {
	str_add2(s, a, strlen(a));
}
static inline void str_set2(str_t *s, const char *a, size_t len) {
	str_clear(s);
	str_add2(s, a, len);
}
static inline void str_set(str_t *s, const char *a) {
	str_set2(s, a, strlen(a));
}

#define str_setstr(P, STR) str_set2(P, (STR).c_str, (STR).len)

static inline void str_addch(str_t *s, char ch) {
	if (s->len == s->cap) {
		str_grow(s, s->cap+1);
	}
	s->c_str[s->len++] = ch;
	s->c_str[s->len] = 0;
}

static inline void str_swap(str_t *a, str_t *b) {
	str_t c = *a;
	*a = *b;
	*b = c;
}


#define STR_INIT {0,0,str_initbuf}

static inline str_t str_init0(void) {
	str_t ret = STR_INIT;
	return ret;
}
static inline str_t str_init(const char *str) {
	str_t ret = STR_INIT;
	str_set(&ret, str);
	return ret;
}


#ifdef __cplusplus
}
#endif

