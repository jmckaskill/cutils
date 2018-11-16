#pragma once
#include "cutils/log.h"
#include <stdint.h>

#if defined _MSC_VER && defined _DEBUG
#include <crtdbg.h>
#define BREAK() (_CrtDbgBreak(),0)
#else
#include <stdlib.h>
#define BREAK() (abort(),0)
#endif

typedef struct str str_t;

typedef int(*test_failure_fn)(int assert, const char* file, int line, const char* msg);
extern test_failure_fn test_failed;

log_t *start_test(int argc, const char *argv[]);
int finish_test();

int expect_true(int assert, int a, const char *file, int line, const char *fmt, ...);
int expect_int_eq(int assert, int64_t a, int64_t b, const char *astr, const char *bstr, const char *file, int line);
int expect_int_gt(int assert, int64_t a, int64_t b, const char *astr, const char *bstr, const char *file, int line);
int expect_int_ge(int assert, int64_t a, int64_t b, const char *astr, const char *bstr, const char *file, int line);
int expect_str_eq(int assert, const char *a, const char *b, const char *astr, const char *bstr, const char *file, int line);
int expect_bytes_eq(int assert, const void *a, size_t alen, const void *b, size_t blen, const char *astr, const char *bstr, const char *file, int line);
int expect_float_eq(int assert, double a, double b, const char *astr, const char *bstr, const char *file, int line);
int expect_near(int assert, double a, double b, double delta, const char *astr, const char *bstr, const char *file, int line);
int expect_ptr_eq(int assert, const void *a, const void *b, const char *astr, const char *bstr, const char *file, int line);

int is_equiv_float(double a, double b);
void print_test_data(str_t *s, const uint8_t *a, size_t sz);

#define CHECK(A, FMT, ...) (expect_true(0, (A), __FILE__, __LINE__, FMT, __VA_ARGS__) && BREAK())
#define ASSERT_TRUE(A) (expect_true(1, (A), __FILE__, __LINE__, "assert failed: " #A) && BREAK())
#define EXPECT_TRUE(A) (expect_true(0, (A), __FILE__, __LINE__, "test failed: " #A) && BREAK())
#define EXPECT_EQ(A, B) (expect_int_eq(0, (A), (B), #A, #B, __FILE__, __LINE__) && BREAK())
#define EXPECT_GT(A, B) (expect_int_gt(0, (A), (B), #A, #B, __FILE__, __LINE__) && BREAK())
#define EXPECT_GE(A, B) (expect_int_ge(0, (A), (B), #A, #B, __FILE__, __LINE__) && BREAK())
#define EXPECT_STREQ(A, B) (expect_str_eq(0, (A), (B), #A, #B, __FILE__, __LINE__) && BREAK())
#define EXPECT_BYTES_EQ(A, ALEN, B, BLEN) (expect_bytes_eq(0, (A), (ALEN), (B), (BLEN), #A, #B, __FILE__, __LINE__) && BREAK())
#define EXPECT_FLOAT_EQ(A, B) (expect_float_eq(0, (A), (B), #A, #B, __FILE__, __LINE__) && BREAK())
#define EXPECT_NEAR(A, B, DELTA) (expect_near(0, (A), (B), (DELTA), #A, #B, __FILE__, __LINE__) && BREAK())
#define EXPECT_PTREQ(A, B) (expect_ptr_eq(0, (A), (B), #A, #B, __FILE__, __LINE__) && BREAK())
