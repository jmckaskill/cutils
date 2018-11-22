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

static int timezone_offset(time_t utc) {
	struct tm *ptm = gmtime(&utc);
	// Request that mktime() looksup dst in timezone database
	ptm->tm_isdst = -1;
	return (int)difftime(utc, mktime(ptm));
}

static int do_log(log_t *log, const char *fmt, ...) {
	uint64_t us = utc_us();
	time_t utc = (time_t)(us / 1000 / 1000);
	int tzoff = timezone_offset(utc) / 60;
	struct tm *tm = localtime(&utc);
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
		us % 1000000,
		tzoff / 60,
		abs(tzoff) % 60);
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
	struct file_logger *fl = (struct file_logger*) log;
	FILE *f = io_fopen(fl->path, "a");
	if (f) {
		struct {
			size_t len;
			char c_str[512];
		} buf;
		va_list ap;
		va_start(ap, fmt);
		ca_vsetf(&buf, fmt, ap);
		fwrite(buf.c_str, 1, buf.len, f);
		fclose(f);
	}
	return 0;
}

log_t *open_file_log(struct file_logger *fl, const char *path) {
	fl->path = path;
	fl->log.log = &do_file_log;
	return &fl->log;
}
