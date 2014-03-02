#ifndef TOKEN_H
#define TOKEN_H

#include <cstddef>
#include <stdexcept>

class buffer;

enum token_type {
#define TOK(X) X,
#include "dejavu/compiler/tokens.tbl"
};

std::ostream &operator <<(std::ostream &o, token_type t);

struct token {
	token() {}
	token(token_type t, size_t r, size_t c) : type(t), row(r), col(c) {}

	token_type type;
	size_t row, col;

	union {
		double real;
		struct {
			size_t length;
			const char *data;
		} string;
	};
};

std::ostream &operator <<(std::ostream &o, const token &t);

class token_stream {
public:
	token_stream(buffer &b);
	token gettoken();

private:
	// helper functions
	void skipwhitespace();
	bool skipcomment();
	void skipnewline();
	token getname();
	token getoperator();
	token getnumber();
	token getstring();

	size_t row, col;

	buffer &source;
	const char *current, *buffer_end;
};

#endif
