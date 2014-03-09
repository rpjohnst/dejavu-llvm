#ifndef STRING_H
#define STRING_H

#include <dejavu/system/table.h>
#include <cstring>

class string_pool;

struct string final {
	string() = delete;
	string(const string&) = delete;
	string &operator=(const string&) = delete;

	string(size_t l) : length(l) {}
	string(size_t l, const char *d);
	static void *operator new(size_t s, size_t len = 0) {
		return ::operator new(s + len);
	}

	void retain() { refcount++; }
	void release();

	static size_t compute_hash(size_t l, const char *data);

	size_t refcount = 0;
	string_pool *pool;

	size_t hash;
	size_t length;
	char data[];
};

template<>
struct hash<string*> {
	size_t operator()(const string *s) const { return s->hash; }
};

template<>
struct equal<string*> {
	size_t operator()(const string *a, const string *b) const {
		return a == b;
	}
};

class string_pool {
	friend struct string;

	struct empty {};

	struct equal {
		bool operator()(const string *a, const string *b) {
			return
				a->hash == b->hash && a->length == b->length &&
				memcmp(a->data, b->data, a->length) == 0;
		}
	};

	typedef table<string*, empty, hash<string*>, equal> string_table;

public:
	string *intern(const char *str) {
		return intern(str, strlen(str));
	}
	string *intern(const char *str, size_t len) {
		return intern(new (len) string(len, str));
	}
	string *intern(string *str);

	size_t size() { return pool.size(); }
	bool empty() { return pool.empty(); }

private:
	string_table pool;
};

inline void string::release() {
	if (--refcount == 0) {
		pool->pool.remove(this);
		delete this;
	}
}

#endif
