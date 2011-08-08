#ifndef PRINTER_H
#define PRINTER_H

#include "lexer.h"
#include "node.h"

void print_token(const token& t);

class node_printer : public node::visitor {
public:
	node_printer() : scope(0), precedence(0) {}
	
	void visit(value *v);
	void visit(unary *u);
	void visit(binary *b);
	void visit(subscript *s);
	void visit(call *c);
	
	void visit(assignment *a);
	void visit(invocation* i);
	void visit(declaration *d);
	void visit(block *b);
	
	void visit(ifstatement *i);
	void visit(whilestatement *w);
	void visit(dostatement *d);
	void visit(repeatstatement *r);
	void visit(forstatement *f);
	void visit(switchstatement *s);
	void visit(withstatement *w);
	
	void visit(jump *j);
	void visit(returnstatement *r);
	void visit(casestatement *c);
	
private:
	void indent();
	void print_branch(statement *s);
	
	size_t scope;
	int precedence;
};

#endif
