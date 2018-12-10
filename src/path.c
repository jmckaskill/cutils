#include <cutils/path.h>
#include <string.h>
#include <assert.h>

static bool is_slash(enum path_type type, char ch) {
	if (type == PATH_WINDOWS) {
		return ch == '/' || ch == '\\';
	} else {
		return ch == '/';
	}
}

static bool is_root(enum path_type type, const char *b, const char *e) {
	// look for C:/ style drive roots
	if (type == PATH_WINDOWS) {
		if (e - b == 3 && (('A' <= b[0] && b[0] <= 'Z') || ('a' <= b[0] && b[0] <= 'z')) && b[1] == ':' && b[2] == '/') {
			return true;
		}
	}
	return e - b == 1 && b[0] == '/';
}

int clean_path(enum path_type type, char *s, size_t *len) {
	// based off of Plan9's cleanname function
	// https://9p.io/sys/doc/lexnames.html
	// 0. Replace windows style \\ with /
	// 1. Reduce multiple slashes to a single slash.
	// 2. Eliminate . path name elements(the current directory).
	// 3. Eliminate .. path name elements(the parent directory) and the non-. non-.., element that precedes them.
	// 4. Eliminate .. elements that begin a rooted path, that is, replace /.. by / at the beginning of a path.
	// 5. Leave intact .. elements that begin a non - rooted path.

	if (!*len) {
		return 0;
	}

	bool changed = false;
	char *to = s;
	char *from = s;
	char *end = s + *len;
	while (from < end) {
		// find the next path segment
		char *seg_start = from;
		while (from < end && !is_slash(type, *from)) {
			from++;
		}
		char *seg_end = from;
		while (from < end && is_slash(type, *from)) {
			from++;
		}
		bool has_trailing_slash = from > seg_end;
		if (has_trailing_slash && (seg_end[0] == '\\' || from > seg_end + 1)) {
			changed = true;
		}

		// decide what to do with it
		if (seg_end - seg_start == 1 && seg_start[0] == '.') {
			// 2. Eliminate . path name elements(the current directory).
			if (*len > 1) {
				changed = true;
			}
			continue;

		} else if (seg_end - seg_start == 2 && seg_start[0] == '.' && seg_start[1] == '.') {
			if (to == s) {
				// 5. Leave intact .. elements that begin a non-rooted path.
				goto add_dot_dot;
			} else if (is_root(type, s, to)) {
				// 4. Eliminate .. elements that begin a rooted path, that is, replace /.. by / at the beginning of a path.
				changed = true;
				continue;

			} else {
				char *to_seg_end = to - 1;
				char *to_seg_start = to_seg_end - 1;
				while (to_seg_start > s && !is_slash(type, to_seg_start[-1])) {
					to_seg_start--;
				}
				if (to_seg_end - to_seg_start == 2 && to_seg_start[0] == '.' && to_seg_start[1] == '.') {
					// 5. Leave intact .. elements that begin a non-rooted path.
					goto add_dot_dot;
				} else {
					// 3. Eliminate .. path name elements(the parent directory) and the non-. non-.., element that precedes them.
					to = to_seg_start;
					changed = true;
					continue;
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
			continue;
		}
	add_dot_dot:
		*(to++) = '.';
		*(to++) = '.';
		if (from < end) {
			*(to++) = '/';
		} else if (has_trailing_slash) {
			changed = true;
		}
	}
	
	if (!changed) {
		return 0;
	}
	if (to == s) {
		*(to++) = '.';
	}
	assert(to <= s + *len);
	memset(to, 0, s + *len - to);
	*len = (size_t)(to - s);
	return 1;
}

const char *path_file_extension2(const char *s, size_t len) {
	if (len > 1) {
		for (const char *p = s + len; p > s && p[-1] != '/'; p--) {
			if (*p == '.') {
				return p;
			}
		}
	}
	return NULL;
}

const char *path_file_extension(const char *s) {
	return path_file_extension2(s, strlen(s));
}

const char *path_last_segment(const char *s) {
	size_t len = strlen(s);
	const char *p = s + len;

	if (len <= 1) {
		// current directory '.'
		// root directory '/'
		// single letter file 'a'
		// or a blank file name ''
		return s;
	} else if (p > s && p[-1] == '/') {
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
	return (len && s[len - 1] == '/')
		|| (len == 1 && s[0] == '.')
		|| (len == 2 && s[0] == '.' && s[1] == '.')
		|| (len > 2 && s[len - 3] == '/' && s[len - 2] == '.' && s[len - 1] == '.');
}
