#ifndef VARIANT_H
#define VARIANT_H

#include "llvm/DerivedTypes.h"

struct variant {
	enum { real, string } type;
	union {
		double real_val;
		struct { int length; const char *data; } string_val;
	};
};

extern "C" {

variant create_real(double val) {
	variant var;
	var.type = variant::real;
	var.real_val = val;
	return var;
}

variant create_string(char *data, int length) {
	variant var;
	var.type = variant::string;
	var.string_val.length = length;
	var.string_val.data = data;
	return var;
}

// ! not

// ~ inv

// - neg

// + pos

}

#endif
