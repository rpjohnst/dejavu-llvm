#ifndef FILE_H
#define FILE_H

#include "dejavu/system/buffer.h"

class file_buffer : public buffer {
public:
	file_buffer() : buffer(0, 0) {}
	~file_buffer();

	int open_file(const char *path);
};

#endif
