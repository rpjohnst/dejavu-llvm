#ifndef FILE_H
#define FILE_H

#include <stdexcept>
#include <fcntl.h>
#include "disallow_copy.h"

class file_open_error : public std::exception {
public:
	explicit file_open_error(int e) : std::exception(), error(e) {}

private:
	int error;
};

class file_descriptor {
public:
	file_descriptor(const char *path, int flags);
	~file_descriptor();
	
	operator int() const { return fd; } 

private:
	int fd;
	
	DISALLOW_COPY(file_descriptor);
};

class file_buffer {
public:
	explicit file_buffer(const file_descriptor& fd);
	~file_buffer();
	
	const char& operator[](size_t i) const { return buffer[i]; }
	
	const char *begin() const { return &buffer[0]; }
	const char *end() const { return &buffer[length]; }
	
private:
	size_t length;
	char *buffer;
	
	DISALLOW_COPY(file_buffer);
};

#endif
