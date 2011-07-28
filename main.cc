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

struct expression_printer : public expression::visitor {
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
};

void test_parser() {
	file_buffer file(file_descriptor("parser.gml", O_RDONLY));
	token_stream tokens(file);
	parser parser(tokens);
	expression_printer p;
	
	try {
		while (true) {
			expression *expr = parser.getexpression();
			expr->accept(&p); printf("\n"); fflush(stdout);
			delete expr;
		}
	}
	catch (unexpected_token_error& e) {
		printf("unexpected ");
		print_token(e.tok);
		printf("\n");
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
