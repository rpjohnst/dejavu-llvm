#include "printer.h"
#include <cstdio>
#include "parser.h"

void print_token(const token& t) {
	switch (t.type) {
	case unexpected:
		printf("%.*s", (int)t.string.length, t.string.data);
		break;
	
	case name:
		printf("%.*s", (int)t.string.length, t.string.data);
		break;
	
	case real:
		printf("%g", t.real);
		break;
	
	case string:
		printf("\"%.*s\"", (int)t.string.length, t.string.data);
		break;
	
#	define KEYWORD(X) case kw_ ## X: printf(#X); break;
#	include "tokens.tbl"
	
#	define OPERATOR(X, Y) case X: printf(Y); break;
#	include "tokens.tbl"
	
	default: ; // do nothing
	}
}

void node_printer::visit_expression_error(expression_error*) {
	printf("<expression error>");
}

void node_printer::visit_value(value *v) {
	if (precedence >= 80 && v->t.type == real) printf("(");
	print_token(v->t);
	if (precedence >= 80 && v->t.type == real) printf(")");
}

void node_printer::visit_unary(unary *u) {
	print_token(token(u->op, 0, 0));
	
	int p = precedence;
	precedence = 70;
	
	visit(u->right);
	
	precedence = p;
}

void node_printer::visit_binary(binary *b) {
	if (symbols[b->op].precedence < precedence) {
		printf("(");
	}
	int p = precedence;
	precedence = symbols[b->op].precedence;
	
	visit(b->left);
	if (b->op != dot) printf(" ");
	print_token(token(b->op, 0, 0));
	if (b->op != dot) printf(" ");
	visit(b->right);
	
	precedence = p;
	if (symbols[b->op].precedence < precedence) {
		printf(")");
	}
}

void node_printer::visit_subscript(subscript *s) {
	visit(s->array);
	printf("[");
	
	int p = precedence;
	precedence = 0;
	
	print_list(s->indices.begin(), s->indices.end());
	
	precedence = p;
	
	printf("]");
}

void node_printer::visit_call(call *c) {
	if (precedence >= 80) printf("(");
	
	visit(c->function);
	printf("(");
	print_list(c->args.begin(), c->args.end());
	printf(")");
	
	if (precedence >= 80) printf(")");
}

void node_printer::visit_statement_error(statement_error*) {
	printf("<statement error>");
}

void node_printer::visit_assignment(assignment *a) {
	visit(a->lvalue); printf(" ");
	print_token(token(a->op, 0, 0)); printf(" ");
	visit(a->rvalue); printf(";");
}

void node_printer::visit_invocation(invocation* i) {
	visit(i->c); printf(";");
}

void node_printer::visit_declaration(declaration *d) {
	printf("var ");
	print_list(d->names.begin(), d->names.end());
	printf(";");
}

void node_printer::visit_block(block *b) {
	printf("{\n"); scope++;
	for (
		std::vector<statement*>::iterator it = b->stmts.begin();
		it != b->stmts.end(); ++it
	) {
		if ((*it)->type == casestatement_node) scope--;
		indent(); visit(*it); printf("\n");
		if ((*it)->type == casestatement_node) scope++;
	};
	scope--; indent(); printf("}");
}

void node_printer::visit_ifstatement(ifstatement *i) {
	printf("if ("); visit(i->cond); printf(")");
	print_branch(i->branch_true);
	if (i->branch_false) {
		printf("\n");
		indent(); printf("else");
		
		if (i->branch_false->type != ifstatement_node) {
			printf("\n"); print_branch(i->branch_false);
		}
		else {
			printf(" "); visit(i->branch_false);
		}
	}
}

void node_printer::visit_whilestatement(whilestatement *w) {
	printf("while ("); visit(w->cond); printf(")");
	print_branch(w->stmt);
}

void node_printer::visit_dostatement(dostatement *d) {
	printf("do");
	print_branch(d->stmt);
	printf(" until ("); visit(d->cond); printf(");");
}

void node_printer::visit_repeatstatement(repeatstatement *r) {
	printf("repeat ("); visit(r->expr); printf(")");
	print_branch(r->stmt);
}

void node_printer::visit_forstatement(forstatement *f) {
	printf("for (");
		visit(f->init); printf(" ");
		visit(f->cond); printf("; ");
		visit(f->inc); printf("\b");
	printf(")");
	print_branch(f->stmt);
}

void node_printer::visit_switchstatement(switchstatement *s) {
	printf("switch ("); visit(s->expr); printf(")");
	print_branch(s->stmts);
}

void node_printer::visit_withstatement(withstatement *w) {
	printf("with ("); visit(w->expr); printf(")");
	print_branch(w->stmt);
}

void node_printer::visit_jump(jump *j) {
	print_token(token(j->type, 0, 0)); printf(";");
}

void node_printer::visit_returnstatement(returnstatement *r) {
	printf("return "); visit(r->expr); printf(";");
}

void node_printer::visit_casestatement(casestatement *c) {
	if (c->expr) {
		printf("case "); visit(c->expr);
	}
	else {
		printf("default");
	}
	
	printf(":");
}

void node_printer::indent() {
	for (size_t i = 0; i < scope; i++)
		printf("    ");
}

void node_printer::print_branch(statement *s) {
	if (s->type == block_node) {
		printf(" "); visit(static_cast<block*>(s));
	}
	else {
		printf("\n");
		scope++;
		indent(); visit(s);
		scope--;
	}
}

template <typename I>
void node_printer::print_list(I begin, I end) {
	for (I it = begin; it != end; ++it) {
		if (it != begin) printf(", ");
		visit(*it);
	}
}
