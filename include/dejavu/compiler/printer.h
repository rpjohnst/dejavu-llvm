#ifndef PRINTER_H
#define PRINTER_H

#include "dejavu/compiler/lexer.h"
#include "dejavu/compiler/node_visitor.h"

void print_token(const token& t);

class node_printer : public node_visitor<node_printer> {
public:
	node_printer() : scope(0), precedence(0) {}

	void visit_expression_error(expression_error *e);

	void visit_value(value *v);
	void visit_unary(unary *u);
	void visit_binary(binary *b);
	void visit_subscript(subscript *s);
	void visit_call(call *c);

	void visit_assignment(assignment *a);
	void visit_invocation(invocation* i);
	void visit_declaration(declaration *d);
	void visit_block(block *b);

	void visit_ifstatement(ifstatement *i);
	void visit_whilestatement(whilestatement *w);
	void visit_dostatement(dostatement *d);
	void visit_repeatstatement(repeatstatement *r);
	void visit_forstatement(forstatement *f);
	void visit_switchstatement(switchstatement *s);
	void visit_withstatement(withstatement *w);

	void visit_jump(jump *j);
	void visit_returnstatement(returnstatement *r);
	void visit_casestatement(casestatement *c);

	void visit_statement_error(statement_error *e);

private:
	void indent();
	void print_branch(statement *s);

	template <typename I>
	void print_list(I begin, I end);

	size_t scope;
	int precedence;
};

#endif
