#include "parser.h"
#include <map>

typedef expression *(parser::*nud_parser)(token);
typedef expression *(parser::*led_parser)(token, expression*);

struct symbol {
	int precedence;
	nud_parser nud;
	led_parser led;
};

// uhm let's just add a constructor here
class symbol_table : public std::map<token_type, symbol> {
public:
	symbol_table();

private:
	void prefix(token_type t, nud_parser nud = &parser::prefix_nud) {
		(*this)[t].nud = nud;
	}

	void infix(token_type t, int prec, led_parser led = &parser::infix_led) {
		(*this)[t].precedence = prec;
		(*this)[t].led = led;
	}
} symbols;

symbol_table::symbol_table() {
	symbols[real].nud = symbols[string].nud =
	symbols[kw_self].nud = symbols[kw_other].nud =
	symbols[kw_all].nud = symbols[kw_noone].nud =
	symbols[kw_global].nud = &parser::id_nud;
	
	symbols[name].nud = &parser::name_und;
	infix(dot, 80, &parser::dot_led);
	
	prefix(l_paren, &parser::paren_nud);
	
	prefix(exclaim);
	prefix(tilde);
	prefix(minus);
	
	infix(times, 60);
	infix(divide, 60);
	infix(kw_div, 60);
	infix(kw_mod, 60);
	
	infix(plus, 50);
	infix(minus, 50);
	
	infix(shift_left, 40);
	infix(shift_right, 40);
	
	infix(bit_and, 30);
	infix(bit_or, 30);
	infix(bit_xor, 30);
	
	infix(less, 20);
	infix(less_equals, 20);
	infix(is_equals, 20);
	infix(equals, 20);
	infix(not_equals, 20);
	infix(greater, 20);
	infix(greater_equals, 20);
	
	infix(ampamp, 10);
	infix(pipepipe, 10);
	infix(caretcaret, 10);
}

parser::parser(token_stream& l) : lexer(l), current(lexer.gettoken()) {}

expression *parser::getexpression(int prec) {
	token t = current;
	current = lexer.gettoken();
	expression *left = (this->*symbols[t.type].nud)(t);
	
	while (prec < symbols[current.type].precedence) {
		t = current;
		current = lexer.gettoken();
		left = (this->*symbols[t.type].led)(t, left);
	}
	
	return left;
}

expression *parser::id_nud(token t) {
	return new value { t };
}

expression *parser::name_nud(token t) {
	token o;
	switch (current.type) {
	case l_paren:
		o = current;
		current = lexer.gettoken();
		return paren_led(o, new value { t });
	
	case l_square:
		o = current;
		current = lexer.gettoken();
		return square_led(o, new value { t });
	
	default: return new value { t };
	}
}

expression *parser::prefix_nud(token t) {
	return new unary { t.type, getexpression(70) };
}

expression *parser::paren_nud(token) {
	expression *expr = getexpression(0);
	if (current.type != r_paren) {
		throw unexpected_token_error(current);
	}
	current = lexer.gettoken();
	
	return expr;
}

expression *parser::infix_led(token t, expression *left) {
	return new binary {
		t.type, left, getexpression(symbols[t.type].precedence)
	};
}

expression *parser::dot_led(token t, expression *left) {
	if (current.type != name) {
		throw unexpected_token_error(current);
	}
	
	token n = current;
	current = lexer.gettoken();
	return new binary { t.type, left, (this->*symbols[n.type].nud)(n) };
}

expression *parser::square_led(token, expression *left) {
	std::vector<expression*> indices;
	
	while (current.type != r_square && current.type != eof) {
		indices.push_back(getexpression());
		
		if (current.type == comma) {
			current = lexer.gettoken();
		}
		else {
			break;
		}
	}
	
	if (current.type != r_square) {
		throw unexpected_token_error(current);
	}
	current = lexer.gettoken();
	
	return new subscript { left, indices };
}

expression *parser::paren_led(token, expression *left) {
	std::vector<expression*> args;
	while (current.type != r_paren && current.type != eof) {
		args.push_back(getexpression(0));
		
		if (current.type == comma) {
			current = lexer.gettoken();
		}
		else {
			break;
		}
	}
	
	if (current.type != r_paren) {
		throw unexpected_token_error(current);
	}
	current = lexer.gettoken();
	
	return new call { left, args };
}
