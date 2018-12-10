#include "cutils/flag.h"
#include "cutils/str.h"
#include "cutils/utf.h"
#include "cutils/path.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include <Windows.h>
#pragma comment(lib, "Shell32.lib")
#endif

enum flag_type {
	FLAG_BOOL,
	FLAG_INT,
	FLAG_DOUBLE,
	FLAG_STRING,
	FLAG_PATH,
};

union all_types {
	bool b;
	const char *s;
	int i;
	double d;
	str_t str;
};

struct flag {
	enum flag_type type;
	union all_types *pval;
	char shopt;
	const char *longopt;
	size_t longlen;
	const char *arg;
	const char *usage;
};

static const char *g_usage;
static const char *g_argv0;
static struct flag *g_flags;
static int g_num;
static int g_cap;

static void normal_exit(int code, const char *msg) {
	fputs(msg, stderr);
	exit(code);
}

flag_exit_fn flag_exit = &normal_exit;

static void print_spaces(str_t *o, int num) {
	while (num > 0) {
		str_addch(o, ' ');
		num--;
	}
}

static void print_usage(str_t *o) {
	str_addf(o, "usage: %s %s\n", g_argv0, g_usage);
	if (g_num) {
		str_add(o, "\noptions:\n");
		for (int i = 0; i < g_num; i++) {
			struct flag *f = &g_flags[i];
			int pad = 32;

			if (f->type == FLAG_BOOL) {
				if (f->shopt && f->longopt) {
					pad -= str_addf(o, "  -%c, --%s, --no-%s  ", f->shopt, f->longopt, f->longopt);
				} else if (f->longopt) {
					pad -= str_addf(o, "  --%s, --no-%s  ", f->longopt, f->longopt);
				} else {
					pad -= str_addf(o, "  -%c  ", f->shopt);
				}
			} else {
				if (f->shopt && f->longopt) {
					pad -= str_addf(o, "  -%c %s, --%s=%s  ", f->shopt, f->arg, f->longopt, f->arg);
				} else if (f->longopt) {
					pad -= str_addf(o, "  --%s=%s  ", f->longopt, f->arg);
				} else {
					pad -= str_addf(o, "  -%c %s  ", f->shopt, f->arg);
				}
			}

			print_spaces(o, pad);
			str_add(o, f->usage);

			switch (f->type) {
			case FLAG_INT:
				str_addf(o, " [default=%d]", f->pval->i);
				break;
			case FLAG_DOUBLE:
				str_addf(o, " [default=%g]", f->pval->d);
				break;
			case FLAG_STRING:
				if (f->pval->s) {
					str_addf(o, " [default=%s]", f->pval->s);
				}
				break;
			case FLAG_PATH:
				if (f->pval->str.len) {
					str_addf(o, " [default=%s]", f->pval->str.c_str);
				}
				break;
			case FLAG_BOOL:
				if (f->pval->b) {
					str_addf(o, " [default=enabled]");
				}
				break;
			}

			str_addch(o, '\n');
		}
	}
}

static void flag_error(int code, const char *fmt, ...) {
	str_t o = STR_INIT;
	va_list ap;
	va_start(ap, fmt);
	str_vaddf(&o, fmt, ap);
	if (o.len) {
		str_addch(&o, '\n');
	}
	print_usage(&o);
	flag_exit(code, o.c_str);
	str_destroy(&o);
}

static void append(enum flag_type type, void *p, char shopt, const char *longopt, const char *arg, const char *usage) {
	if (g_num == g_cap) {
		g_cap = (g_cap + 16) * 3 / 2;
		g_flags = (struct flag*) realloc(g_flags, g_cap * sizeof(*g_flags));
	}
	struct flag *f = &g_flags[g_num++];
	f->type = type;
	f->shopt = shopt;
	f->longopt = longopt;
	f->longlen = longopt ? strlen(longopt) : 0;
	f->arg = arg;
	f->usage = usage;
	f->pval = (union all_types *) p;
}

void flag_bool(bool *p, char shopt, const char *longopt, const char *usage) {
	append(FLAG_BOOL, p, shopt, longopt, NULL, usage);
}

void flag_int(int *p, char shopt, const char *longopt, const char *arg, const char *usage) {
	append(FLAG_INT, p, shopt, longopt, arg, usage);
}

void flag_double(double *p, char shopt, const char *longopt, const char *arg, const char *usage) {
	append(FLAG_DOUBLE, p, shopt, longopt, arg, usage);
}

void flag_string(const char **p, char shopt, const char *longopt, const char *arg, const char *usage) {
	append(FLAG_STRING, (void*) p, shopt, longopt, arg, usage);
}

void flag_path(str_t *p, char shopt, const char *longopt, const char *usage) {
	append(FLAG_PATH, (void*) p, shopt, longopt, "PATH", usage);
}

static struct flag *find_long(const char *name, size_t len) {
	for (int j = 0; j < g_num; j++) {
		struct flag *f = &g_flags[j];
		if (f->longopt && len == f->longlen && !memcmp(name, f->longopt, len)) {
			return f;
		}
	}
	return NULL;
}

static struct flag *find_short(char name) {
	for (int j = 0; j < g_num; j++) {
		struct flag *f = &g_flags[j];
		if (f->shopt == name) {
			return f;
		}
	}
	return NULL;
}

static int process_flag(struct flag *f, const char *arg, const char *str_value, bool bool_value) {
	if (!str_value && f->type != FLAG_BOOL) {
		flag_error(FLAG_EXIT_MISSING_VALUE, "expected value for %s", arg);
		return -1;
	}

	switch (f->type) {
	case FLAG_BOOL:
		f->pval->b = bool_value;
		break;
	case FLAG_INT:
		f->pval->i = strtol(str_value, NULL, 0);
		break;
	case FLAG_DOUBLE:
		f->pval->d = strtod(str_value, NULL);
		break;
	case FLAG_STRING:
		f->pval->s = str_value;
		break;
	case FLAG_PATH:
		str_set(&f->pval->str, str_value);
		clean_path(PATH_NATIVE, f->pval->str.c_str, &f->pval->str.len);
		break;
	}

	return 0;
}

static const char *remove_argument(int i, int *pargc, char **argv) {
	const char *ret = argv[i];
	memmove((void*)&argv[i], (void*)&argv[i + 1], (*pargc - (i + 1)) * sizeof(argv[i]));
	(*pargc)--;
	return ret;
}

static void unknown_flag(const char *arg) {
	flag_error(FLAG_EXIT_UNKNOWN_FLAG, "unknown flag %s", arg);
}

// On windows int main() uses the local charset. We could use
// wmain and convert, but that requires every entry point to be
// conditionally renamed. Instead we use the standard main function
// and then reparse from the source when the user calls flag_parse into UTF-8.
static char **reparse_command_line(int *pargc, const char **argv) {
#ifdef WIN32
	if (flag_exit == &normal_exit) {
		wchar_t *cmdline = GetCommandLineW();
		wchar_t **wargs = CommandLineToArgvW(cmdline, pargc);
		size_t memsz = (*pargc + 1) * sizeof(char*);
		for (int i = 0; i < *pargc; i++) {
			memsz += UTF16LE_to_UTF8_size(2*wcslen(wargs[i])) + 1;
		}
		char **cargs = malloc(memsz);
		char *mem = (char*)&cargs[*pargc + 1];
		for (int i = 0; i < *pargc; i++) {
			size_t clen = UTF16LE_to_UTF8(mem, wargs[i], 2*wcslen(wargs[i]));
			cargs[i] = mem;
			mem[clen] = 0;
			mem += clen + 1;
		}
		LocalFree(wargs);
		cargs[*pargc] = NULL;
		return cargs;
	}
#endif

	// on linux, changes to the input array are visible outside the process
	// copy the input argument array, so that any changes we make aren't externally visible
	char **ret = malloc((*pargc + 1) * sizeof(char*));
	for (int i = 0; i < *pargc; i++) {
		ret[i] = (char*)argv[i];
	}
	return ret;
}

char **flag_parse(int *pargc, const char **in_argv, const char *usage, int minargs) {
	char **argv = reparse_command_line(pargc, in_argv);
	g_usage = usage;
	g_argv0 = remove_argument(0, pargc, argv);
	for (int i = 0; i < *pargc;) {
		if (argv[i][0] != '-') {
			i++;
			continue;
		}

		const char *arg = remove_argument(i, pargc, argv);

		if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) {
			flag_error(FLAG_EXIT_HELP, "");
			goto end;
		} else if (!strcmp(arg, "--")) {
			break;
		}

		if (!strncmp(arg, "--no-", 5)) {
			// long form negative
			size_t len = strlen(arg);
			struct flag *f = find_long(arg + 5, len - 5);
			if (!f) {
				unknown_flag(arg);
				goto end;
			}

			if (process_flag(f, arg, NULL, false)) {
				goto end;
			}

		} else if (arg[1] == '-') {
			// long form
			size_t len = strlen(arg);
			const char *value = memchr(arg, '=', len);
			if (value) {
				len = value - arg;
				value++;
			}

			struct flag *f = find_long(arg + 2, len - 2);
			if (!f) {
				unknown_flag(arg);
				goto end;
			}

			if (process_flag(f, arg, value, true)) {
				goto end;
			}

		} else if (arg[1] && !arg[2]) {
			// short form
			struct flag *f = find_short(arg[1]);
			if (!f) {
				unknown_flag(arg);
				goto end;
			}

			const char *value = NULL;
			if (f->type != FLAG_BOOL && i < *pargc) {
				value = remove_argument(i, pargc, argv);
			}

			if (process_flag(f, arg, value, true)) {
				goto end;
			}
		} else {
			unknown_flag(arg);
			goto end;
		}
	}

	if (*pargc < minargs) {
		flag_error(FLAG_EXIT_INSUFFICIENT_ARGS, "expected %d arguments", minargs);
		goto end;
	}

end:
	// re null-terminate the argument list
	argv[*pargc] = NULL;
	free(g_flags);
	g_flags = NULL;
	g_num = 0;
	g_cap = 0;
	return argv;
}

