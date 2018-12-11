#pragma once

#ifdef HAVE_THREADS_H
#include <threads.h>
#define THREAD_API
static inline void thrd_set_name(const char *name) {
	(void) name;
}

#elif defined WIN32
#include <os/thread-win32.h>

#else
#include <os/thread-pthread.h>
#define THREAD_API
static inline void thrd_set_name(const char *name) {
	(void) name;
}

#endif
