#include "parser.h"
#include <map>

parser::parser(token_stream& l) : lexer(l), current(lexer.gettoken()) {}

node *parser::getprogram() {
	statement *stmt;
	if (current.type == l_brace) {
		stmt = getstatement();
	}
	else {
		std::vector<statement*> stmts;
		while (current.type != eof) {
			stmts.push_back(getstatement());
		}
		
		stmt = new block { stmts };
	}
	
	if (current.type != eof) {
		throw unexpected_token_error(current, eof);
	}
	return stmt;
}

typedef statement *(parser::*std_parser)();
typedef expression *(parser::*nud_parser)(token);
typedef expression *(parser::*led_parser)(token, expression*);

struct symbol {
	int precedence;
	std_parser std;
	nud_parser nud;
	led_parser led;
};

class symbol_table : public std::map<token_type, symbol> {
public:
	symbol_table();

private:
	void prefix(token_type t, nud_parser nud = &parser::prefix_nud);
	void infix(token_type t, int prec, led_parser led = &parser::infix_led);
} symbols;

expression *parser::getexpression(int prec) {
	token t = current;
	current = lexer.gettoken();
	
	nud_parser n = symbols[t.type].nud;
	if (!n) {
		throw unexpected_token_error(t);
	}
	expression *left = (this->*n)(t);
	
	while (prec < symbols[current.type].precedence) {
		t = current;
		current = lexer.gettoken();
		
		led_parser l = symbols[t.type].led;
		if (!l) {
			throw unexpected_token_error(t);
		}
		left = (this->*l)(t, left);
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
		throw unexpected_token_error(current, r_paren);
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
		throw unexpected_token_error(current, name);
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
		throw unexpected_token_error(current, comma, r_square);
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
		throw unexpected_token_error(current, comma, r_paren);
	}
	current = lexer.gettoken();
	
	return new call { left, args };
}

statement *parser::getstatement() {
	std_parser s = symbols[current.type].std;
	if (!s) {
		throw unexpected_token_error(current);
	}
	statement *stmt = (this->*s)();
	
	while (current.type == semicolon) {
		current = lexer.gettoken();
	}
	
	return stmt;
}

bool isassignment(token_type t) {
	switch (t) {
	case equals:
	case plus_equals: case minus_equals:
	case times_equals: case div_equals:
	case and_equals: case or_equals: case xor_equals:
		return true;
	
	default:
		return false;
	}
}

statement *parser::expr_std() {
	expression *lvalue = getexpression(symbols[equals].precedence);
	
	call *b = dynamic_cast<call*>(lvalue);
	if (b && !isassignment(current.type)) {
		return new invocation { dynamic_cast<call*>(lvalue) };
	}
	
	if (!isassignment(current.type)) {
		throw unexpected_token_error(
			current,
			equals, plus_equals, minus_equals,
			times_equals, div_equals,
			and_equals, or_equals,
			xor_equals
		);
	}
	
	token_type op = current.type;
	current = lexer.gettoken();
	
	expression *rvalue = getexpression();
	
	return new assignment { op, lvalue, rvalue };
}

statement *parser::var_std() {
	token t = current;
	current = lexer.gettoken();
	
	std::vector<value*> names;
	while (current.type != semicolon && current.type != eof) {
		if (current.type != name) {
			throw unexpected_token_error(current, name);
		}
		
		token n = current;
		current = lexer.gettoken();
		names.push_back((value*)(this->*symbols[n.type].nud)(n));
		
		if (current.type == comma) {
			current = lexer.gettoken();
		}
	}
	
	if (current.type != semicolon) {
		throw unexpected_token_error(current);
	}
	current = lexer.gettoken();
	
	return new declaration { t, names };
}

statement *parser::brace_std() {
	current = lexer.gettoken();
	
	std::vector<statement*> stmts;
	while (current.type != r_brace && current.type != eof) {
		stmts.push_back(getstatement());
	}
	
	if (current.type == eof) {
		throw unexpected_token_error(current, r_brace);
	}
	current = lexer.gettoken();
	
	return new block { stmts };
}

statement *parser::if_std() {
	current = lexer.gettoken();
	expression *cond = getexpression();
	
	if (current.type == kw_then) {
		current = lexer.gettoken();
	}
	statement *branch_true = getstatement();
	
	statement *branch_false = 0;
	if (current.type == kw_else) {
		current = lexer.gettoken();
		branch_false = getstatement();
	}
	
	return new ifstatement { cond, branch_true, branch_false };
}

statement *parser::while_std() {
	current = lexer.gettoken();
	expression *cond = getexpression();
	
	if (current.type == kw_do) {
		current = lexer.gettoken();
	}
	statement *stmt = getstatement();
	
	return new whilestatement { cond, stmt };
}

statement *parser::do_std() {
	current = lexer.gettoken();
	statement *stmt = getstatement();
	
	if (current.type != kw_until) {
		throw unexpected_token_error(current, kw_until);
	}
	current = lexer.gettoken();
	
	expression *cond = getexpression();
	
	return new dostatement { cond, stmt };
}

statement *parser::repeat_std() {
	current = lexer.gettoken();
	expression *count = getexpression();
	statement *stmt = getstatement();
	return new repeatstatement { count, stmt };
}

statement *parser::for_std() {
	current = lexer.gettoken();
	
	if (current.type != l_paren) {
		throw unexpected_token_error(current, l_paren);
	}
	current = lexer.gettoken();
	
	statement *init = getstatement();
	
	expression *cond = getexpression();
	if (current.type == semicolon) {
		current = lexer.gettoken();
	}
	
	statement *inc = getstatement();
	
	if (current.type != r_paren) {
		throw unexpected_token_error(current, r_paren);
	}
	current = lexer.gettoken();
	
	statement *stmt = getstatement();
	
	return new forstatement { init, cond, inc, stmt };
}

statement *parser::switch_std() {
	current = lexer.gettoken();
	expression *expr = getexpression();
	
	if (current.type != l_brace) {
		throw unexpected_token_error(current, l_brace);
	}
	block *stmts = static_cast<block*>(brace_std());
	
	return new switchstatement { expr, stmts };
}

statement *parser::with_std() {
	current = lexer.gettoken();
	expression *expr = getexpression();
	
	if (current.type == kw_do) {
		current = lexer.gettoken();
	}
	statement *stmt = getstatement();
	
	return new withstatement { expr, stmt };
}

statement *parser::jump_std() {
	token t = current;
	current = lexer.gettoken();
	
	return new jump { t.type };
}

statement *parser::return_std() {
	current = lexer.gettoken();
	
	expression *expr = getexpression();
	
	return new returnstatement { expr };
}

statement *parser::case_std() {
	token t = current;
	current = lexer.gettoken();
	
	expression *expr = 0;
	if (t.type == kw_case) {
		expr = getexpression();
	}
	
	if (current.type != colon) {
		throw unexpected_token_error(current, colon);
	}
	current = lexer.gettoken();
	
	return new casestatement { expr };
}

void symbol_table::prefix(token_type t, nud_parser nud) {
	(*this)[t].nud = nud;
}

void symbol_table::infix(token_type t, int prec, led_parser led) {
	(*this)[t].precedence = prec;
	(*this)[t].led = led;
}

symbol_table::symbol_table() {
	symbols[real].nud = symbols[string].nud =
	symbols[kw_self].nud = symbols[kw_other].nud =
	symbols[kw_all].nud = symbols[kw_noone].nud =
	symbols[kw_global].nud =
	symbols[kw_true].nud = symbols[kw_false].nud =
	&parser::id_nud;
	
	symbols[name].nud = &parser::name_nud;
	infix(dot, 80, &parser::dot_led);
	
	prefix(l_paren, &parser::paren_nud);
	
	prefix(exclaim);
	prefix(tilde);
	prefix(minus);
	prefix(plus);
	
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
	
	symbols[kw_var].std =
	symbols[kw_globalvar].std = &parser::var_std;
	
	symbols[name].std = symbols[l_paren].std =
	symbols[kw_self].std = symbols[kw_other].std =
	symbols[kw_all].std = symbols[kw_noone].std =
	symbols[kw_global].std = &parser::expr_std;
	
	symbols[l_brace].std = &parser::brace_std;
	
	symbols[kw_if].std = &parser::if_std;
	symbols[kw_while].std = &parser::while_std;
	symbols[kw_do].std = &parser::do_std;
	symbols[kw_repeat].std = &parser::repeat_std;
	symbols[kw_for].std = &parser::for_std;
	symbols[kw_switch].std = &parser::switch_std;
	symbols[kw_with].std = &parser::with_std;
	
	symbols[kw_break].std = symbols[kw_continue].std =
	symbols[kw_exit].std = &parser::jump_std;
	symbols[kw_return].std = &parser::return_std;
	symbols[kw_case].std = symbols[kw_default].std =
	&parser::case_std;
}
