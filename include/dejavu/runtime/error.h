#ifndef RUNTIME_ERROR_H
#define RUNTIME_ERROR_H

#include "dejavu/runtime/variant.h"

struct scope;

extern "C" variant show_error(
	scope *self, scope *other, variant msg, variant abort
);

#endif
