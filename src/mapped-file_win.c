#include "cutils/file.h"
#include "cutils/file.h"
#include <limits.h>

#ifdef WIN32

#include <Windows.h>

int map_file(mapped_file *mf, const char *fn) {
	mf->data = NULL;
	HANDLE fd = INVALID_HANDLE_VALUE;
	HANDLE md = INVALID_HANDLE_VALUE;
	str16_t w = { 0 };
	if (append_windows_path(&w, fn)) {
		goto err;
	}

	fd = CreateFileW(w.v, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fd == INVALID_HANDLE_VALUE) {
		goto err;
	}

	LARGE_INTEGER sz;
	if (!GetFileSizeEx(fd, &sz) || sz.QuadPart > (uint64_t)SIZE_MAX) {
		goto err;
	}

	md = CreateFileMapping(fd, NULL, PAGE_READONLY, sz.HighPart, sz.LowPart, NULL);
	if (md == INVALID_HANDLE_VALUE) {
		goto err;
	}
	// we no longer need the file handle, the mapping handle will keep the view open
	CloseHandle(fd);
	fd = INVALID_HANDLE_VALUE;

	void *ptr = MapViewOfFileEx(md, FILE_MAP_READ, 0, 0, (size_t)sz.QuadPart, NULL);
	if (!ptr) {
		goto err;
	}

	free(w.v);
	mf->h = (intptr_t)md;
	mf->data = (uint8_t*)ptr;
	mf->size = (size_t)sz.QuadPart;
	return 0;
err:
	free(w.v);
	CloseHandle(fd);
	CloseHandle(md);
	return -1;
}

void unmap_file(mapped_file *mf) {
	if (mf->data) {
		UnmapViewOfFile(mf->data);
		CloseHandle((HANDLE)mf->h);
		mf->data = NULL;
		mf->size = 0;
	}
}

#endif
