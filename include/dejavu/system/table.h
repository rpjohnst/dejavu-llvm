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
	struct node;

public:
	table(size_t s = 1) { resize(s); }
	~table() { delete[] contents; }

	value &operator[](const key &k) {
		node *p = find(k);
		if (p != end()) return p->v;
		else return insert(k);
	}

	value &insert(const key &k) {
		node *main = main_position(k);

		if (!is_empty(main)) {
			node *free = find_free();
			if (free == end()) {
				resize(size * 2);
				return (*this)[k];
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

		main->k = k;
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

	node *end() {
		return contents + size;
	}

private:
	void resize(size_t s) {
		node *old_contents = contents;
		size_t old_size = size;

		size = s;
		contents = new node[size];
		lastfree = end();
		for (size_t i = 0; i < size; i++) {
			contents[i].next = end();
		}

		for (size_t i = 0; i < old_size; i++) {
			node *old = &old_contents[i];
			if (!is_empty(old)) {
				(*this)[old->k] = old->v;
			}
		}

		delete[] old_contents;
	}

	node *main_position(const key &k) {
		return &contents[hash()(k) % size];
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

	struct node {
		key k;
		value v;
		node *next;
	};

	node *contents = nullptr;
	size_t size = 0;

	node *lastfree;
};

#endif
