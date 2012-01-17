#include "dejavu/runtime/error.h"
#include <cstdio>
#include <cstdlib>

extern "C" void show_error(variant msg, variant abort) {
	string error = to_string(msg);
	fprintf(stderr, "error: %.*s\n", error.length, error.data);

	if (to_real(abort)) exit(1);
}
