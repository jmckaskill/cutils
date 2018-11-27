#include "cutils/test.h"
#include "cutils/flag.h"
#include "cutils/str.h"
#include "cutils/log.h"
#include <cutils/timer.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#ifdef WIN32
#include <windows.h>
#include <crtdbg.h>
#else
#include <unistd.h>
#include <time.h>
#endif

static const char *output_fn;
static str_t slog = STR_INIT;
static int error_count;
static const char *test_name;
static struct timer start_time;

static int do_log(log_t *l, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	size_t sz = slog.len;
	str_vaddf(&slog, fmt, ap);
	str_addch(&slog, '\n');

#ifdef WIN32
	if (IsDebuggerPresent()) {
		OutputDebugStringA(slog.c_str + sz);
	}
#endif

	if (error_count || !output_fn) {
		fwrite(slog.c_str + sz, 1, slog.len - sz, stderr);
		fflush(stderr);
	}

	return (int) (slog.len - sz);
}

static log_t test_logger = {
	&do_log,
};

#ifdef WIN32
static DWORD WINAPI timeout_thread(void *udata) {
	Sleep(1000);
	OutputDebugStringA("test timed out\n");
	fprintf(stderr, "test timed out\n");
	ExitProcess(3);
	return 0;
}
#endif

static char **g_argv;

log_t *start_test(int argc, const char *argv[]) {
	test_name = argv[0];
	flag_string(&output_fn, 'o', "output", "FILE", "test status output file");
	g_argv = flag_parse(&argc, argv, "[options]", 0);
#ifdef WIN32
	if (output_fn) {
		DeleteFileA(output_fn);
		HANDLE h = CreateThread(NULL, 0, &timeout_thread, NULL, 0, NULL);
		CloseHandle(h);
	}
#else
	if (output_fn) {
		unlink(output_fn);
		alarm(1);
	}
#endif
	str_clear(&slog);
	start_timer(&start_time);
	return &test_logger;
}

int finish_test() {
	LOG(&test_logger, "test finished|test:%s|timeMillis:%g", test_name, stop_timer(&start_time) * 1e3);
	if (output_fn) {
		FILE *f = fopen(output_fn, "wb");
		if (f) {
			fwrite(slog.c_str, 1, slog.len, f);
			fclose(f);
		}
	}
	free(g_argv);
	str_destroy(&slog);
	return error_count;
}

static int do_test_failed(int assert, const char *file, int line, const char *msg) {
	if (!error_count) {
		fwrite(slog.c_str, 1, slog.len, stderr);
	}
	error_count++;
	fprintf(stderr, "%s(%d,1): error: %s", file, line, msg);
#if defined _MSC_VER && !defined NDEBUG
	if (IsDebuggerPresent()) {
		return _CrtDbgReport(_CRT_ASSERT, file, line, NULL, "%s", msg);
	}
#endif
	return assert;
}

test_failure_fn test_failed = &do_test_failed;

int expect_true(int assert, int a, const char *file, int line, const char *fmt, ...) {
	int ret = 0;
	if (!a) {
		str_t s = STR_INIT;
		va_list ap;
		va_start(ap, fmt);
		str_vaddf(&s, fmt, ap);
		str_addch(&s, '\n');
		ret = test_failed(assert, file, line, s.c_str);
		str_destroy(&s);
	}
	return ret;
}

static void print_int_argument(str_t *s, int64_t a, const char *astr) {
	char *end;
	if (strtoll(astr, &end, 0) != a || *end) {
		str_addf(s, " (%s)", astr);
	}
}

static void print_float_argument(str_t *s, double a, const char *astr) {
	char *end;
	if (strtod(astr, &end) != a || *end) {
		str_addf(s, " (%s)", astr);
	}
}

int expect_int_gt(int assert, int64_t a, int64_t b, const char *astr, const char *bstr, const char *file, int line) {
	int ret = 0;
	if (!(a > b)) {
		str_t s = STR_INIT;
		str_addf(&s, "expected %"PRId64, a);
		print_int_argument(&s, a, astr);
		str_addf(&s, " > %"PRId64, b);
		print_int_argument(&s, b, bstr);
		str_addch(&s, '\n');
		ret = test_failed(assert, file, line, s.c_str);
		str_destroy(&s);
	}
	return ret;
}

int expect_int_ge(int assert, int64_t a, int64_t b, const char *astr, const char *bstr, const char *file, int line) {
	int ret = 0;
	if (!(a >= b)) {
		str_t s = STR_INIT;
		str_addf(&s, "expected %"PRId64, a);
		print_int_argument(&s, a, astr);
		str_addf(&s, " >= %"PRId64, b);
		print_int_argument(&s, b, bstr);
		str_addch(&s, '\n');
		ret = test_failed(assert, file, line, s.c_str);
		str_destroy(&s);
	}
	return ret;
}

int expect_int_eq(int assert, int64_t a, int64_t b, const char *astr, const char *bstr, const char *file, int line) {
	int ret = 0;
	if (a != b) {
		str_t s = STR_INIT;
		str_addf(&s, "value of %s\n\t  Actual: %"PRId64"\n\tExpected: %"PRId64, bstr, b, a);
		print_int_argument(&s, a, astr);
		str_addch(&s, '\n');
		ret = test_failed(assert, file, line, s.c_str);
		str_destroy(&s);
	}
	return ret;
}

int expect_str_eq(int assert, const char *a, const char *b, const char *astr, const char *bstr, const char *file, int line) {
	int ret = 0;
	if (strcmp(a, b)) {
		str_t s = STR_INIT;
		str_addf(&s, "value of %s\n\t  Actual: \"%s\"\n\tExpected: \"%s\" (%s)\n", bstr, b, a, astr);
		ret = test_failed(assert, file, line, s.c_str);
		str_destroy(&s);
	}
	return ret;
}

void print_test_data(str_t *s, const uint8_t *p, size_t sz) {
	str_addch(s, '[');
	for (size_t i = 0; i < sz; i++) {
		if (i) {
			str_addch(s, ',');
		}
		str_addf(s, "x%02X", p[i]);
	}
	str_addch(s, ']');
}

int expect_bytes_eq(int assert, const void *a, size_t alen, const void *b, size_t blen, const char *astr, const char *bstr, const char *file, int line) {
	int ret = 0;
	if (alen != blen || memcmp(a, b, alen)) {
		str_t s = STR_INIT;
		str_addf(&s, "value of %s\n\t  Actual: ", bstr);
		print_test_data(&s, b, blen);
		str_add(&s, "\n\tExpected: ");
		print_test_data(&s, a, alen);
		str_addf(&s, " (%s)\n", astr);
		ret = test_failed(assert, file, line, s.c_str);
		str_destroy(&s);
	}
	return ret;
}

int expect_ptr_eq(int assert, const void *a, const void *b, const char *astr, const char *bstr, const char *file, int line) {
	int ret = 0;
	if (a != b) {
		str_t s = STR_INIT;
		str_addf(&s, "value of %s\n\t  Actual: %p\n\tExpected: %p (%s)\n", bstr, b, a, astr);
		ret = test_failed(assert, file, line, s.c_str);
		str_destroy(&s);
	}
	return ret;
}

int is_equiv_float(double a, double b) {
	if (isfinite(a) && isfinite(b)) {
		return fabs(a-b) < 0.00001;
	} else if (isinf(a) && isinf(b)) {
		return (a > 0) == (b > 0);
	} else {
		return fpclassify(a) == fpclassify(b);
	}
}

int expect_float_eq(int assert, double a, double b, const char *astr, const char *bstr, const char *file, int line) {
	int ret = 0;
	if (!is_equiv_float(a, b)) {
		str_t s = STR_INIT;
		str_addf(&s, "value of %s\n\t  Actual: %.17g\n\tExpected: %.17g", bstr, b, a);
		print_float_argument(&s, a, astr);
		str_addch(&s, '\n');
		ret = test_failed(assert, file, line, s.c_str);
		str_destroy(&s);
	}
	return ret;
}

int expect_near(int assert, double a, double b, double delta, const char *astr, const char *bstr, const char *file, int line) {
	int ret = 0;
	if (fabs(a-b) > delta) {
		str_t s = STR_INIT;
		str_addf(&s, "value of %s\n\t  Actual: %.17g\n\tExpected: %.17g", bstr, b, a);
		print_float_argument(&s, a, astr);
		str_addf(&s, "\n\t  Within: %g\n", delta);
		ret = test_failed(assert, file, line, s.c_str);
		str_destroy(&s);
	}
	return ret;
}
