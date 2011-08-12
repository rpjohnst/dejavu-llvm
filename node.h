#ifndef NODE_H
#define NODE_H

#include <vector>
#include <algorithm>
#include "lexer.h"

class node {
public:
	node() {} // wtf
	// todo: can we use a custom allocator to free all nodes at once?
	// maybe bump pointer in a large buffer?
	
	struct visitor {
		virtual void visit(class expression_error*) = 0;
		
		virtual void visit(class value*) = 0;
		virtual void visit(class unary*) = 0;
		virtual void visit(class binary*) = 0;
		virtual void visit(class subscript*) = 0;
		virtual void visit(class call*) = 0;
		
		virtual void visit(class statement_error*) = 0;
		
		virtual void visit(class assignment*) = 0;
		virtual void visit(class invocation*) = 0;
		virtual void visit(class declaration*) = 0;
		virtual void visit(class block*) = 0;
		
		virtual void visit(class ifstatement*) = 0;
		virtual void visit(class whilestatement*) = 0;
		virtual void visit(class dostatement*) = 0;
		virtual void visit(class repeatstatement*) = 0;
		virtual void visit(class forstatement*) = 0;
		virtual void visit(class switchstatement*) = 0;
		virtual void visit(class withstatement*) = 0;
		
		virtual void visit(class jump*) = 0;
		virtual void visit(class returnstatement*) = 0;
		virtual void visit(class casestatement*) = 0;
	};
	virtual void accept(visitor*) = 0;

private:
	DISALLOW_COPY(node);
};

#define IMPLEMENT_ACCEPT() void accept(visitor *v) { \
	v->visit(this); \
}

class expression : public node {};

struct expression_error : public expression {
	IMPLEMENT_ACCEPT();
};

struct value : public expression {
	value(token t) : t(t) {}
	token t;
	
	IMPLEMENT_ACCEPT()
};

struct unary : public expression {
	unary(token_type op, expression *right) : op(op), right(right) {}
	~unary() { delete right; }
	
	token_type op;
	expression *right;
	
	IMPLEMENT_ACCEPT()
};

struct binary : public expression {
	binary(token_type op, expression *left, expression *right) :
		op(op), left(left), right(right) {}
	~binary() { delete left; delete right; }
	
	token_type op;
	expression *left, *right;
	
	IMPLEMENT_ACCEPT()
};

struct subscript : public expression {
	subscript(expression *array, std::vector<expression*>& indices) :
		array(array), indices(indices) {}
	~subscript() {
		delete array;
		std::for_each(indices.begin(), indices.end(), [](expression *e) {
			delete e;
		});
	}
	
	expression *array;
	std::vector<expression*> indices;
	
	IMPLEMENT_ACCEPT()
};

struct call : public expression {
	call(expression *function, std::vector<expression*>& args) :
		function(function), args(args) {}
	~call() {
		delete function;
		std::for_each(args.begin(), args.end(), [](expression *e) {
			delete e;
		});
	}
	
	expression *function;
	std::vector<expression*> args;
	
	IMPLEMENT_ACCEPT()
};

class statement : public node {};

struct statement_error : public statement {
	IMPLEMENT_ACCEPT();
};

struct assignment : public statement {
	assignment(token_type op, expression *lvalue, expression *rvalue) :
		op(op), lvalue(lvalue), rvalue(rvalue) {}
	~assignment() { delete lvalue; delete rvalue; }
	
	token_type op;
	expression *lvalue, *rvalue;
	
	IMPLEMENT_ACCEPT()
};

struct invocation : public statement {
	invocation(call *c) : c(c) {}
	~invocation() { delete c; }
	
	call *c;
	
	IMPLEMENT_ACCEPT()
};

struct declaration : public statement {
	declaration(token type, std::vector<value*>& names) :
		type(type), names(names) {}
	~declaration() {
		std::for_each(names.begin(), names.end(), [](value *n) {
			delete n;
		});
	}
	
	token type;
	std::vector<value*> names;
	
	IMPLEMENT_ACCEPT()
};

struct block : public statement {
	block(std::vector<statement*>& stmts) : stmts(stmts) {}
	~block() {
		std::for_each(stmts.begin(), stmts.end(), [](statement *s) {
			delete s;
		});
	}
	
	std::vector<statement*> stmts;
	
	IMPLEMENT_ACCEPT()
};

struct ifstatement : public statement {
	ifstatement(
		expression *cond,
		statement *branch_true,
		statement *branch_false
	) :
		cond(cond),
		branch_true(branch_true),
		branch_false(branch_false) {}
	~ifstatement() {
		delete cond; delete branch_true; delete branch_false;
	}
	
	expression *cond;
	statement *branch_true, *branch_false;
	
	IMPLEMENT_ACCEPT()
};

struct whilestatement : public statement {
	whilestatement(expression *cond, statement *stmt) :
		cond(cond), stmt(stmt) {}
	~whilestatement() { delete cond; delete stmt; }
	
	expression *cond;
	statement *stmt;
	
	IMPLEMENT_ACCEPT()
};

struct dostatement : public statement {
	dostatement(expression *cond, statement *stmt) :
		cond(cond), stmt(stmt) {}
	~dostatement() { delete cond; delete stmt; }
	
	expression *cond;
	statement *stmt;
	
	IMPLEMENT_ACCEPT()
};

struct repeatstatement : public statement {
	repeatstatement(expression *expr, statement *stmt) :
		expr(expr), stmt(stmt) {}
	~repeatstatement() { delete expr; delete stmt; }
	
	expression *expr;
	statement *stmt;
	
	IMPLEMENT_ACCEPT()
};

struct forstatement : public statement {
	forstatement(
		statement *init,
		expression *cond,
		statement *inc,
		statement *stmt
	) : init(init), cond(cond), inc(inc), stmt(stmt) {}
	~forstatement() {
		delete init; delete cond; delete inc; delete stmt;
	}
	
	statement *init;
	expression *cond;
	statement *inc;
	statement *stmt;
	
	IMPLEMENT_ACCEPT()
};

struct switchstatement : public statement {
	switchstatement(expression *expr, block *stmts) :
		expr(expr), stmts(stmts) {}
	~switchstatement() { delete expr; delete stmts; }
	
	expression *expr;
	block *stmts;
	
	IMPLEMENT_ACCEPT()
};

struct withstatement : public statement {
	withstatement(expression *expr, statement *stmt) :
		expr(expr), stmt(stmt) {}
	~withstatement() { delete expr; delete stmt; }
	
	expression *expr;
	statement *stmt;
	
	IMPLEMENT_ACCEPT()
};

struct jump : public statement {
	jump(token_type type) : type(type) {}
	
	token_type type;
	
	IMPLEMENT_ACCEPT()
};

struct returnstatement : public statement {
	returnstatement(expression *expr) : expr(expr) {}
	~returnstatement() { delete expr; }
	
	expression *expr;
	
	IMPLEMENT_ACCEPT()
};

struct casestatement : public statement {
	casestatement(expression *expr) : expr(expr) {}
	~casestatement() { delete expr; }
	
	expression *expr;
	
	IMPLEMENT_ACCEPT()
};

#undef IMPLEMENT_ACCEPT

#endif
