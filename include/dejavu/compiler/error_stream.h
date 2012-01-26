#ifndef ERROR_STREAM_H
#define ERROR_STREAM_H

#include "dejavu/compiler/lexer.h"

struct unexpected_token_error {
	unexpected_token_error(token unexpected, const char *expected) :
		unexpected(unexpected), expected(expected) {}

	unexpected_token_error(token unexpected, token_type exp) :
		unexpected(unexpected), expected_token(exp) {}

	token unexpected;
	const char *expected;
	token_type expected_token;
};

// todo: codegen errors
// todo: linker errors

struct error_stream {
	virtual void push_back(const unexpected_token_error&) = 0;
};

#endif
