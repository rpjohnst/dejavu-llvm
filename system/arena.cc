#include <dejavu/system/arena.h>
#include <cassert>

arena::arena(size_t slab_size) :
	slab_size(slab_size), current_slab(NULL), bytes_allocated(0) {}

arena::~arena() {
	while (current_slab) {
		slab *next = current_slab->next;
		::operator delete(current_slab);
		current_slab = next;
	}
}

void *arena::allocate(size_t size) {
	if (!current_slab) new_slab();
	bytes_allocated += size;

	char *p = current; // TODO: align
	if (p + size <= end) {
		current = p + size;
		return p;
	}

	new_slab();
	p = current; // TODO: align
	current = p + size;
	assert(current <= end && "Unable to allocate memory");
	return p;
}

void arena::new_slab() {
	slab *s = static_cast<slab*>(::operator new(slab_size));
	s->size = slab_size;
	s->next = current_slab;

	current_slab = s;
	current = reinterpret_cast<char*>(current_slab + 1);
	end = reinterpret_cast<char*>(current_slab) + current_slab->size;
}
