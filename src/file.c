#include "cutils/file.h"
#include "cutils/utf.h"
#include "cutils/vector.h"
#include "cutils/char-array.h"
#include <stdlib.h>

#ifdef WIN32
#include "cutils/str.h"
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

int append_windows_path(str16_t *s, const char *path) {
	size_t pathlen = strlen(path);
	size_t maxlen = (UTF8_to_UTF16LE_size(pathlen) / 2) + 1;
	uint16_t *p = GROW_VECTOR(s, maxlen+1);
	if (!p) {
		return -1;
	}

	size_t addlen = UTF8_to_UTF16LE(p, path, pathlen) / 2;

	for (size_t i = 0; i < addlen; i++) {
		if (p[i] == L'/') {
			p[i] = L'\\';
		}
	}

	p[addlen++] = 0;
	s->size += addlen;
	return 0;
}

#ifdef WIN32
FILE *io_tmpfile(struct mt_rand *r) {
	// The default windows implementation of tmpfile is stupid. It puts files in C:/
	// Therefore we supply our own version
	str_t path = STR_INIT;
	str_grow(&path, MAX_PATH);
	DWORD sz = GetTempPathA(MAX_PATH, path.c_str);
	if (!sz) {
		goto err;
	}
	for (int i = 0; i < 5; i++) {
		str_setlen(&path, sz);
		str_addf(&path, "\\io.%u.tmp", mt_rand_u32(r));

		int fd = _open(path.c_str, _O_BINARY | _O_CREAT | _O_TEMPORARY | _O_EXCL | _O_RDWR, _S_IREAD | _S_IWRITE);
		if (fd >= 0) {
			str_destroy(&path);
			return _fdopen(fd, "w+b");
		}
	}

err:
	str_destroy(&path);
	return NULL;
}
#else
FILE *io_tmpfile(struct mt_rand *r) {
	(void)r;
	return tmpfile();
}
#endif


FILE *io_fopen(const char *path, const char *mode) {
#ifdef WIN32
	str16_t wpath = { 0 };
	str16_t wmode = { 0 };
	FILE *f = NULL;
	if (!append_windows_path(&wpath, path) && !append_windows_path(&wmode, mode)) {
		f = _wfopen(wpath.v, wmode.v);
	}
	free(wpath.v);
	free(wmode.v);
	return f;
#else
	return fopen(path, mode);
#endif
}

#ifdef WIN32
int io_scandir(const char *dir, scandir_cb cb, void *user) {
	str_t s = STR_INIT;
	str16_t w = { 0 };
	HANDLE h = INVALID_HANDLE_VALUE;
	int ret = -1;
	size_t dirlen = strlen(dir);

	if (!dirlen || dir[dirlen-1] != '/') {
		// dir must be cleaned first
		goto end;
	}

	str_set(&s, dir);
	str_add(&s, "*");
	if (append_windows_path(&w, s.c_str)) {
		goto end;
	}

	WIN32_FIND_DATAW fd;
	h = FindFirstFileW(w.v, &fd);
	if (h == INVALID_HANDLE_VALUE) {
		goto end;
	}

	ret = 0;
	do {
		size_t fdlen = wcslen(fd.cFileName);
		if ((fdlen == 1 && fd.cFileName[0] == '.') || (fdlen == 2 && fd.cFileName[0] == '.' && fd.cFileName[1] == '.')) {
			continue;
		}
		size_t maxlen = UTF16LE_to_UTF8_size(2 * fdlen);
		str_grow(&s, dirlen + maxlen);
		size_t addlen = UTF16LE_to_UTF8(s.c_str + dirlen, fd.cFileName, 2 * fdlen);
		str_setlen(&s, dirlen + addlen);
		ret = cb(user, s.c_str);
	} while (!ret && FindNextFileW(h, &fd));

end:
	str_destroy(&s);
	free(w.v);
	FindClose(h);
	return ret;
}
void io_mkdir(const char *dir) {
	str16_t w = { 0 };
	if (append_windows_path(&w, dir)) {
		return;
	}
	for (size_t i = 1; i < w.size-1; i++) {
		if (w.v[i] == L'\\') {
			w.v[i] = 0;
			CreateDirectoryW(w.v, NULL);
			w.v[i] = L'\\';
		}
	}
	free(w.v);
}
#else
void io_mkdir(const char *dir) {
	str_t s = STR_INIT;
	str_set(&s, dir);
	for (size_t i = 1; i < s.len; i++) {
		if (s.c_str[i] == '/') {
			s.c_str[i] = 0;
			mkdir(s.c_str, 0755);
			s.c_str[i] = '/';
		}
	}
	str_destroy(&s);
}
int io_scandir(const char *dir, scandir_cb cb, void *user) {
	DIR *d = opendir(dir);
	if (!d) {
		return -1;
	}
	str_t s = STR_INIT;
	str_add(&s, dir);
	size_t dirlen = strlen(dir);
	int ret = 0;
	struct dirent *de;
	while (!ret && (de = readdir(d)) != NULL) {
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
			continue;
		}
		str_setlen(&s, dirlen);
		str_add(&s, de->d_name);
		if (de->d_type == DT_DIR) {
			str_addch(&s, '/');
		}
		ret = cb(user, s.c_str);
	}
	closedir(d);
	str_destroy(&s);
	return ret;
}
#endif
