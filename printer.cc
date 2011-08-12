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

void node_printer::visit(expression_error*) {
	printf("<expression error>");
}

void node_printer::visit(value *v) {
	if (precedence >= 80 && v->t.type == real) printf("(");
	print_token(v->t);
	if (precedence >= 80 && v->t.type == real) printf(")");
}

void node_printer::visit(unary *u) {
	print_token(token(u->op, 0, 0));
	
	int p = precedence;
	precedence = 70;
	
	u->right->accept(this);
	
	precedence = p;
}

void node_printer::visit(binary *b) {
	if (symbols[b->op].precedence < precedence) {
		printf("(");
	}
	int p = precedence;
	precedence = symbols[b->op].precedence;
	
	if (b->op == dot) {
		b->left->accept(this);
		print_token(token(b->op, 0, 0));
		b->right->accept(this);
	}
	else {
		b->left->accept(this);
		printf(" "); print_token(token(b->op, 0, 0)); printf(" ");
		b->right->accept(this);
	}
	
	precedence = p;
	if (symbols[b->op].precedence < precedence) {
		printf(")");
	}
}

void node_printer::visit(subscript *s) {
	s->array->accept(this);
	printf("[");
	
	int p = precedence;
	precedence = 0;
	
	for (auto it = s->indices.begin(); it != s->indices.end(); ++it) {
		if (it != s->indices.begin()) printf(", ");
		(*it)->accept(this);
	}
	
	precedence = p;
	
	printf("]");
}

void node_printer::visit(call *c) {
	if (precedence >= 80) printf("(");
	
	c->function->accept(this);
	printf("(");
	for (auto it = c->args.begin(); it != c->args.end(); ++it) {
		if (it != c->args.begin()) printf(", ");
		(*it)->accept(this);
	}
	printf(")");
	
	if (precedence >= 80) printf(")");
}

void node_printer::visit(statement_error*) {
	printf("<statement error>");
}

void node_printer::visit(assignment *a) {
	a->lvalue->accept(this); printf(" ");
	print_token(token(a->op, 0, 0)); printf(" ");
	a->rvalue->accept(this); printf(";");
}

void node_printer::visit(invocation* i) {
	i->c->accept(this); printf(";");
}

void node_printer::visit(declaration *d) {
	printf("var ");
	for (auto it = d->names.begin(); it != d->names.end(); ++it) {
		if (it != d->names.begin()) printf(", ");
		(*it)->accept(this);
	}
	printf(";");
}

void node_printer::visit(block *b) {
	printf("{\n"); scope++;
	std::for_each(b->stmts.begin(), b->stmts.end(), [&](statement* s) {
		casestatement *c = dynamic_cast<casestatement*>(s);
		if (c) scope--;
		indent(); s->accept(this); printf("\n");
		if (c) scope++;
	});
	scope--; indent(); printf("}");
}

void node_printer::visit(ifstatement *i) {
	printf("if ("); i->cond->accept(this); printf(")");
	print_branch(i->branch_true);
	if (i->branch_false) {
		printf("\n");
		indent(); printf("else");
		
		if (!dynamic_cast<ifstatement*>(i->branch_false)) {
			printf("\n"); print_branch(i->branch_false);
		}
		else {
			printf(" "); i->branch_false->accept(this);
		}
	}
}

void node_printer::visit(whilestatement *w) {
	printf("while ("); w->cond->accept(this); printf(")");
	print_branch(w->stmt);
}

void node_printer::visit(dostatement *d) {
	printf("do");
	print_branch(d->stmt);
	printf(" until ("); d->cond->accept(this); printf(");");
}

void node_printer::visit(repeatstatement *r) {
	printf("repeat ("); r->expr->accept(this); printf(")");
	print_branch(r->stmt);
}

void node_printer::visit(forstatement *f) {
	printf("for (");
		f->init->accept(this); printf(" ");
		f->cond->accept(this); printf("; ");
		f->inc->accept(this); printf("\b");
	printf(")");
	print_branch(f->stmt);
}

void node_printer::visit(switchstatement *s) {
	printf("switch ("); s->expr->accept(this); printf(")");
	print_branch(s->stmts);
}

void node_printer::visit(withstatement *w) {
	printf("with ("); w->expr->accept(this); printf(")");
	print_branch(w->stmt);
}

void node_printer::visit(jump *j) {
	print_token(token(j->type, 0, 0)); printf(";");
}

void node_printer::visit(returnstatement *r) {
	printf("return "); r->expr->accept(this); printf(";");
}

void node_printer::visit(casestatement *c) {
	if (c->expr) {
		printf("case ");
		c->expr->accept(this);
	}
	else
		printf("default");
	
	printf(":");
}

void node_printer::indent() {
	for (size_t i = 0; i < scope; i++)
		printf("    ");
}

void node_printer::print_branch(statement *s) {
	block *b = dynamic_cast<block*>(s);
	if (b) {
		printf(" "); b->accept(this);
	}
	else {
		printf("\n");
		scope++;
		indent(); s->accept(this);
		scope--;
	}
}
