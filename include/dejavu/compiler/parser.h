#ifndef PARSER_H
#define PARSER_H

#include <dejavu/compiler/lexer.h>
#include <dejavu/compiler/node.h>
#include <dejavu/system/arena.h>
#include <dejavu/compiler/error_stream.h>
#include <exception>
#include <vector>
#include <map>
#include <algorithm>

class parser {
	friend class symbol_table;

public:
	parser(token_stream& l, arena &allocator, error_stream& e);
	node *getprogram();

private:
	// expressions
	expression *getexpression(int prec = 0);

	expression *id_nud(token t);
	expression *name_nud(token t);
	expression *prefix_nud(token t);
	expression *paren_nud(token t);

	expression *null_nud(token t) { return error_expr(unexpected_token_error(t, "expression")); }

	expression *infix_led(token t, expression *left);
	expression *dot_led(token t, expression *left);
	expression *square_led(token t, expression *left);
	expression *paren_led(token t, expression *left);

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

	statement *null_std() { return error_stmt(unexpected_token_error(current, "statement")); }

	// utilities
	token advance();
	token advance(token_type t);

	statement_error *error_stmt(const unexpected_token_error&);
	expression_error *error_expr(const unexpected_token_error&);

	token_stream& lexer;
	token current;

	arena &allocator;
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
