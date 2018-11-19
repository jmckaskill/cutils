#include "cutils/log.h"
#include "cutils/char-array.h"
#include "cutils/timer.h"
#include "cutils/file.h"
#include <stdio.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#endif

#if defined WIN32 && defined DEBUG
#include <crtdbg.h>
#endif

static int do_log(log_t *log, const char *fmt, ...) {
	int tzoffmin;
	uint64_t utc = utc_us(&tzoffmin);
	time_t sec = (time_t)(utc / 1000 / 1000) + (tzoffmin * 60);
	struct tm *tm = gmtime(&sec);
	struct {
		size_t len;
		char c_str[512];
	} buf;
	va_list ap;
	va_start(ap, fmt);
	ca_setf(&buf, "%d-%d-%d %02d:%02d:%02d.%06d %+03d:%02d: ",
		tm->tm_year + 1900,
		tm->tm_mon + 1,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec,
		(utc / 1000) % 1000000,
		tzoffmin / 60,
		abs(tzoffmin) % 60);
	ca_vaddf(&buf, fmt, ap);
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

static int do_file_log(log_t *log, const char *fmt, ...) {
	struct file_log *fl = (struct file_log*) log;
	struct {
		size_t len;
		char c_str[512];
	} buf;
	va_list ap;
	va_start(ap, fmt);
	ca_vsetf(&buf, fmt, ap);
	return fwrite(buf.c_str, 1, buf.len, fl->f);
}

log_t *open_file_log(struct file_log *fl, const char *path) {
	FILE *f = io_fopen(path, "r");
	if (!f) {
		return NULL;
	}
	fl->f = f;
	fl->log.log = &do_file_log;
	return &fl->log;
}