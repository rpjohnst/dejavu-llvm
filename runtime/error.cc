#include "dejavu/runtime/error.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

struct scope;

extern "C" variant show_error(scope *self, scope *other, signed char n, ...) {
	if (n != 2) show_error(
		NULL, NULL, 2,
		variant("Wrong number of arguments to show_error"), variant(true)
	);
	va_list va;
	va_start(va, n);
		variant msg = va_arg(va, variant);
		variant abort = va_arg(va, variant);
	va_end(va);

	fputs("error: ", stderr);
	string error = to_string(msg);
	fwrite(error.data, 1, error.length, stderr);
	fputs("\n", stderr);

	if (to_real(abort)) exit(1);
	return variant(0);
}
