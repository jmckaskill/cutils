#pragma once

#include "c.h"
#include <time.h>

typedef void* thrd_t; /*HANDLE*/
#define THREAD_API __stdcall
typedef int (THREAD_API *thrd_start_t)(void*);

enum {
	thrd_success,
	thrd_timedout,
	thrd_error,
};

CFUNC int nanosleep(const struct timespec *ts, struct timespec *rem);
CFUNC int thrd_create(thrd_t *thr, thrd_start_t func, void * arg);
CFUNC int thrd_join(thrd_t thr, int *res);
CFUNC int thrd_detach(thrd_t thr);
CFUNC void thrd_sleep(const struct timespec *ts, struct timespec *rem);
CFUNC void thrd_set_name(const char *name);

typedef struct {
	// copy of CRITICAL_SECTION
	void *DebugInfo;
	long LockCount;
	long RecursionCount;
	void *OwningThread;
	void *LockSemaphore;
	void *SpinCount;
} mtx_t;

enum {
	mtx_plain,
};

CFUNC int mtx_init(mtx_t *mutex, int type);
CFUNC void mtx_destroy(mtx_t *mutex);
CFUNC int mtx_lock(mtx_t *mutex);
CFUNC int mtx_unlock(mtx_t *mutex);

typedef struct {
	// copy of CONDITION_VARIABLE
	void *ptr;
} cnd_t;

CFUNC int cnd_init(cnd_t *cond);
CFUNC void cnd_destroy(cnd_t *cond);
CFUNC int cnd_signal(cnd_t *cond);
CFUNC int cnd_broadcast(cnd_t *cond);
CFUNC int cnd_wait(cnd_t *cond, mtx_t *mtx);
CFUNC int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts);

typedef struct {
	// copy of INIT_ONCE
	void *ptr;
} once_flag;

// copy of INIT_ONCE_STATIC_INIT
#define ONCE_FLAG_INIT {0}

CFUNC void call_once(once_flag *flag, void (*func)(void));
