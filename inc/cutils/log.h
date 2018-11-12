#pragma once

#if defined _MSC_VER && defined _DEBUG
#include <crtdbg.h>
#define BREAK() (_CrtDbgBreak(),0)
#else
#include <stdlib.h>
#define BREAK() (abort(),0)
#endif

typedef struct logger log_t;

struct logger {
#ifdef __GNUC__
	__attribute__((format(printf, 2, 3)))
#endif
	int(*log)(struct logger* log, const char *fmt, ...);
};

#define LOG(PLOG, ...) ((void) ((void*)(PLOG) != NULL && (PLOG)->log(PLOG, __VA_ARGS__)))
#define FATAL(PLOG, ...) (LOG(PLOG, __VA_ARGS__), BREAK())

#ifndef container_of
#define container_of(ptr, type, member) ((type*) ((char*) (ptr) - offsetof(type, member)))
#endif

extern log_t stderr_log;
extern log_t stdout_log;
