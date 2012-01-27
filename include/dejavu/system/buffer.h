#ifndef BUFFER_H
#define BUFFER_H

#include <cassert>
#include <cstddef>

class buffer {
public:
	buffer(size_t len, char *data) : length(len), data(data) {}

	const char &operator[](size_t i) const {
		assert(i < length && "index out of range");
		return data[i];
	}

	const char *begin() const { return &data[0]; }
	const char *end() const { return &data[length]; }

protected:
	size_t length;
	char *data;
};

#endif
