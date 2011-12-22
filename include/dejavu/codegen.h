#ifndef CODEGEN_H
#define CODEGEN_H

#include "dejavu/node_visitor.h"
#include "llvm/LLVMContext.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Module.h"
#include <map>
#include <string>

namespace llvm {
	class TargetData;
}

class node_codegen : public node_visitor<node_codegen, llvm::Value*> {
public:
	node_codegen(const llvm::TargetData*);
	llvm::Module &get_module(node*);

	llvm::Value *visit_value(value *v);
	llvm::Value *visit_unary(unary *u);
	llvm::Value *visit_binary(binary *b);
	llvm::Value *visit_subscript(subscript *s);
	llvm::Value *visit_call(call *c);

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
	llvm::Function *get_function(const char *name, int args);
	llvm::Value *get_real(double val);
	llvm::Value *get_string(int length, const char *val);

	llvm::LLVMContext context;
	llvm::IRBuilder<> builder;
	llvm::Module module;
	const llvm::TargetData *td;

	llvm::Type *real_type;
	llvm::StructType *string_type;
	llvm::StructType *variant_type;
	llvm::StructType *var_type;

	llvm::Function *lookup;
	llvm::Function *to_real;
	llvm::Function *to_string;

	std::map<std::string, llvm::AllocaInst*> scope;
	llvm::BasicBlock *current_loop;
	llvm::BasicBlock *current_end;
};

#endif
