#include "file.h"
#include <cerrno>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int file_buffer::open_file(const char *path, int flags) {
	int fd = open(path, flags);
	if (fd == -1)
		return errno;
	
	struct stat s;
	if (fstat(fd, &s) == -1) {
		close(fd);
		return errno;
	}
	length = s.st_size;
	
	buffer = static_cast<char*>(mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0));
	if (buffer == MAP_FAILED) {
		close(fd);
		return errno;
	}
	
	close(fd);
	return 0;
}

file_buffer::~file_buffer() {
	munmap(buffer, length);
}
