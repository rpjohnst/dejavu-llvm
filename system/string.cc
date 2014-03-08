#include <dejavu/system/string.h>
#include <memory>

string::string(size_t l, const char *d) : length(l) {
	memcpy(data, d, length);

	hash = l;
	size_t step = (l >> 5) + 1; // don't hash all chars of a long string
	for (; l >= step; l -= step)
		hash ^= (hash << 5) + (hash >> 2) + (unsigned char)data[l - 1];
}

string_pool::ptr string_pool::intern(const char *str, size_t len) {
	std::unique_ptr<entry> s(new (len) entry(len, str));

	string_table::node *n = pool.find(s.get());
	if (n != pool.end()) {
		return ptr(n->k);
	}

	pool.insert(s.get());
	s->refcount = 0;
	s->pool = this;
	return ptr(s.release());
}
