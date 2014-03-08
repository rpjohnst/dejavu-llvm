#include <dejavu/system/string.h>
#include <gtest/gtest.h>
#include <memory>

TEST(string, allocate) {
	const char *t = "abcde";
	const size_t l = strlen(t);

	std::unique_ptr<string> s(new (l) string(l, t));

	EXPECT_EQ(0, memcmp(s->data, t, l));
}

TEST(string, intern) {
	string_pool pool;

	{
		string_pool::ptr p1 = pool.intern("hello");
		string_pool::ptr p2 = pool.intern("hello");
		string_pool::ptr p3 = pool.intern("world");

		EXPECT_EQ(2, pool.size());
		EXPECT_EQ(p1, p2);
		EXPECT_NE(p1, p3);
	}

	EXPECT_TRUE(pool.empty());
}
