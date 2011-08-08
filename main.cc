#include <cstdio>
#include "file.h"
#include "lexer.h"
#include "parser.h"
#include "printer.h"

void test_parser() {
	file_buffer file(file_descriptor("parser.gml", O_RDONLY));
	token_stream tokens(file);
	parser parser(tokens);
	node_printer printer;
	
	try {
		node *program = parser.getprogram();
		program->accept(&printer); printf("\n");
		delete program;
	}
	catch (unexpected_token_error& e) {
		printf(
			":%lu:%lu: error: unexpected ",
			e.unexpected.row, e.unexpected.col
		);
		print_token(e.unexpected);
		
		if (e.expected.size() > 0) {
			printf("; expected ");
			for (auto it = e.expected.begin(); it != e.expected.end(); ++it) {
				if (it != e.expected.begin()) printf(" or ");
				print_token(token(*it, 0, 0));
			}
		}
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
