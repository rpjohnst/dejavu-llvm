#ifndef FILE_H
#define FILE_H

#include <fcntl.h>
#include <cassert>

class file_buffer {
public:
	file_buffer() : length(0), buffer(0) {}
	~file_buffer();
	
	int open_file(const char *path, int flags);
	
	const char& operator[](size_t i) const {
		assert(i < length && "index out of range");
		return buffer[i];
	}
	
	const char *begin() const { return &buffer[0]; }
	const char *end() const { return &buffer[length]; }
	
private:
	size_t length;
	char *buffer;
};

#endif
