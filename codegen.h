#ifndef CODEGEN_H
#define CODEGEN_H

#include "llvm/LLVMContext.h"
#include "llvm/Support/IRBuilder.h"
#include "node_visitor.h"

class node_codegen : public node_visitor<node_codegen, llvm::Value*> {
public:
	node_codegen() : builder(context) {}
	
	llvm::Value *visit_expression_error(expression_error *e);
	
	llvm::Value *visit_value(value *v);
	llvm::Value *visit_unary(unary *u);
	llvm::Value *visit_binary(binary *b);
	llvm::Value *visit_subscript(subscript *s);
	llvm::Value *visit_call(call *c);
	
	llvm::Value *visit_statement_error(statement_error *e);
	
	llvm::Value *visit_assignment(assignment *a);
	llvm::Value *visit_invocation(invocation* i);
	llvm::Value *visit_declaration(declaration *d);
	llvm::Value *visit_block(block *b);
	
	llvm::Value *visit_ifstatement(ifstatement *i);
	llvm::Value *visit_whilestatement(whilestatement *w);
	llvm::Value *visit_dostatement(dostatement *d);
	llvm::Value *visit_repeatstatement(repeatstatement *r);
	llvm::Value *visit_forstatement(forstatement *f);
	llvm::Value *visit_switchstatement(switchstatement *s);
	llvm::Value *visit_withstatement(withstatement *w);
	
	llvm::Value *visit_jump(jump *j);
	llvm::Value *visit_returnstatement(returnstatement *r);
	llvm::Value *visit_casestatement(casestatement *c);

private:
	llvm::LLVMContext context;
	llvm::IRBuilder<> builder;
};

#endif
