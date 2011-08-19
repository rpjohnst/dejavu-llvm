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
	
	case kw_self: {
		Constant *real = ConstantFP::get(Type::getDoubleTy(context), -1);
		return builder.CreateCall(get_function("create_real", 1), real);
	}
	case kw_other: {
		Constant *real = ConstantFP::get(Type::getDoubleTy(context), -2);
		return builder.CreateCall(get_function("create_real", 1), real);
	}
	case kw_all: {
		Constant *real = ConstantFP::get(Type::getDoubleTy(context), -3);
		return builder.CreateCall(get_function("create_real", 1), real);
	}
	case kw_noone: {
		Constant *real = ConstantFP::get(Type::getDoubleTy(context), -4);
		return builder.CreateCall(get_function("create_real", 1), real);
	}
	case kw_global: {
		Constant *real = ConstantFP::get(Type::getDoubleTy(context), -5);
		return builder.CreateCall(get_function("create_real", 1), real);
	}
	
	case kw_true: {
		Constant *real = ConstantFP::get(Type::getDoubleTy(context), 1);
		return builder.CreateCall(get_function("create_real", 1), real);
	}
	case kw_false: {
		Constant *real = ConstantFP::get(Type::getDoubleTy(context), 0);
		return builder.CreateCall(get_function("create_real", 1), real);
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
	
	case dot:
		visit(b->left);
		return visit_value(static_cast<value*>(b->right));
		break;
	
	// todo: can we pull this out of a table instead of copypasting?
	case less: name = "less"; break;
	case less_equals: name = "less_equals"; break;
	case is_equals: name = "is_equals"; break;
	case not_equals: name = "not_equals"; break;
	case greater_equals: name = "greater_equals"; break;
	case greater: name = "greater"; break;
	
	case plus: name = "plus"; break;
	case minus: name = "minus"; break;
	case times: name = "times"; break;
	case divide: name = "divide"; break;
	
	case ampamp: name = "ampamp"; break;
	case pipepipe: name = "pipepipe"; break;
	case caretcaret: name = "caretcaret"; break;
	
	case bit_and: name = "bit_and"; break;
	case bit_or: name = "bit_or"; break;
	case bit_xor: name = "bit_xor"; break;
	case shift_left: name = "shift_left"; break;
	case shift_right: name = "shift_right"; break;
	
	case kw_div: name = "div"; break;
	case kw_mod: name = "mod"; break;
	}
	
	return builder.CreateCall2(
		get_function(name, 2),
		visit(b->left), visit(b->right)
	);
}

Value *node_codegen::visit_subscript(subscript *s) {
	return 0;
}

struct vector_codegen {
	vector_codegen(node_codegen &v) : v(v) {}
	Value *operator() (expression *e) {
		return v.visit(e);
	}
	node_codegen &v;
};

Value *node_codegen::visit_call(call *c) {
	value *f = static_cast<value*>(c->function);
	
	std::string name(f->t.string.data, f->t.string.length);
	Function *function = get_function(name.c_str(), c->args.size());
	
	std::vector<Value*> args(c->args.size());
	std::transform(
		c->args.begin(), c->args.end(),
		args.begin(), vector_codegen(*this)
	);
	
	return builder.CreateCall(function, args.begin(), args.end());
}

Value *node_codegen::visit_statement_error(statement_error*) {
	return 0;
}

Value *node_codegen::visit_assignment(assignment *a) {
	builder.CreateStore(visit(a->rvalue), get_lvalue(a->lvalue));
	return 0;
}

Value *node_codegen::visit_invocation(invocation* i) {
	visit(i->c);
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

AllocaInst *node_codegen::get_lvalue(expression *lvalue) {
	switch (lvalue->type) {
	default: return 0;
	
	case value_node: {
		value *v = static_cast<value*>(lvalue);
		std::string name(v->t.string.data, v->t.string.length);
		return scope[name];
	}
	
	case binary_node: {
		binary *b = static_cast<binary*>(lvalue);
		if (b->op != dot) return 0;
		
		Value *left = visit(b->left);
		return get_lvalue(b->right);
	}
	
	case subscript_node: {
		subscript *s = static_cast<subscript*>(lvalue);
		return get_lvalue(s->array);
	}
	}
}
