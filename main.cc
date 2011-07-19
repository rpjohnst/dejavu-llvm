#include <cstdio>
#include "file.h"

int main() {
	file_buffer file(file_descriptor("test.gml", O_RDONLY));
	printf("\n");
}

