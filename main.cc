#include <cstdio>
#include "dejavu/codegen.h"
#include "dejavu/file.h"
#include "dejavu/lexer.h"
#include "dejavu/parser.h"
#include "dejavu/printer.h"

class error_printer : public error_stream {
public:
	error_printer(const char *name) : name(name) {}
	
	void push_back(const unexpected_token_error &e) {
		printf(
			"%s:%lu:%lu: error: unexpected '",
			name, e.unexpected.row, e.unexpected.col
		);
		print_token(e.unexpected);
		
		if (e.expected) {
			printf("'; expected %s", e.expected);
		}
		else {
			printf("'; expected '");
			print_token(token(e.expected_token, 0, 0));
			printf("'");
		}
		
		printf("\n");
	}

private:
	const char *name;
};

void test_compiler() {
	file_buffer file;
	file.open_file("compiler.gml", O_RDONLY);
	error_printer errors("compiler.gml");
	
	token_stream tokens(file);
	parser parser(tokens, errors);
	
	node_codegen compiler;
	node *program = parser.getprogram();
	compiler.get_module(program).dump();
	delete program;
}

void test_parser() {
	file_buffer file;
	file.open_file("parser.gml", O_RDONLY);
	
	token_stream tokens(file);
	error_printer errors("parser.gml");
	parser parser(tokens, errors);
	
	node_printer printer;
	node *program = parser.getprogram();
	printer.visit(program); printf("\n");
	delete program;
}

void test_lexer() {
	file_buffer file;
	file.open_file("lexer.gml", O_RDONLY);
	
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
	// test_parser();
	test_compiler();
}
