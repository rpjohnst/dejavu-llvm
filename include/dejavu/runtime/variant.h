#ifndef RUNTIME_VARIANT_H
#define RUNTIME_VARIANT_H

#include <dejavu/system/string.h>

extern string_pool strings;

struct variant {
	variant() = default;

	variant(double r) : type(0), real(r) {}
	variant(string *s) : type(1), string(s) {}

	variant(const char *s) : variant(strings.intern(s)) {}

	// todo: remove magic numbers
	unsigned char type;
	union {
		double real;
		string *string;
	};
};

struct var {
	unsigned short x, y;
	variant *contents;
};

extern "C" {
	double to_real(const variant &a);
	string *to_string(const variant &a);

	string *intern(string *s) __attribute__((pure));

	variant *access(var *a, unsigned short x, unsigned short y);

	void retain(variant *a);
	void release(variant *a);

	void retain_var(var *a);
	void release_var(var *a);
}

#endif
