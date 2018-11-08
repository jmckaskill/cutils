#include "cutils/str.h"
#include "cutils/test.h"

static void test_path(const char *in, const char *out) {
	str_t s = STR_INIT;
	str_set(&s, in);
	str_clean_path(&s);
	EXPECT_STREQ(out, s.c_str);
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

	test_path("/../foo", "/foo");
	test_path("../foo", "../foo");
	test_path("a/b/c/../../foo", "a/foo");
	test_path("../../foo", "../../foo");
	test_path("/../../../foo", "/foo");
	test_path("/foo/../foo", "/foo");
	test_path("/./foo", "/foo");
	test_path("/foo/.", "/foo/");
	test_path("/foo/./", "/foo/");
	test_path("//foo//./", "/foo/");
	test_path("../foo/.", "../foo/");
	test_path(".", "./");
	test_path("./", "./");
	test_path("/", "/");
	test_path("../", "../");
	test_path("..", "../");
#ifdef WIN32
	test_path("C:\\", "C:/");
	test_path("C:\\foo\\", "C:/foo/");
	test_path("C:\\..\\foo", "C:/foo");
	test_path("C:\\..\\foo\\.", "C:/foo/");
#endif

	EXPECT_STREQ("bar/", path_last_segment("foo/bar/"));
	EXPECT_STREQ("foo", path_last_segment("foo"));
	EXPECT_STREQ("bar", path_last_segment("foo/bar"));

	EXPECT_PTREQ(NULL, path_file_extension("foo"));
	EXPECT_PTREQ(NULL, path_file_extension("foo.h/bar"));
	EXPECT_STREQ(".h", path_file_extension("foo.c/bar.h"));

	str_destroy(&s);
	return finish_test();
}
