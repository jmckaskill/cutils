#include "cutils/str.h"
#include "cutils/test.h"
#include "cutils/path.h"

static void test_path(enum path_type type, const char *in, const char *out) {
	bool same = !strcmp(in, out);
	str_t s = STR_INIT;
	str_set(&s, in);
	EXPECT_EQ(same ? 0 : 1, clean_path(type, s.c_str, &s.len));
	EXPECT_STREQ(out, s.c_str);
	EXPECT_EQ(s.len, strlen(out));
	str_destroy(&s);
}

int main(int argc, const char *argv[]) {
	start_test(argc, argv);

	str_t s = STR_INIT;
	str_set(&s, "foo bar foo bar");
	str_replace_all(&s, "foo", "abcde");
	EXPECT_STREQ("abcde bar abcde bar", s.c_str);
	str_replace_all(&s, "bar", "a");
	EXPECT_STREQ("abcde a abcde a", s.c_str);
	str_replace_all(&s, "abcde", "ghijk");
	EXPECT_STREQ("ghijk a ghijk a", s.c_str);

	test_path(PATH_UNIX, "/../foo", "/foo");
	test_path(PATH_UNIX, "../foo", "../foo");
	test_path(PATH_UNIX, "a/b/c/../../foo", "a/foo");
	test_path(PATH_UNIX, "../../foo", "../../foo");
	test_path(PATH_UNIX, "/../../../foo", "/foo");
	test_path(PATH_UNIX, "/foo/../foo", "/foo");
	test_path(PATH_UNIX, "/./foo", "/foo");
	test_path(PATH_UNIX, "/foo/.", "/foo/");
	test_path(PATH_UNIX, "/foo/./", "/foo/");
	test_path(PATH_UNIX, "//foo//./", "/foo/");
	test_path(PATH_UNIX, "../foo/.", "../foo/");
	test_path(PATH_UNIX, "foo/../", ".");
	test_path(PATH_UNIX, "", "");
	test_path(PATH_UNIX, ".", ".");
	test_path(PATH_UNIX, "./", ".");
	test_path(PATH_UNIX, "/", "/");
	test_path(PATH_UNIX, "..", "..");
	test_path(PATH_UNIX, "../", "..");
	test_path(PATH_UNIX, "../../", "../..");
	test_path(PATH_UNIX, "../..", "../..");
	test_path(PATH_WINDOWS, "C:\\", "C:/");
	test_path(PATH_WINDOWS, "C:\\foo\\", "C:/foo/");
	test_path(PATH_WINDOWS, "C:\\..\\foo", "C:/foo");
	test_path(PATH_WINDOWS, "C:\\..\\foo\\.", "C:/foo/");

	EXPECT_STREQ("bar/", path_last_segment("foo/bar/"));
	EXPECT_STREQ("foo", path_last_segment("foo"));
	EXPECT_STREQ("bar", path_last_segment("foo/bar"));

	EXPECT_PTREQ(NULL, path_file_extension("foo"));
	EXPECT_PTREQ(NULL, path_file_extension("foo.h/bar"));
	EXPECT_STREQ(".h", path_file_extension("foo.c/bar.h"));

	str_destroy(&s);
	return finish_test();
}
