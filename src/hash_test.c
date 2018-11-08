#include "cutils/hash.h"
#include "cutils/test.h"

int main(int argc, const char *argv[]) {
	start_test(argc, argv);

	struct {
		hash_t h;
		uint32_t *keys;
	} uu = { 0 };

	int added;
	size_t ii = INSERT_SET(&uu, 3, &added);
	EXPECT_GT(uu.h.end, ii);
	EXPECT_EQ(1, added);
	EXPECT_EQ(1, uu.h.size);
	EXPECT_EQ(ii, FIND_SET(&uu, 3));

	for (uint32_t key = 16; key < 32; key++) {
		INSERT_SET(&uu, key, &added);
		EXPECT_EQ(1, added);
	}

	EXPECT_GT(uu.h.end, FIND_SET(&uu, 3));
	EXPECT_EQ(uu.h.end, FIND_SET(&uu, 4));
	for (uint32_t key = 16; key < 32; key++) {
		EXPECT_GT(uu.h.end, FIND_HASH(&uu, key));
		EXPECT_EQ(key, uu.keys[FIND_HASH(&uu, key)]);
	}

	FREE_SET(&uu);
	return finish_test();
}
