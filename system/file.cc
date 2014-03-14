#include <dejavu/system/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

int file_buffer::open_file(const char *path) {
	int fd = open(path, O_RDONLY);
	if (fd == -1)
		return errno;

	struct stat s;
	if (fstat(fd, &s) == -1) {
		close(fd);
		return errno;
	}
	length = s.st_size;

	data = static_cast<char*>(mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0));
	if (data == MAP_FAILED) {
		close(fd);
		return errno;
	}

	close(fd);
	return 0;
}

file_buffer::~file_buffer() {
	munmap(const_cast<char*>(data), length);
}
