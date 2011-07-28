#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <exception>
#include <vector>
#include "disallow_copy.h"

class expression {
public:
	expression() {} // wtf
	
	struct visitor {
		virtual void visit(class value*) = 0;
		virtual void visit(class unary*) = 0;
		virtual void visit(class binary*) = 0;
		virtual void visit(class subscript*) = 0;
		virtual void visit(class call*) = 0;
	};
	virtual void accept(visitor*) = 0;
	
private:
	DISALLOW_COPY(expression);
};

#define IMPLEMENT_ACCEPT() void accept(visitor *v) { \
	v->visit(this); \
}

struct value : public expression {
	value(token t) : t(t) {}
	token t;
	
	IMPLEMENT_ACCEPT()
};

struct unary : public expression {
	unary(token_type op, expression *right) : op(op), right(right) {}
	
	token_type op;
	expression *left, *right;
	
	IMPLEMENT_ACCEPT()
};

struct binary : public expression {
	binary(token_type op, expression *left, expression *right) :
		op(op), left(left), right(right) {}
	
	token_type op;
	expression *left, *right;
	
	IMPLEMENT_ACCEPT()
};

struct subscript : public expression {
	subscript(expression *array, std::vector<expression*>& indices) :
		array(array), indices(indices) {}
	
	expression *array;
	std::vector<expression*> indices;
	
	IMPLEMENT_ACCEPT()
};

struct call : public expression {
	call(expression *function, std::vector<expression*>& args) :
		function(function), args(args) {}
	
	expression *function;
	std::vector<expression*> args;
	
	IMPLEMENT_ACCEPT()
};

#undef IMPLEMENT_ACCEPT

struct unexpected_token_error : public std::exception {
	explicit unexpected_token_error(token t) : std::exception(), tok(t) {}
	token tok;
};

class parser {
	friend class symbol_table;
	
public:
	parser(token_stream& l);
	expression *getexpression(int prec = 0);

private:
	expression *id_nud(token t);
	expression *name_nud(token t);
	expression *prefix_nud(token t);
	expression *paren_nud(token t);
	
	expression *infix_led(token t, expression *left);
	expression *dot_led(token t, expression *left);
	expression *square_led(token t, expression *left);
	expression *paren_led(token t, expression *left);
	
	token_stream& lexer;
	token current;
	
	DISALLOW_COPY(parser);
};

#endif
