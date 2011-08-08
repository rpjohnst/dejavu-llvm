#include "parser.h"

symbol_table symbols;

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
	
	advance(eof);
	return stmt;
}

expression *parser::getexpression(int prec) {
	token t = advance();
	
	nud_parser n = symbols[t.type].nud;
	if (!n) {
		throw unexpected_token_error(t/*, any tokens with .nud */);
	}
	expression *left = (this->*n)(t);
	
	while (prec < symbols[current.type].precedence) {
		t = advance();
		
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
	switch (current.type) {
	case l_paren: return paren_led(advance(), new value { t });
	case l_square: return square_led(advance(), new value { t });
	default: return new value { t };
	}
}

expression *parser::prefix_nud(token t) {
	return new unary { t.type, getexpression(70) };
}

expression *parser::paren_nud(token) {
	expression *expr = getexpression(0);
	advance(r_paren);	
	return expr;
}

expression *parser::infix_led(token t, expression *left) {
	return new binary {
		t.type, left, getexpression(symbols[t.type].precedence)
	};
}

expression *parser::dot_led(token t, expression *left) {
	token n = advance(name);
	return new binary { t.type, left, (this->*symbols[n.type].nud)(n) };
}

expression *parser::square_led(token, expression *left) {
	std::vector<expression*> indices;
	while (current.type != r_square && current.type != eof) {
		indices.push_back(getexpression());
		
		if (current.type == comma) {
			advance();
		}
		else {
			break;
		}
	}
	
	advance(r_square); // or expected comma
	
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
	
	advance(r_paren); // or expected comma
	
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
	
	token_type op = advance().type;
	
	expression *rvalue = getexpression();
	
	return new assignment { op, lvalue, rvalue };
}

statement *parser::var_std() {
	token t = current;
	current = lexer.gettoken();
	
	std::vector<value*> names;
	while (current.type != semicolon && current.type != eof) {
		token n = advance(name);
		names.push_back((value*)(this->*symbols[n.type].nud)(n));
		
		if (current.type == comma) {
			current = lexer.gettoken();
		}
	}
	
	advance(semicolon);
	
	return new declaration { t, names };
}

statement *parser::brace_std() {
	current = lexer.gettoken();
	
	std::vector<statement*> stmts;
	while (current.type != r_brace && current.type != eof) {
		stmts.push_back(getstatement());
	}
	
	advance(r_brace);

	return new block { stmts };
}

statement *parser::if_std() {
	current = lexer.gettoken();
	expression *cond = getexpression();
	
	if (current.type == kw_then) {
		advance();
	}
	statement *branch_true = getstatement();
	
	statement *branch_false = 0;
	if (current.type == kw_else) {
		advance();
		branch_false = getstatement();
	}
	
	return new ifstatement { cond, branch_true, branch_false };
}

statement *parser::while_std() {
	current = lexer.gettoken();
	expression *cond = getexpression();
	
	if (current.type == kw_do) {
		advance();
	}
	statement *stmt = getstatement();
	
	return new whilestatement { cond, stmt };
}

statement *parser::do_std() {
	current = lexer.gettoken();
	statement *stmt = getstatement();
	
	advance(kw_until);
	
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
	
	advance(l_paren);
	
	statement *init = getstatement();
	
	expression *cond = getexpression();
	if (current.type == semicolon) {
		advance();
	}
	
	statement *inc = getstatement();
	
	advance(r_paren);
	
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
		advance();
	}
	statement *stmt = getstatement();
	
	return new withstatement { expr, stmt };
}

statement *parser::jump_std() {
	return new jump { advance().type };
}

statement *parser::return_std() {
	advance();
	
	expression *expr = getexpression();
	
	return new returnstatement { expr };
}

statement *parser::case_std() {
	token t = advance();
	
	expression *expr = 0;
	if (t.type == kw_case) {
		expr = getexpression();
	}
	
	advance(colon);
	
	return new casestatement { expr };
}

token parser::advance() {
	token r = current;
	current = lexer.gettoken();
	return r;
}

token parser::advance(token_type t) {
	if (current.type != t) {
		throw unexpected_token_error(current, t);
	}
	return advance();
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
