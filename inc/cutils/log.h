#pragma once

typedef struct logger log_t;

struct logger {
#ifdef __GNUC__
	__attribute__((format(printf, 2, 3)))
#endif
	int(*log)(struct logger* log, const char *fmt, ...);
};

#define LOG(PLOG, ...) ((void) ((void*)(PLOG) != NULL && (PLOG)->log(PLOG, __VA_ARGS__)))

#ifndef container_of
#define container_of(ptr, type, member) ((type*) ((char*) (ptr) - offsetof(type, member)))
#endif
