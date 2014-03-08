#ifndef TABLE_H
#define TABLE_H

#include <cstddef>

template <class k>
struct hash;

template <class k>
struct equal;

/*
 * Lua-style hash table- chained scatter table with Brent's variation
 * Brent's variation avoids coalescing by relocating colliding keys that are
 * not in their main position
 */
template <
	class key, class value, class hash = hash<key>, class equal = equal<key>
>
class table {
public:
	struct node {
		key k;
		value v;
		node *next;
	};

	table(size_t s = 1) { resize(s); }
	~table() { delete[] contents; }

	value &operator[](const key &k) {
		node *p = find(k);
		if (p != end()) return p->v;
		else return insert(k);
	}

	// don't insert an already-existing node
	value &insert(const key &k) {
		node *main = main_position(k);

		if (!is_empty(main)) {
			node *free = find_free();
			if (free == end()) {
				resize(length * 2);
				return insert(k);
			}

			node *other = main_position(main->k);
			if (other != main) {
				while (other->next != main) other = other->next;
				other->next = free;
				*free = *main;
				main->next = nullptr;
			}
			else {
				free->next = main->next;
				main->next = free;
				main = free;
			}
		}
		else {
			main->next = nullptr;
		}

		count++;
		main->k = k;
		main->v = value();
		return main->v;
	}

	node *find(const key &k) {
		node *n = main_position(k);

		if (is_empty(n))
			return end();

		do {
			if (equal()(k, n->k))
				return n;
			else
				n = n->next;
		} while (n);

		return end();
	}

	void remove(const key &k) {
		node *n = find(k);
		if (n == end())
			return;

		if (n->next) {
			node *next = n->next;
			*n = *next;

			next->next = end();
			if (next > lastfree)
				lastfree = next + 1;
		}
		else {
			n->next = end();
		}

		count--;
	}

	node *end() {
		return contents + length;
	}

	size_t size() { return count; }
	bool empty() { return size() == 0; }
	size_t capacity() { return length; }

private:
	void resize(size_t s) {
		node *old_contents = contents;
		size_t old_length = length;

		length = s;
		contents = new node[length];
		lastfree = end();
		for (size_t i = 0; i < length; i++) {
			contents[i].next = end();
		}

		size_t old_count = count;
		for (size_t i = 0; i < old_length; i++) {
			node *old = &old_contents[i];
			if (!is_empty(old)) {
				(*this)[old->k] = old->v;
			}
		}
		count = old_count;

		delete[] old_contents;
	}

	node *main_position(const key &k) {
		return &contents[hash()(k) % length];
	}

	bool is_empty(node *n) {
		return n->next == end();
	}

	node *find_free() {
		while (lastfree > contents) {
			lastfree--;
			if (is_empty(lastfree))
				return lastfree;
		}
		return end();
	}

	node *contents = nullptr;
	size_t length = 0;

	node *lastfree;
	size_t count = 0;
};

#endif
