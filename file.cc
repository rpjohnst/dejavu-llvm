#include "file.h"
#include <cerrno>
#include <sys/mman.h>
#include <sys/stat.h>

file_descriptor::file_descriptor(const char *path, int flags) : fd(open(path, flags)) {
	if (fd == -1)
		throw file_open_error(errno);
}

file_descriptor::~file_descriptor() {
	close(fd);
}

file_buffer::file_buffer(const file_descriptor& fd) {
	struct stat s;
	if (fstat(fd, &s) == -1)
		throw file_open_error(errno);
	length = s.st_size;
	
	buffer = static_cast<char*>(mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0));
	if (buffer == MAP_FAILED)
		throw file_open_error(errno);
}

file_buffer::~file_buffer() {
	munmap(buffer, length);
}
