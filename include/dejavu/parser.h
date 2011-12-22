#ifndef PARSER_H
#define PARSER_H

#include "dejavu/lexer.h"
#include "dejavu/node.h"
#include <exception>
#include <vector>
#include <map>
#include <algorithm>

struct unexpected_token_error {
	unexpected_token_error(token unexpected, const char *expected) :
		unexpected(unexpected), expected(expected) {}

	unexpected_token_error(token unexpected, token_type exp) :
		unexpected(unexpected), expected_token(exp) {}

	token unexpected;
	const char *expected;
	token_type expected_token;
};

struct error_stream {
	virtual void push_back(const unexpected_token_error&) = 0;
};

class parser {
	friend class symbol_table;

public:
	parser(token_stream& l, error_stream& e);
	node *getprogram();

private:
	// expressions
	expression *getexpression(int prec = 0);

	expression *id_nud(token t);
	expression *name_nud(token t);
	expression *prefix_nud(token t);
	expression *paren_nud(token t);

	expression *infix_led(token t, expression *left);
	expression *dot_led(token t, expression *left);
	expression *square_led(token t, token left);
	expression *paren_led(token t, token left);

	// statements
	statement *getstatement();

	statement *expr_std();
	statement *var_std();
	statement *brace_std();

	statement *if_std();
	statement *while_std();
	statement *do_std();
	statement *repeat_std();
	statement *for_std();
	statement *switch_std();
	statement *with_std();

	statement *jump_std();
	statement *return_std();
	statement *case_std();

	// utilities
	token advance();
	token advance(token_type t);

	statement_error *error_stmt(const unexpected_token_error&);
	expression_error *error_expr(const unexpected_token_error&);

	token_stream& lexer;
	token current;

	error_stream& errors;
};

typedef statement *(parser::*std_parser)();
typedef expression *(parser::*nud_parser)(token);
typedef expression *(parser::*led_parser)(token, expression*);

struct symbol {
	int precedence;
	std_parser std;
	nud_parser nud;
	led_parser led;
};

class symbol_table : public std::map<token_type, symbol> {
public:
	symbol_table();

private:
	void prefix(token_type t, nud_parser nud = &parser::prefix_nud);
	void infix(token_type t, int prec, led_parser led = &parser::infix_led);
};

extern symbol_table symbols;

#endif
