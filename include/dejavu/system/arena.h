#ifndef ARENA_H
#define ARENA_H

#include <cstddef>

class arena {
public:
	arena(size_t slab_size = 4096);
	~arena();

	void *allocate(size_t s);
	void reset();

private:
	struct slab {
		size_t size;
		slab *next;
	};

	void new_slab();

	size_t slab_size;
	slab *current_slab;
	char *current;
	char *end;
	size_t bytes_allocated;

	size_t offset = 0;
};

inline void *operator new (size_t size, arena &a) {
	return a.allocate(size);
}

#endif
