#include <dejavu/system/table.h>
#include <gtest/gtest.h>

template<>
struct hash<int> {
	size_t operator()(int x) { return x; }
};

template<>
struct equal<int> {
	bool operator()(int x, int y) { return x == y; }
};

TEST(table, empty) {
	table<int, int> t;
	EXPECT_EQ(t.end(), t.find(0));
}

TEST(table, insert) {
	table<int, int> t(3);

	t.insert(3) = 1;
	t.insert(5) = 2;
	t.insert(7) = 3;

	EXPECT_EQ(1, t.find(3)->v);
	EXPECT_EQ(2, t.find(5)->v);
	EXPECT_EQ(3, t.find(7)->v);
}

TEST(table, resize) {
	table<int, int> t(1);

	t.insert(3) = 1;
	t.insert(5) = 2;

	EXPECT_EQ(1, t.find(3)->v);
	EXPECT_EQ(2, t.find(5)->v);
}

TEST(table, chain) {
	table<int, int> t(2);

	t.insert(2) = 1;
	t.insert(4) = 2;

	EXPECT_EQ(1, t.find(2)->v);
	EXPECT_EQ(2, t.find(4)->v);
}

TEST(table, subscript) {
	table<int, int> t;

	t[3] = 1;
	t[5] = 2;
	t[7] = 3;

	EXPECT_EQ(1, t[3]);
	EXPECT_EQ(2, t[5]);
	EXPECT_EQ(3, t[7]);
}
