#ifndef SCOPE_H
#define SCOPE_H

#include <dejavu/runtime/variant.h>
#include <dejavu/system/table.h>

struct scope : public table<string*, var> {};

#endif
