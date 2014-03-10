#ifndef RUNTIME_ERROR_H
#define RUNTIME_ERROR_H

#include "dejavu/runtime/variant.h"

struct scope;

extern "C" variant show_error(
	scope *self, scope *other, const variant &msg, const variant &abort
);

#endif
