#include <dejavu/system/string.h>
#include <gtest/gtest.h>
#include <memory>

static const char t[] = "abcde";
static const size_t l = sizeof(t) - 1;

TEST(string, allocate) {
	string *s = new (l) string(l, t);
	s->retain();

	EXPECT_EQ(0, memcmp(s->data, t, l));
}

TEST(string, intern) {
	string_pool pool;

	string *p1 = pool.intern("hello");
	p1->retain();
	string *p2 = pool.intern("hello");
	p2->retain();
	string *p3 = pool.intern("world");
	p3->retain();

	EXPECT_EQ(2, pool.size());
	EXPECT_EQ(p1, p2);
	EXPECT_NE(p1, p3);

	p1->release();
	p2->release();
	p3->release();

	EXPECT_TRUE(pool.empty());
}

TEST(string, use) {
	string_pool pool;

	string *str = new (l) string(l, t);
	str->retain();

	EXPECT_EQ(str, pool.intern(str));

	str->release();
}
