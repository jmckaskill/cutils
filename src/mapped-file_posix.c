#include "cutils/file.h"

#ifndef WIN32

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

int map_file(mapped_file *mf, const char *fn) {
	mf->data = NULL;

	int fd = open(fn, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		return -1;
	}
	struct stat st;
	if (fstat(fd, &st)) {
		close(fd);
		return -1;
	}

	if (st.st_size > SIZE_MAX) {
		close(fd);
		return -1;
	}

	size_t sz = (size_t) st.st_size;
	void *p = mmap(NULL, sz, PROT_READ, MAP_FILE|MAP_PRIVATE, fd, 0);
	if (p == MAP_FAILED) {
		close(fd);
		return -1;
	}

	mf->h = fd;
	mf->data = p;
	mf->size = sz;
	return 0;
}

void unmap_file(mapped_file *mf) {
	if (mf->data) {
		munmap((void*) mf->data, mf->size);
		close(mf->h);
		mf->data = NULL;
		mf->size = 0;
		mf->h = -1;
	}
}


#endif
