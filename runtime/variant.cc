#include "dejavu/runtime/variant.h"

// unary operators

typedef variant unary(variant);
static variant unary_error(variant) { }

static variant pos_real(variant a) {
	return a;
}
static unary *pos_table[] = { pos_real, unary_error };
extern "C" variant pos(variant a) { return pos_table[a.type](a); }

static variant neg_real(variant a) {
	variant ret = { .type = 0, .real = -a.real };
	return ret;
}
static unary *neg_table[] = { neg_real, unary_error };
extern "C" variant neg(variant a) { return neg_table[a.type](a); }

// binary operators

typedef variant binary(variant, variant);
static variant binary_error(variant, variant) { }

static variant plus_real_real(variant a, variant b) {
	variant ret = { .type = 0, .real = a.real + b.real };
	return ret;
}
static variant plus_string_string(variant a, variant b) {
	// todo: concatenation, garbage collection, death
	variant ret = { .type = 1, .string = a.string };
	return ret;
}
static binary *plus_table[][2] = {
	{ plus_real_real, binary_error }, { binary_error, plus_string_string }
};
extern "C" variant plus(variant a, variant b) { return plus_table[a.type][b.type](a, b); }
