#ifndef PARSER_H
#define PARSER_H

#include <exception>
#include <vector>
#include <map>
#include <algorithm>
#include "disallow_copy.h"
#include "lexer.h"
#include "node.h"

struct unexpected_token_error : public std::exception {
	template<typename... Args>
	unexpected_token_error(token unexpected, Args... exp) :
		std::exception(), unexpected(unexpected), expected { exp... } {}
	
	~unexpected_token_error() throw () {} // wtf
	
	token unexpected;
	std::vector<token_type> expected;
};

class parser {
	friend class symbol_table;
	
public:
	parser(token_stream& l);
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
	
	// utilities
	token advance();
	token advance(token_type t);
	
	token_stream& lexer;
	token current;
	
	DISALLOW_COPY(parser);
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
