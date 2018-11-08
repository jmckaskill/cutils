#pragma once
#include "cutils/mersenne-twister.h"
#include "cutils/str.h"
#include "cutils/vector.h"
#include <stdio.h>

typedef struct str16 str16_t;
struct str16 {
	uint16_t *v;
	size_t size, cap;
};

int append_windows_path(str16_t *s, const char *path);
FILE *io_tmpfile(struct mt_rand *r);
FILE *io_fopen(const char *u8path, const char *mode);

typedef int(*scandir_cb)(void *user, const char *path);
int io_scandir(const char *dir, scandir_cb cb, void *user);

void io_mkdir(const char *dir);

typedef struct mapped_file mapped_file;
struct mapped_file {
	intptr_t h;
	const uint8_t *data;
	size_t size;
};

int map_file(mapped_file *mf, const char *fn);
void unmap_file(mapped_file *mf);
