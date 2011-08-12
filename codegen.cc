#include "codegen.h"

using llvm::Value;

using llvm::ConstantFP;
using llvm::APFloat;

using llvm::ConstantArray;
using llvm::StringRef;

Value *node_codegen::visit_expression_error(expression_error*) {
	return 0;
}

Value *node_codegen::visit_value(value *v);
Value *node_codegen::visit_unary(unary *u);
Value *node_codegen::visit_binary(binary *b);
Value *node_codegen::visit_subscript(subscript *s);
Value *node_codegen::visit_call(call *c);

Value *node_codegen::visit_statement_error(statement_error*) {
	return 0;
}

Value *node_codegen::visit_assignment(assignment *a);
Value *node_codegen::visit_invocation(invocation* i);
Value *node_codegen::visit_declaration(declaration *d);
Value *node_codegen::visit_block(block *b);

Value *node_codegen::visit_ifstatement(ifstatement *i);
Value *node_codegen::visit_whilestatement(whilestatement *w);
Value *node_codegen::visit_dostatement(dostatement *d);
Value *node_codegen::visit_repeatstatement(repeatstatement *r);
Value *node_codegen::visit_forstatement(forstatement *f);
Value *node_codegen::visit_switchstatement(switchstatement *s);
Value *node_codegen::visit_withstatement(withstatement *w);

Value *node_codegen::visit_jump(jump *j);
Value *node_codegen::visit_returnstatement(returnstatement *r);
Value *node_codegen::visit_casestatement(casestatement *c);
