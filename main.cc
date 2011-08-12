#include <cstdio>
#include <deque>
#include "file.h"
#include "lexer.h"
#include "parser.h"
#include "printer.h"

class error_printer : public error_stream {
public:
	error_printer(const char *name) : name(name) {}
	
	void push_back(const unexpected_token_error &e) {
		printf(
			"%s:%lu:%lu: error: unexpected ",
			name, e.unexpected.row, e.unexpected.col
		);
		print_token(e.unexpected);
		
		if (e.expected) {
			printf("; expected %s", e.expected);
		}
		
		if (e.expected_tokens.size() > 0) {
			printf("; expected ");
			for (
				auto it = e.expected_tokens.begin();
				it != e.expected_tokens.end(); ++it
			) {
				if (it != e.expected_tokens.begin())
					printf(" or ");
				print_token(token(*it, 0, 0));
			}
		}
		
		printf("\n");
	}

private:
	const char *name;
};

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
	test_parser();
}
