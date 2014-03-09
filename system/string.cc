#include <dejavu/system/string.h>
#include <memory>

size_t string::compute_hash(size_t l, const char *d) {
	size_t hash = l;
	size_t step = (l >> 5) + 1; // don't hash all chars of a long string
	for (; l >= step; l -= step)
		hash ^= (hash << 5) + (hash >> 2) + (unsigned char)d[l - 1];

	return hash;
}

string::string(size_t l, const char *d)
	: hash(compute_hash(l, d)), length(l) {
	memcpy(data, d, length);
}

string *string_pool::intern(string *str) {
	string_table::node *n = pool.find(str);
	if (n != pool.end()) {
		return n->k;
	}

	pool.insert(str);
	str->pool = this;
	return str;
}
