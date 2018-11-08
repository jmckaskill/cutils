#pragma once
#include "cutils/stream.h"
#include <time.h>

struct zip_writer;

struct zip_writer *new_zip_writer(FILE *w);
void free_zip_writer(struct zip_writer *z);
int finish_zip(struct zip_writer *z);
int write_zip_file(struct zip_writer *z, stream *source, const char *name, const struct tm *mtime);

