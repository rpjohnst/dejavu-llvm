#ifndef RUNTIME_ERROR_H
#define RUNTIME_ERROR_H

#include "dejavu/runtime/variant.h"

extern "C" void show_error(variant msg, variant abort);

#endif
