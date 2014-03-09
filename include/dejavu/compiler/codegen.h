#ifndef CODEGEN_H
#define CODEGEN_H

#include <dejavu/compiler/node_visitor.h>
#include <dejavu/compiler/error_stream.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/ADT/StringMap.h>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace llvm {
	class DataLayout;
}

class node_codegen : public node_visitor<node_codegen, llvm::Value*> {
public:
	node_codegen(const llvm::DataLayout*, error_stream &e);
	llvm::Function *add_function(
		node*, const char *name, size_t nargs, bool var
	);
	llvm::Module &get_module() { return module; }

	void register_script(const std::string &name);

// really should be private
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
	llvm::Function *get_function(llvm::StringRef name, int args, bool var);
	llvm::Function *get_operator(llvm::StringRef name, int args);

	llvm::Value *get_real(double val);
	llvm::Value *get_real(llvm::Value *val);
	llvm::Value *get_string(llvm::StringRef val);

	llvm::Value *to_bool(node *val);
	llvm::Value *is_equal(llvm::Value *a, llvm::Value *b);

	llvm::Value *make_local(llvm::StringRef name, llvm::Value *value);
	llvm::Value *make_local(
		llvm::StringRef name, llvm::Value *x, llvm::Value *y,
		llvm::Value *values
	);

	llvm::AllocaInst *alloc(llvm::Type*, const llvm::Twine&);
	llvm::AllocaInst *alloc(llvm::Type*, llvm::Value*, const llvm::Twine&);

	llvm::Value *do_lookup(llvm::Value *left, llvm::Value *right);

	llvm::LLVMContext context;
	llvm::IRBuilder<> builder;
	llvm::Module module;
	const llvm::DataLayout *dl;

	llvm::StringMap<llvm::GlobalVariable*> string_literals;
	std::unordered_set<std::string> scripts;

	// runtime types
	llvm::PointerType *scope_type;
	llvm::StructType *var_type;
	llvm::StructType *variant_type;
	llvm::Type *real_type;
	llvm::Type *string_type;
	int union_diff;

	// runtime functions
	llvm::Function *to_real;
	llvm::Function *to_string;

	llvm::Function *lookup;
	llvm::Function *access;

	llvm::Function *retain;
	llvm::Function *release;
	llvm::Function *retain_var;
	llvm::Function *release_var;

	llvm::Function *with_begin;
	llvm::Function *with_inc;

	// scope handling
	std::unordered_map<std::string, llvm::Value*> scope;
	llvm::Instruction *alloca_point = 0;
	llvm::Value *return_value = 0;
	llvm::Value *self_scope = 0;
	llvm::Value *other_scope = 0;

	llvm::BasicBlock *current_loop = 0;
	llvm::BasicBlock *current_end = 0;

	llvm::Function::iterator current_cond = 0;
	llvm::BasicBlock *current_default = 0;
	llvm::Value *current_switch = 0;

	error_stream& errors;
};

inline void node_codegen::register_script(const std::string &name) {
	scripts.insert(name);
}

inline llvm::AllocaInst *node_codegen::alloc(
	llvm::Type *type, const llvm::Twine &name = ""
) {
	return alloc(type, NULL, name);
}
inline llvm::AllocaInst *node_codegen::alloc(
	llvm::Type *type, llvm::Value *n, const llvm::Twine &name = ""
) {
	return new llvm::AllocaInst(type, n, name, alloca_point);
}

#endif
