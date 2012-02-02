#ifndef ERROR_STREAM_H
#define ERROR_STREAM_H

#include "dejavu/compiler/lexer.h"

struct unexpected_token_error {
	unexpected_token_error(token unexpected, const char *expected) :
		unexpected(unexpected), expected(expected) {}

	unexpected_token_error(token unexpected, token_type exp) :
		unexpected(unexpected), expected_token(exp) {}

	token unexpected;
	const char *expected = 0;
	token_type expected_token = ::unexpected;
};

// todo: lexer errors
// todo: codegen errors
// todo: linker errors

struct error_stream {
	virtual void set_context(const std::string &) = 0;
	virtual int count() = 0;

	virtual void error(const unexpected_token_error&) = 0;
	virtual void error(const std::string &) = 0;

	virtual void progress(int i, const std::string &) = 0;
};

#endif
