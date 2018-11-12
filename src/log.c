#include "cutils/log.h"
#include "cutils/char-array.h"
#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#endif

#if defined WIN32 && defined DEBUG
#include <crtdbg.h>
#endif

static int do_log(log_t *log, const char *fmt, ...) {
	struct {
		size_t len;
		char c_str[256];
	} buf;
	va_list ap;
	va_start(ap, fmt);
	ca_vsetf(&buf, fmt, ap);
	if (!str_ends_with(buf, "\n")) {
		ca_addch(&buf, '\n');
	}
	FILE *out = stdout;
	if (log == &stderr_log) {
#ifdef WIN32
		OutputDebugStringA(buf.c_str);
#endif
		out = stderr;
	}
	return (int)fwrite(buf.c_str, 1, buf.len, out);
}

log_t stderr_log = { &do_log };
log_t stdout_log = { &do_log };

static int do_fatal(log_t *log, const char *fmt, ...) {
	struct {
		size_t len;
		char c_str[256];
	} buf;
	va_list ap;
	va_start(ap, fmt);
	ca_vsetf(&buf, fmt, ap);
	if (!str_ends_with(buf, "\n")) {
		ca_addch(&buf, '\n');
	}
#if defined WIN32 && defined DEBUG
	_Crt
	FILE *out = stdout;
	if (log == &stderr_log) {
		OutputDebugStringA(buf.c_str);
		out = stderr;
	}
	return (int)fwrite(buf.c_str, 1, buf.len, out);
}

