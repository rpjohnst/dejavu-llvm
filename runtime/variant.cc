#include "dejavu/runtime/variant.h"
#include "dejavu/runtime/error.h"
#include <cmath>

extern "C" double to_real(const variant &a) {
	switch (a.type) {
	case 0: return a.real;
	default: show_error(0, 0, "expected a real", true); return 0;
	}
}

extern "C" string *to_string(const variant &a) {
	switch (a.type) {
	case 1: return a.string;
	default: show_error(0, 0, "expected a string", true); return nullptr;
	}
}

extern "C" __attribute__((pure)) string *intern(string *s) {
	return strings.intern(s);
}

// todo: resizing
extern "C" variant *access(var *a, unsigned short x, unsigned short y) {
	if (x >= a->x) {
		show_error(0, 0, "index out of bounds", true);
		return 0;
	}

	if (y >= a->y) {
		show_error(0, 0, "index out of bounds", true);
		return 0;
	}

	return &a->contents[x + y * a->x];
}

extern "C" void retain(variant *a) {
	switch (a->type) {
	case 1: a->string->retain(); break;
	}
}

extern "C" void release(variant *a) {
	switch (a->type) {
	case 1: a->string->release(); break;
	}
}

extern "C" void retain_var(var *a) {
	for (size_t y = 0; y < a->y; y++) {
		for (size_t x = 0; x < a->x; x++) {
			variant *v = access(a, x, y);
			retain(v);
		}
	}
}

extern "C" void release_var(var *a) {
	for (size_t y = 0; y < a->y; y++) {
		for (size_t x = 0; x < a->x; x++) {
			variant *v = access(a, x, y);
			release(v);
		}
	}
}

// unary operators

typedef variant unary(variant);

#define UNARY_OP(name) static variant name(variant a)
#define UNARY_ERROR(op, s) static variant op ## _error(variant) { \
	return show_error(0, 0, "wrong type to " #s, true), 0; \
}
#define UNARY_TABLE(op) static unary *const op ## _table[]
#define UNARY_DISPATCH(op) extern "C" variant op(variant *a) { \
	return op ## _table[a->type](*a); \
}

#define UNARY_OP_REAL(name, op) \
UNARY_OP(name ## _real); \
UNARY_ERROR(name, op) \
UNARY_TABLE(name) = { name ## _real, name ## _error }; \
UNARY_DISPATCH(name) \
UNARY_OP(name ## _real)

#define UNARY_OP_DEFAULT(name, op) UNARY_OP_REAL(name, op) { return op a.real; } 

UNARY_OP_DEFAULT(not_, !) // get around c
UNARY_OP_REAL(inv, ~) { return ~(int)a.real; }
UNARY_OP_DEFAULT(neg, -)
UNARY_OP_DEFAULT(pos, +)

// binary operators

typedef variant binary(variant, variant);

#define BINARY_OP(name) static variant name( \
	__attribute__((unused)) variant a, __attribute__((unused)) variant b \
)
#define BINARY_ERROR(op, s) static variant op ## _error(variant, variant) { \
	return show_error(0, 0, "wrong types to " #s, true), 0; \
}
#define BINARY_TABLE(op) static binary *const op ## _table[][2]
#define BINARY_DISPATCH(op) extern "C" variant op(variant *a, variant *b) { \
	return op ## _table[a->type][b->type](*a, *b); \
}

#define BINARY_OP_REAL(name, op) \
BINARY_OP(name ## _real_real); \
BINARY_ERROR(name, op) \
BINARY_TABLE(name) = { \
	{ name ## _real_real, name ## _error }, { name ## _error, name ## _error } \
}; \
BINARY_DISPATCH(name) \
BINARY_OP(name ## _real_real)

#define BINARY_OP_DEFAULT(name, op) BINARY_OP_REAL(name, op) { return a.real op b.real; }

BINARY_OP_DEFAULT(less, <)
BINARY_OP_DEFAULT(less_equals, <=)

BINARY_OP(is_equals_real_real) { return a.real == b.real; }
BINARY_OP(is_equals_string_string) { return a.string == b.string; }
BINARY_OP(is_equals_default) { return false; }
BINARY_TABLE(is_equals) = {
	{ is_equals_real_real, is_equals_default }, { is_equals_default, is_equals_string_string }
};
BINARY_DISPATCH(is_equals)

extern "C" variant not_equals(variant *a, variant *b) {
	return !is_equals(a, b).real;
}

BINARY_OP_DEFAULT(greater_equals, >=)
BINARY_OP_DEFAULT(greater, >)

BINARY_OP(plus_real_real) { return a.real + b.real; }
BINARY_OP(plus_string_string) {
	size_t length = a.string->length + b.string->length;
	string *str = new (length) string(length);
	str->retain();

	memcpy((void*)str->data, (void*)a.string->data, a.string->length);
	memcpy((void*)(str->data + a.string->length), (void*)b.string->data, b.string->length);
	str->hash = string::compute_hash(str->length, str->data);

	string *ret = strings.intern(str);
	ret->retain();
	str->release();
	return ret;
}
BINARY_ERROR(plus, +)
BINARY_TABLE(plus) = { { plus_real_real, plus_error }, { plus_error, plus_string_string } };
BINARY_DISPATCH(plus)

BINARY_OP_DEFAULT(minus, -)
BINARY_OP_DEFAULT(times, *)
BINARY_OP_DEFAULT(divide, /)

BINARY_OP_DEFAULT(log_and, &&)
BINARY_OP_DEFAULT(log_or, ||)
BINARY_OP_REAL(log_xor, ^^) { return (bool)a.real != (bool)b.real; }

BINARY_OP_REAL(bit_and, &) { return (int)a.real & (int)b.real; }
BINARY_OP_REAL(bit_or, |) { return (int)a.real | (int)b.real; }
BINARY_OP_REAL(bit_xor, ^) { return (int)a.real ^ (int)b.real; }
BINARY_OP_REAL(shift_left, <<) { return (int)a.real << (int)b.real; }
BINARY_OP_REAL(shift_right, >>) { return (int)a.real >> (int)b.real; }

BINARY_OP_REAL(div, div) { return (int)(a.real / b.real); }
BINARY_OP_REAL(mod, mod) { return fmod(a.real, b.real); }
