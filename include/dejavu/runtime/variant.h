#ifndef RUNTIME_VARIANT_H
#define RUNTIME_VARIANT_H

struct variant {
	char type;
	union {
		double real;
		struct { int length; const char *data; } string;
	};
};

extern "C" {
	variant pos(variant);
	variant neg(variant);

	variant plus(variant, variant);
}

#endif
