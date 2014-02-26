#ifndef CODEGEN_H
#define CODEGEN_H

#include "dejavu/compiler/node_visitor.h"
#include <dejavu/compiler/error_stream.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <map>
#include <string>

namespace llvm {
	class DataLayout;
}

class node_codegen : public node_visitor<node_codegen, llvm::Value*> {
public:
	node_codegen(const llvm::DataLayout*, error_stream &e);
	llvm::Function *add_function(node*, const char *name, size_t nargs, bool var);
	llvm::Module &get_module() { return module; }

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
	llvm::Function *get_function(const char *name, int args, bool var);
	llvm::Function *get_operator(const char *name, int args);

	llvm::Value *get_real(double val);
	llvm::Value *get_string(int length, const char *val);
	llvm::Value *to_bool(node *val);
	llvm::Value *is_equal(llvm::Value *a, llvm::Value *b);

	llvm::AllocaInst *alloc(llvm::Type *type, const llvm::Twine &name = "");
	llvm::Value *do_lookup(llvm::Value *left, llvm::Value *right);

	llvm::LLVMContext context;
	llvm::IRBuilder<> builder;
	llvm::Module module;
	const llvm::DataLayout *dl;

	// runtime types
	llvm::PointerType *scope_type;
	llvm::StructType *var_type;
	llvm::StructType *variant_type;
	llvm::Type *real_type;
	llvm::StructType *string_type;
	int union_diff;

	// runtime functions
	llvm::Function *to_real;
	llvm::Function *to_string;

	llvm::Function *lookup;
	llvm::Function *access;

	llvm::Function *with_begin;
	llvm::Function *with_inc;

	// scope handling
	std::map<std::string, llvm::Value*> scope;
	llvm::Instruction *alloca_point = 0;
	llvm::Value *self_scope = 0;
	llvm::Value *other_scope = 0;
	llvm::Value *return_value = 0;
	llvm::Value *passed_args = 0;

	llvm::BasicBlock *current_loop = 0;
	llvm::BasicBlock *current_end = 0;

	llvm::Function::iterator current_cond = 0;
	llvm::BasicBlock *current_default = 0;
	llvm::Value *current_switch = 0;

	error_stream& errors;
};

#endif
