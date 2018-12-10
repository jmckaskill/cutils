#pragma once
#include <stdlib.h>
#include <stdbool.h>

enum path_type {
	PATH_WINDOWS,
	PATH_UNIX,
#ifdef WIN32
	PATH_NATIVE = PATH_WINDOWS,
#else
	PATH_NATIVE = PATH_UNIX,
#endif
};

int clean_path(enum path_type type, char *s, size_t *len);

// Returns the file extension of a file path. The path must be cleaned first.
// Will return NULL (no extension) or a pointer into the string.
// The pointer can then be use to strip the extension using str_setend.
const char *path_file_extension(const char *s);
const char *path_file_extension2(const char *s, size_t len);

// Returns the last segment in the provided path.
// This is a pointer into the source string and can be used with str_setend
// to strip the last segment or str_add to copy it into a new string.
const char *path_last_segment(const char *s);

// Returns whether the path is a directory by seeing if it ends in a slash.
// Path must be cleaned first.
bool path_is_directory(const char *s);

