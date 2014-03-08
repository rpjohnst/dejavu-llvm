#ifndef STRING_H
#define STRING_H

#include <dejavu/system/table.h>
#include <cstring>

struct string final {
	string() = delete;
	string(const string&) = delete;
	string &operator=(const string&) = delete;

	string(size_t l, const char *d);
	static void *operator new(size_t s, size_t len = 0) {
		return ::operator new(s + len);
	}

	unsigned int hash;
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
	struct entry {
		size_t refcount;
		string_pool *pool;
		string s;

		entry(size_t l, const char *d) : s(l, d) {}
		static void *operator new(size_t s, size_t len = 0) {
			return string::operator new(s, len);
		}
	};

	struct empty {};

	struct hash {
		size_t operator()(const entry *e) {
			return ::hash<string*>()(&e->s);
		}
	};

	struct equal {
		bool operator()(entry *a, entry *b) {
			return
				a->s.hash == b->s.hash && a->s.length == b->s.length &&
				memcmp(a->s.data, b->s.data, a->s.length) == 0;
		}
	};

	typedef table<entry*, empty, hash, equal> string_table;

public:
	class ptr {
	public:
		ptr() : s(nullptr) {}
		explicit ptr(entry *n) : s(n) {
			if (s) ++s->refcount;
		}

		ptr(const ptr &that) : s(that.s) {
			if (s) ++s->refcount;
		}
		ptr &operator=(const ptr &that) {
			if (s != that.s) {
				clear();
				s = that.s;
				if (s) ++s->refcount;
			}
			return *this;
		}
		~ptr() { clear(); }

		const string &operator*() const { return s->s; }
		bool operator==(const ptr &that) const { return s == that.s; }
		bool operator!=(const ptr &that) const { return s != that.s; }

	private:
		void clear() {
			if (!s) return;
			if (--s->refcount == 0) {
				delete s;
				s->pool->pool.remove(s);
			}
			s = 0;
		}

		entry *s;
	};

	ptr intern(const char *str) { return intern(str, strlen(str)); }
	ptr intern(const char *str, size_t len);

	size_t size() { return pool.size(); }
	bool empty() { return pool.empty(); }

private:
	string_table pool;
};

#endif
