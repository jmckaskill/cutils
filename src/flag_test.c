#include "cutils/flag.h"
#include "cutils/str.h"
#include "cutils/test.h"

static str_t g_lastmsg = STR_INIT;
static int g_code;

static void my_exit(int code, const char *msg) {
	str_set(&g_lastmsg, msg);
	g_code = code;
}

int main(int argc, const char *argv[]) {
	start_test(argc, argv);

	bool b = false;
	int i = 34;
	const char *str = "default";

	flag_exit = &my_exit;

	// test the help
	flag_bool(&b, 'b', "bool", "bool usage");
	flag_int(&i, 0, "int", "N", "int usage");
	flag_string(&str, 's', NULL, "S", "string usage");

	const char *args1[] = { "foo", "-h", NULL };
	int argc1 = 2;
	char **argv1 = flag_parse(&argc1, args1, "[options]", 0);
	EXPECT_EQ(FLAG_EXIT_HELP, g_code);
	EXPECT_STREQ(
		"usage: foo [options]\n"
		"\n"
		"options:\n"
		"  -b, --bool, --no-bool         bool usage\n"
		"  --int=N                       int usage [default=34]\n"
		"  -s S                          string usage [default=default]\n",
		g_lastmsg.c_str);
	free(argv1);
	g_code = 0;

	// test the parsing
	flag_bool(&b, 'b', "bool", "bool usage");
	flag_int(&i, 0, "int", "N", "int usage");
	flag_string(&str, 's', NULL, "S", "string usage");

	const char *args2[] = { "foo", "-b", "--int=3", "argument", "-s", "foobar", NULL };
	int argc2 = 6;
	char **argv2 = flag_parse(&argc2, args2, "[arguments]", 0);
	EXPECT_EQ(0, g_code);
	EXPECT_EQ(1, argc2);
	EXPECT_STREQ("argument", argv2[0]);
	EXPECT_EQ(true, b);
	EXPECT_EQ(i, 3);
	EXPECT_STREQ(str, "foobar");
	free(argv2);

	str_destroy(&g_lastmsg);
	return finish_test();
}
