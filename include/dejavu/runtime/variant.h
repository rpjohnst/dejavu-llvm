#ifndef RUNTIME_VARIANT_H
#define RUNTIME_VARIANT_H

#include <cstring>

struct string {
	string() = default;

	template <int l>
	string(const char (&d)[l]) : length(l), data(d) {}

	size_t length;
	const char *data;
};

inline bool operator==(string a, string b) {
	return a.length == b.length && memcmp((void*)a.data, (void*)b.data, a.length) == 0;
}

struct variant {
	variant() = default;

	variant(double r) : type(0), real(r) {}
	variant(string s) : type(1), string(s) {}

	template <int l>
	variant(const char (&d)[l]) : type(1), string(d) {}

	unsigned char type;
	union {
		double real;
		string string;
	};
};

struct var {
	unsigned short x, y;
	variant *contents;
};

extern "C" {
	double to_real(variant a);
	string to_string(variant a);
}

#endif
