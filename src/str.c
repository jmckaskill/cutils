#include "cutils/str.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

char str_initbuf[] = {0};

void str_destroy(str_t *s) {
    if (s->cap) {
        free(s->c_str);
    }
}

void str_grow(str_t *s, size_t cap) {
    if (cap < s->cap) {
        return;
    }

    char *buf = s->cap ? s->c_str : NULL;
    size_t newcap = (s->cap + 16) * 3 / 2;
    if (newcap < cap) {
        newcap = cap;
    }

    s->c_str = (char*) realloc(buf, newcap+1);
    s->cap = newcap;
}

void str_add2(str_t *s, const char *a, size_t len) {
    str_grow(s, s->len + len);
    memcpy(s->c_str + s->len, a, len);
    s->len += len;
    s->c_str[s->len] = 0;
}

int str_addf(str_t *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    return str_vaddf(s, fmt, ap);
}

int str_vaddf(str_t *s, const char *fmt, va_list ap) {
    str_grow(s, s->len + 1);
    for (;;) {
        va_list aq;
        va_copy(aq, ap);

        // detect when snprintf runs out of buffer by seeing whether
        // it overwrites the null terminator at the end of the buffer
        char* buf = s->c_str + s->len;
        size_t bufsz = s->cap - s->len;
        buf[bufsz] = '\0';
        int ret = vsnprintf(buf, bufsz + 1, fmt, aq);

        if (ret > (int)bufsz) {
            // snprintf has told us the size of buffer required (ISO C99
            // behavior)
            str_grow(s, s->len + ret);
            // loop around to try again

        } else if (ret >= 0) {
            /* success */
            s->len += ret;
            return ret;

        } else if (buf[bufsz] != '\0') {
            /* snprintf has returned an error but has written to the end of the
             * buffer (MSVC behavior). The buffer is not large enough so grow
             * and retry. This can also occur with a format error if it occurs
             * right on the boundary, but then we grow the buffer and can
             * figure out its an error next time around.
             */
            str_grow(s, s->cap + 1);

        } else {
            /* snprintf has returned an error but has not written to the last
             * character in the buffer. We have a format error.
             */
            return -1;
        }
    }
}

void str_fread_all(str_t *s, FILE *f) {
#ifdef WIN32
	_setmode(_fileno(f), _O_BINARY);
#endif
	for (;;) {
        str_grow(s, s->len + 4096);
        int r = (int) fread(s->c_str + s->len, 1, (int)(s->cap - s->len), f);
		if (r == 0) {
			break;
		}
        s->len += r;
        s->c_str[s->len] = 0;
    }
}

int str_read_file(str_t *s, const char *fn) {
	FILE *f;
	if (!strcmp(fn, "-")) {
		str_fread_all(s, stdin);
		return 0;
	} else if ((f = fopen(fn, "rb")) != NULL) {
		str_fread_all(s, f);
		fclose(f);
		return 0;
	} else {
		return -1;
	}
}

void str_replace_all(str_t *s, const char *find, const char *replacement) {
    size_t flen = strlen(find);
    size_t rlen = strlen(replacement);
    size_t off = 0;
    for (;;) {
        char *f = (char*) memmem(s->c_str + off, s->len - off, find, flen);
        if (!f) {
            break;
        }
        off = (int) (f - s->c_str);
		if (flen == rlen) {
            memcpy(f, replacement, rlen);
        } else {
            if (rlen > flen) {
                str_grow(s, s->len + rlen - flen);
                f = s->c_str + off;
            }
            memmove(f + rlen, f + flen, s->len - (off + flen));
            memcpy(f, replacement, rlen);
            str_setlen(s, s->len + rlen - flen);
        }
        off += rlen;
    }
}

static bool is_slash(char ch) {
#ifdef WIN32
	return ch == '/' || ch == '\\';
#else
	return ch == '/';
#endif
}

static bool is_root(const char *b, const char *e) {
#ifdef WIN32
	// look for C:/ style drive roots
	if (e - b == 3 && (('A' <= b[0] && b[0] <= 'Z') || ('a' <= b[0] && b[0] <= 'z')) && b[1] == ':' && b[2] == '/') {
		return true;
	}
#endif
	return e - b == 1 && b[0] == '/';
}

void str_clean_path(str_t *s) {
	// based off of Plan9's cleanname function
	// https://9p.io/sys/doc/lexnames.html
	// 0. Replace windows style \\ with /
	// 1. Reduce multiple slashes to a single slash.
	// 2. Eliminate . path name elements(the current directory).
	// 3. Eliminate .. path name elements(the parent directory) and the non-. non-.., element that precedes them.
	// 4. Eliminate .. elements that begin a rooted path, that is, replace /.. by / at the beginning of a path.
	// 5. Leave intact ..elements that begin a non - rooted path.

	// If the path ends with . or .. we may add a trailing slash.
	// Thus the string could grow.
	if (s->len && s->c_str[s->len - 1] == '.') {
		str_grow(s, s->len + 1);
	}

	char *to = s->c_str;
	char *from = s->c_str;
	char *end = from + s->len;
	while (from < end) {
		// find the next path segment
		char *seg_start = from;
		while (from < end && !is_slash(*from)) {
			from++;
		}
		char *seg_end = from;
		while (from < end && is_slash(*from)) {
			from++;
		}
		bool has_trailing_slash = from > seg_end;

		// decide what to do with it
		if (seg_end - seg_start == 1 && seg_start[0] == '.') {
			// 2. Eliminate . path name elements(the current directory).

		} else if (seg_end - seg_start == 2 && seg_start[0] == '.' && seg_start[1] == '.') {
			if (to == s->c_str) {
				// 5. Leave intact .. elements that begin a non-rooted path.
				*(to++) = '.';
				*(to++) = '.';
				*(to++) = '/'; // .. elements are always directories
			} else if (is_root(s->c_str, to)) {
				// 4. Eliminate .. elements that begin a rooted path, that is, replace /.. by / at the beginning of a path.

			} else {
				char *to_seg_end = to - 1;
				char *to_seg_start = to_seg_end - 1;
				while (to_seg_start > s->c_str && !is_slash(to_seg_start[-1])) {
					to_seg_start--;
				}
				if (to_seg_end - to_seg_start == 2 && to_seg_start[0] == '.' && to_seg_start[1] == '.') {
					// 5. Leave intact .. elements that begin a non-rooted path.
					*(to++) = '.';
					*(to++) = '.';
					*(to++) = '/'; // .. elements are always directories
				} else {
					// 3. Eliminate .. path name elements(the parent directory) and the non-. non-.., element that precedes them.
					to = to_seg_start;
				}
			}
		} else {
			// copy over the valid segment
			memcpy(to, seg_start, seg_end - seg_start);
			to += seg_end - seg_start;
			if (has_trailing_slash) {
				// 0. Replace windows style \\ with /
				*(to++) = '/';
			}
		}
	}

	str_setend(s, to);

	if (!s->len) {
		str_set(s, "./");
	}
}

const char *path_file_extension2(const char *s, size_t len) {
	for (const char *p = s + len; p > s && p[-1] != '/'; p--) {
		if (*p == '.') {
			return p;
		}
	}
	return NULL;
}

const char *path_file_extension(const char *s) {
	return path_file_extension2(s, strlen(s));
}

const char *path_last_segment(const char *s) {
	const char *p = s + strlen(s);

	if (p > s && p[-1] == '/') {
		// path is a directory
		// skip over the trailing slash
		p--;
	}

	// find the next slash
	while (p > s && p[-1] != '/') {
		p--;
	}
	return p;
}

bool path_is_directory(const char *s) {
	size_t len = strlen(s);
	return len && s[len - 1] == '/';
}
