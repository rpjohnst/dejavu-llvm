#include "dejavu/compiler/lexer.h"
#include "dejavu/system/buffer.h"
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>

namespace {

bool isspace(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
bool isnewline(char c) {
	return c == '\n' || c == '\r';
}

bool isnamestart(char c) {
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}
bool isname(char c) { 
	return c == '_' || isnamestart(c) || isdigit(c);
}

bool isoperator(char c) {
	switch (c) {
	case '{': case '}': case '(': case ')': case '[': case ']':
	case '.': case ',': case ':': case ';':
	case '+': case '-': case '*': case '/':
	case '|': case '&': case '^': case '~':
	case '=': case '<': case '>':
	case '!': return true;

	default: return false;
	}
}

bool isdigit(char c) {
	return '0' <= c && c <= '9';
}

struct keyword_table : public std::map<std::string, token_type> {
	keyword_table();
} keywords;

keyword_table::keyword_table() {
	keywords["begin"] = l_brace;
	keywords["end"] = r_brace;
	keywords["not"] = exclaim;
	keywords["and"] = ampamp;
	keywords["or"] = pipepipe;
	keywords["xor"] = caretcaret;
#	define KEYWORD(X) keywords[#X] = kw_ ## X;
#	include "dejavu/compiler/tokens.tbl"
}

}

token_stream::token_stream(buffer &b) :
	row(1), col(1), source(b), current(b.begin()), buffer_end(b.end()) {
}

// todo: potential cleanup/optimization with a switch statement
token token_stream::gettoken() {
	skipwhitespace();

	if (isnamestart(*current))
		return getname();

	if (
		isoperator(*current) &&

		// we need a special case for stupid numbers like .25
		(*current != '.' || !isdigit(*(current + 1)))
	)
		return getoperator();

	if (isdigit(*current) || *current == '$' || *current == '.')
		return getnumber();

	if (*current == '"' || *current == '\'')
		return getstring();

	// eof
	if (current == buffer_end) {
		return token(eof, row, col);
	}

	// error
	token u = token(unexpected, row, ++col);
	u.string.data = current++; u.string.length = 1;
	return u;
}

// skips any whitespace or comments at the current position
void token_stream::skipwhitespace() {
	for (; current != buffer_end; current++) {
		if (*current == '\t') {
			col += 4;
		}
		else if (isnewline(*current)) {
			skipnewline();
		}
		else if (isspace(*current)) {
			col += 1;
		}
		else if (*current == '/') {
			if (skipcomment()) {
				if (current == buffer_end) {
					break;
				}
			}
			else {
				break;
			}
		}
		else {
			break;
		}
	}
}

bool token_stream::skipcomment() {
	// single-line comment
	if (*(current + 1) == '/') {
		current += 2;

		for (; current != buffer_end; current++) {
			if (isnewline(*current)) {
				break;
			}
		}

		return true;
	}
	// multi-line comment
	else if (*(current + 1) == '*') {
		current += 2;
		col += 2;

		for (; current != buffer_end; current++) {
			if (*current == '*' && *(current + 1) == '/') {
				current++;
				col += 2;
				break;
			}
			else if (isnewline(*current)) {
				skipnewline();
			}
			else {
				col += 1;
			}
		}

		return true;
	}

	return false;
}

// skips a newline at the current position
// does NOT update current for single-char newlines
// if there's no newline, behavior is undefined
void token_stream::skipnewline() {
	row += 1;
	col = 1;

	if (*current == '\r' && *(current + 1) == '\n') {
		current++;
	}
}

// returns the name or keyword at the current position
// if there is none, behavior is undefined
token token_stream::getname() {
	token t(name, row, col);
	t.string.data = current;

	do {
		current++;
	} while (isname(*current));
	t.string.length = current - t.string.data;

	col += t.string.length;

	// todo: figure out a better way than constructing an std::string?
	std::string key = std::string(t.string.data, t.string.length);
	if (keywords.count(key))
		t.type = keywords[key];

	return t;
}

// returns the operator token at the current position
// if there is none, behavior is undefined
token token_stream::getoperator() {
	token t(unexpected, row, col);

	// todo: can we use tokens.tbl here for maintainability? it just
	// needs to be put in the right order for e.g. a regex parser
	col++;
	char c = *current++;
	switch (c) {
	case '{': t.type = l_brace; return t;
	case '}': t.type = r_brace; return t;
	case '(': t.type = l_paren; return t;
	case ')': t.type = r_paren; return t;
	case '[': t.type = l_square; return t;
	case ']': t.type = r_square; return t;

	case ',': t.type = comma; return t;
	case '.': t.type = dot; return t;
	case ';': t.type = semicolon; return t;
	case ':':
		switch (*current) {
		case '=': col++; current++; t.type = equals; return t;
		default: t.type = colon; return t;
		}

	case '~': t.type = tilde; return t;

	case '=':
		switch (*current) {
		case '=': col++; current++; t.type = is_equals; return t;
		default: t.type = equals; return t;
		}
	case '!':
		switch (*current) {
		case '=': col++; current++; t.type = not_equals; return t;
		default: t.type = exclaim; return t;
		}
	case '<':
		switch (*current) {
		case '<': col++; current++; t.type = shift_left; return t;
		case '=': col++; current++; t.type = less_equals; return t;
		default: t.type = less; return t;
		}
	case '>':
		switch (*current) {
		case '>': col++; current++; t.type = shift_right; return t;
		case '=': col++; current++; t.type = greater_equals; return t;
		default: t.type = greater; return t;
		}

	case '+':
		switch (*current) {
		case '=': col++; current++; t.type = plus_equals; return t;
		default: t.type = plus; return t;
		}
	case '-':
		switch (*current) {
		case '=': col++; current++; t.type = minus_equals; return t;
		default: t.type = minus; return t;
		}
	case '*':
		switch (*current) {
		case '=': col++; current++; t.type = times_equals; return t;
		default: t.type = times; return t;
		}
	case '/':
		switch (*current) {
		case '=': col++; current++; t.type = div_equals; return t;
		default: t.type = divide; return t;
		}

	case '&':
		switch (*current) {
		case '=': col++; current++; t.type = and_equals; return t;
		case '&': col++; current++; t.type = ampamp; return t;
		default: t.type = bit_and; return t;
		}
	case '|':
		switch (*current) {
		case '=': col++; current++; t.type = or_equals; return t;
		case '|': col++; current++; t.type = pipepipe; return t;
		default: t.type = bit_or; return t;
		}
	case '^':
		switch (*current) {
		case '=': col++; current++; t.type = xor_equals; return t;
		case '^': col++; current++; t.type = caretcaret; return t;
		default: t.type = bit_xor; return t;
		}

	default: return t;
	}
}

// returns the number at the current position
// if there is none, behavior is undefined
token token_stream::getnumber() {
	token t(real, row, col);

	char *end;
	if (*current == '$')
		t.real = strtoul(current + 1, &end, 16);
	else
		t.real = strtod(current, &end);

	col += end - current;
	current = end;

	return t;
}

// returns the string literal at the current position
// GML makes this easy without escape sequences but we'll want them later
// todo: error on unterminated strings
token token_stream::getstring() {
	token t(string, row, col);

	char delim = *current++;
	col += 1;

	t.string.data = current;
	while (current != buffer_end && *current != delim) {
		if (isnewline(*current))
			skipnewline();

		current++;
		col += 1;
	}
	t.string.length = current - t.string.data;

	current++;
	col += 1;

	return t;
}

std::ostream &operator <<(std::ostream &o, token_type t) {
	switch (t) {
#	define KEYWORD(X) case kw_ ## X: return o << #X;
#	define OPERATOR(X, Y) case X: return o << Y;
#	include "dejavu/compiler/tokens.tbl"
	default: return o;
	}
}

std::ostream &operator <<(std::ostream &o, const token& t) {
	switch (t.type) {
	case real: return o << t.real;
	case unexpected: case name: return o.write(t.string.data, t.string.length);
	case string: o << '"'; o.write(t.string.data, t.string.length); return o << '"';

#	define KEYWORD(X) case kw_ ## X: return o << #X;
#	define OPERATOR(X, Y) case X: return o << Y;
#	include "dejavu/compiler/tokens.tbl"

	default: return o;
	}
}
