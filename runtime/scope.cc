#include "dejavu/runtime/variant.h"
#include "dejavu/runtime/error.h"
#include <unordered_map>

struct string_hash {
public:
	size_t operator()(const string &s) const {
		size_t result = 2166136261;
		for (size_t i = 0; i < s.length; i++) {
			result = (result * 16777619) ^ s.data[i];
		}
		return result;
	}
};

struct scope : public std::unordered_map<string, var, string_hash> {};

static scope global;

extern "C" var *lookup(scope *self, scope *other, double id, string name) {
	// todo: globalvar

	scope *s = 0;
	switch ((int)id) {
	case -1: s = self; break;
	case -2: s = other; break;
	case -5: s = &global; break;

	// todo: check on all.foo
	case -3: case -4: show_error("variable does not exist", true); return 0;
	case -6: show_error("local is not supported", true); return 0;

	// todo: other instance access
	default: return 0;
	}

	// todo: uninitialized vars
	return &(*s)[name];
}

// todo: resizing
extern "C" variant *access(var *a, double x, double y) {
	if (x < 0 || x >= a->x) {
		show_error("index out of bounds", true);
		return 0;
	}

	if (y < 0 || y >= a->y) {
		show_error("index_out of bounds", true);
		return 0;
	}

	return &a->contents[static_cast<int>(x + y * a->x)];
}
