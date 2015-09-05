#include <dejavu/runtime/string.h>
#include <dejavu/runtime/error.h>
#include <cstdio>
#include <cfloat>

struct scope;

extern "C" variant string(
	scope *, scope *, const variant &val
) {
	switch (val.type) {
	case 0: {
		int length = snprintf(nullptr, 0, "%f", val.real) - 1; // we don't want the \0
		struct string *str = new (length) struct string(length);

		snprintf(str->data, length, "%f", val.real);
		str->hash = string::compute_hash(str->length, str->data);

		struct string *ret = strings.intern(str);
		ret->retain();
		return ret;
	}

	case 1: return val;

	default: return show_error(0, 0, "bad value", true);
	}
}
