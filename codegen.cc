#include "codegen.h"
#include <algorithm>
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"

using namespace llvm;

node_codegen::node_codegen() :
	builder(context), module("", context) {
	
	Type *variant = StructType::get(
		context,
		Type::getInt32Ty(context),
		StructType::get(
			context,
			Type::getInt32Ty(context),
			Type::getInt8PtrTy(context),
			NULL
		),
		NULL
	);
	module.addTypeName("variant", variant);
}

Module &node_codegen::get_module(node *program) {
	FunctionType *type = FunctionType::get(
		Type::getVoidTy(context), false
	);
	Function *function = Function::Create(
		type, Function::ExternalLinkage, "main", &module
	);
	
	BasicBlock *entry = BasicBlock::Create(context, "entry", function);
	builder.SetInsertPoint(entry);
	
	visit(program);
	
	builder.CreateRetVoid();
	
	return module;
}

Value *node_codegen::visit_expression_error(expression_error*) {
	return 0;
}

Value *node_codegen::visit_value(value *v) {
	switch (v->t.type) {
	default: return 0;
	
	case real: {
		Constant *real = ConstantFP::get(
			Type::getDoubleTy(context), v->t.real
		);
		return builder.CreateCall(get_function("create_real", 1), real);
	}
	
	case string: {
		Constant *array = ConstantArray::get(
			context,
			StringRef(v->t.string.data, v->t.string.length),
			false
		);
		GlobalVariable *string = new GlobalVariable(
			module,
			array->getType(), true,
			GlobalValue::PrivateLinkage, array,
			"string"
		);
		Constant *zero = ConstantInt::get(
			Type::getInt32Ty(context), 0
		);
		Constant *indices[] = { zero, zero };
		
		Constant *data = ConstantExpr::getGetElementPtr(
			string, indices, 2, true
		);
		Constant *length = ConstantInt::get(
			Type::getInt32Ty(context), v->t.string.length
		);
		
		Function *create_string = get_function("create_string", 2);
		return builder.CreateCall2(create_string, data, length);
	}
	
	case name: {
		std::string name(v->t.string.data, v->t.string.length);
		return builder.CreateLoad(scope[name], name);
	}
	}
}

Value *node_codegen::visit_unary(unary *u) {
	const char *name;
	switch (u->op) {
	default: return 0;
	case exclaim: name = "not"; break;
	case tilde: name = "inv"; break;
	case minus: name = "neg"; break;
	case plus: name = "pos"; break;
	}
	
	return builder.CreateCall(get_function(name, 1), visit(u->right));
}

Value *node_codegen::visit_binary(binary *b) {
	const char *name;
	switch (b->op) {
	default: return 0;
	case plus: name = "plus"; break;
	case times: name = "times"; break;
	case divide: name = "divide"; break;
	}
	
	return builder.CreateCall2(
		get_function(name, 2),
		visit(b->left), visit(b->right)
	);
}

Value *node_codegen::visit_subscript(subscript *s) {
	return 0;
}

Value *node_codegen::visit_call(call *c) {
	value *f = static_cast<value*>(c->function);
	
	std::string name(f->t.string.data, f->t.string.length);
	Function *function = get_function(name.c_str(), c->args.size());
	
	struct vector_codegen {
		vector_codegen(node_codegen &v) : v(v) {}
		Value *operator() (expression *e) {
			return v.visit(e);
		}
		node_codegen &v;
	} t(*this);
	std::vector<Value*> args(c->args.size());
	std::transform(c->args.begin(), c->args.end(), args.begin(), t);
	
	return builder.CreateCall(function, args.begin(), args.end());
}

Value *node_codegen::visit_statement_error(statement_error*) {
	return 0;
}

Value *node_codegen::visit_assignment(assignment *a) {
	value *lvalue = static_cast<value*>(a->lvalue);
	std::string name(lvalue->t.string.data, lvalue->t.string.length);
	builder.CreateStore(visit(a->rvalue), scope[name]);
	return 0;
}

Value *node_codegen::visit_invocation(invocation* i) {
	
}

Value *node_codegen::visit_declaration(declaration *d) {
	for (
		std::vector<value*>::iterator it = d->names.begin();
		it != d->names.end(); ++it
	) {
		std::string name((*it)->t.string.data, (*it)->t.string.length);
		scope[name] = builder.CreateAlloca(
			module.getTypeByName("variant"), 0, name
		);
	}
	
	return 0;
}

Value *node_codegen::visit_block(block *b) {
	for (
		std::vector<statement*>::iterator it = b->stmts.begin();
		it != b->stmts.end(); ++it
	) {
		visit(*it);
	}
	
	return 0;
}

Value *node_codegen::visit_ifstatement(ifstatement *i) {
	
}

Value *node_codegen::visit_whilestatement(whilestatement *w) {
	
}

Value *node_codegen::visit_dostatement(dostatement *d) {
	
}

Value *node_codegen::visit_repeatstatement(repeatstatement *r) {
	
}

Value *node_codegen::visit_forstatement(forstatement *f) {
	
}

Value *node_codegen::visit_switchstatement(switchstatement *s) {
	
}

Value *node_codegen::visit_withstatement(withstatement *w) {
	
}

Value *node_codegen::visit_jump(jump *j) {
	
}

Value *node_codegen::visit_returnstatement(returnstatement *r) {
	
}

Value *node_codegen::visit_casestatement(casestatement *c) {
	
}

Function *node_codegen::get_function(const char *name, int args) {
	Function *function = module.getFunction(name);
	if (!function) {
		const Type *variant = module.getTypeByName("variant");
		FunctionType *type = FunctionType::get(
			variant, std::vector<const Type*>(args, variant), false
		);
		function = Function::Create(
			type, Function::ExternalLinkage, name, &module
		);
	}
	return function;
}
