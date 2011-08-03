#include <string>
#include <cstdio>
#include <algorithm>
#include "file.h"
#include "lexer.h"
#include "parser.h"

void print_token(const token& t) {
	switch (t.type) {
	case eof:
		printf("(eof:%lu:%lu) ", t.row, t.col);
		break;
	
	case name:
		printf("(name:%lu:%lu %.*s) ", t.row, t.col, (int)t.string.length, t.string.data);
		break;
	
	case real:
		printf("(real:%lu:%lu %f) ", t.row, t.col, t.real);
		break;
	
	case string:
		printf("(string:%lu:%lu \"%.*s\") ", t.row, t.col, (int)t.string.length, t.string.data);
		break;
	
#	define KEYWORD(X) case kw_ ## X: printf("(" #X ") "); break;
#	include "tokens.tbl"
	
#	define OPERATOR(X, Y) case X: printf(Y " "); break;
#	include "tokens.tbl"
	
	default:
		printf("(unknown) ");
	}
}

struct ast_printer : public node::visitor {
	void visit(value *v) {
		print_token(v->t);
	}
	
	void visit(unary *u) {
		print_token(token(u->op, 0, 0));
		u->right->accept(this);
	}
	
	void visit(binary *b) {
		b->left->accept(this);
		print_token(token(b->op, 0, 0));
		b->right->accept(this);
	}
	
	void visit(subscript *s) {
		s->array->accept(this);
		printf("[");
		std::for_each(s->indices.begin(), s->indices.end(), [&](expression *e) {
			e->accept(this);
			printf(", ");
		});
		printf("] ");
	}
	
	void visit(call *c) {
		c->function->accept(this);
		printf("(");
		std::for_each(c->args.begin(), c->args.end(), [&](expression *e) {
			e->accept(this);
			printf(", ");
		});
		printf(")");
	}
	
	void visit(assignment *a) {
		a->lvalue->accept(this);
		print_token(token(a->op, 0, 0));
		a->rvalue->accept(this);
		printf(";");
	}
	
	void visit(invocation* i) {
		i->c->accept(this);
	}
	
	void visit(declaration *d) {
		printf("var ");
		std::for_each(d->names.begin(), d->names.end(), [&](value* v) {
			v->accept(this);
			printf(", ");
		});
		printf("; ");
	}
	
	void visit(block *b) {
		printf("{\n");
		std::for_each(b->stmts.begin(), b->stmts.end(), [&](statement* s) {
			s->accept(this);
			printf("\n");
		});
		printf("}");
	}
	
	void visit(ifstatement *i) {
		printf("if "); i->cond->accept(this); printf("\n");
		i->branch_true->accept(this);
		if (i->branch_false) {
			printf("\n");
			printf("else "); printf("\n");
			i->branch_false->accept(this);
		}
	}
	
	void visit(whilestatement *w) {
		printf("while "); w->cond->accept(this); printf("\n");
		w->stmt->accept(this);
	}
	
	void visit(dostatement *d) {
		printf("do "); printf("\n");
		d->stmt->accept(this); printf("\n");
		printf("until "); d->cond->accept(this);
	}
	
	void visit(repeatstatement *r) {
		printf("repeat "); r->expr->accept(this); printf("\n");
		r->stmt->accept(this);
	}
	
	void visit(forstatement *f) {
		printf("for (");
			f->init->accept(this);
			f->cond->accept(this); printf("; ");
			f->inc->accept(this);
		printf(")\n");
		f->stmt->accept(this);
	}
	
	void visit(switchstatement *s) {
		printf("switch "); s->expr->accept(this); printf("\n");
		s->stmts->accept(this);
	}
	
	void visit(withstatement *w) {
		printf("with "); w->expr->accept(this); printf("\n");
		w->stmt->accept(this);
	}
	
	void visit(jump *j) {
		print_token(token(j->type, 0, 0));
	}
	
	void visit(returnstatement *r) {
		printf("return ");
		r->expr->accept(this);
	}
	
	void visit(casestatement *c) {
		if (c->expr) {
			printf("case ");
			c->expr->accept(this);
		}
		else
			printf("default");
		
		printf(": ");
	}
};

void test_parser() {
	file_buffer file(file_descriptor("parser.gml", O_RDONLY));
	token_stream tokens(file);
	parser parser(tokens);
	ast_printer p;
	
	try {
		node *program = parser.getprogram();
		program->accept(&p); printf("\n");
		delete program;
	}
	catch (unexpected_token_error& e) {
		printf(
			":%lu:%lu: error: unexpected '%.*s'\n",
			e.tok.row, e.tok.col,
			(int)e.tok.string.length, e.tok.string.data
		);
	}
}

void test_lexer() {
	file_buffer file(file_descriptor("lexer.gml", O_RDONLY));
	token_stream tokens(file);
	token t;
	
	do {
		t = tokens.gettoken();
		print_token(t);
	} while (t.type != eof);
	
	printf("\n");
}

int main() {
	// test_lexer();
	test_parser();
}
