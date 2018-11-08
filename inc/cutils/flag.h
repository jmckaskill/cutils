#pragma once

#include "cutils/str.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FLAG_EXIT_HELP 1
#define FLAG_EXIT_UNKNOWN_FLAG 2
#define FLAG_EXIT_MISSING_VALUE 3
#define FLAG_EXIT_INSUFFICIENT_ARGS 4

typedef void(*flag_exit_fn)(int code, const char *msg);
extern flag_exit_fn flag_exit;

void flag_bool(bool *p, char shopt, const char *longopt, const char *usage);
void flag_int(int *p, char shopt, const char *longopt, const char *arg, const char *usage);
void flag_double(double *p, char shopt, const char *longopt, const char *arg, const char *usage);
void flag_string(const char **p, char shopt, const char *longopt, const char *arg, const char *usage);
void flag_path(str_t *p, char shopt, const char *longopt, const char *usage);
char **flag_parse(int *argc, const char **argv, const char *usage, int minargs);

#ifdef __cplusplus
}
#endif
