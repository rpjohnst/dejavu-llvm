#include <string>
#include <cstdio>
#include "file.h"
#include "lexer.h"

int main() {
	file_buffer file(file_descriptor("test.gml", O_RDONLY));
	token_stream tokens(file);
	token t;
	
	do {
		t = tokens.gettoken();

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
		
#		define KEYWORD(X) case kw_ ## X: printf("(" #X ") "); break;
#		include "tokens.tbl"
		
#		define OPERATOR(X, Y) case X: printf(Y " "); break;
#		include "tokens.tbl"
		
		default:
			printf("(unknown) ");
		}
	} while (t.type != eof);
	
	printf("\n");
}
