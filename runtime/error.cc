#include "dejavu/runtime/error.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

struct scope;

extern "C" variant show_error(
	scope *, scope *, variant msg, variant abort
) {
	fputs("error: ", stderr);
	string error = to_string(msg);
	fwrite(error.data, 1, error.length, stderr);
	fputs("\n", stderr);

	if (to_real(abort)) exit(1);
	return 0;
}
