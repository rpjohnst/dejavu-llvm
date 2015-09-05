#ifndef RUNTIME_STRING_H
#define RUNTIME_STRING_H

#include <dejavu/runtime/variant.h>

struct scope;

extern "C" variant string(
	scope *self, scope *other, const variant &val
);

#endif
