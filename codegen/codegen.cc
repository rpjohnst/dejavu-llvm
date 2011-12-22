#include "dejavu/codegen.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include <algorithm>

using namespace llvm;

node_codegen::node_codegen() :
	builder(context), module("", context) {
	
	Type *types[] = {
		Type::getInt32Ty(context),
		StructType::get(Type::getInt32Ty(context), Type::getInt32Ty(context), NULL)
	};
	StructType::create(types, "variant");
}

Module &node_codegen::get_module(node *program) {
	FunctionType *type = FunctionType::get(Type::getVoidTy(context), false);
	Function *function = Function::Create(type, Function::ExternalLinkage, "main", &module);

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
			string, indices, true
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

	return builder.CreateCall2(get_function(name, 2), visit(b->left), visit(b->right));
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
	std::string name(c->function.string.data, c->function.string.length);
	Function *function = get_function(name.c_str(), c->args.size());

	std::vector<Value*> args(c->args.size());
	std::transform(c->args.begin(), c->args.end(), args.begin(), vector_codegen(*this));

	return builder.CreateCall(function, args);
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
	return 0;
}

Value *node_codegen::visit_declaration(declaration *d) {
	for (std::vector<value*>::iterator it = d->names.begin(); it != d->names.end(); ++it) {
		std::string name((*it)->t.string.data, (*it)->t.string.length);
		scope[name] = builder.CreateAlloca(
			module.getTypeByName("variant"), 0, name
		);
	}

	return 0;
}

Value *node_codegen::visit_block(block *b) {
	for (std::vector<statement*>::iterator it = b->stmts.begin(); it != b->stmts.end(); ++it) {
		visit(*it);
	}

	return 0;
}

Value *node_codegen::visit_ifstatement(ifstatement *i) {
	Value *b = visit(i->cond); // todo: get a real out of it
	Value *cond = builder.CreateFCmpOGT(
		b, ConstantFP::get(Type::getDoubleTy(context), 0.5)
	);
	
	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *branch_true = BasicBlock::Create(context, "then");
	BasicBlock *branch_false = BasicBlock::Create(context, "else");
	BasicBlock *merge = BasicBlock::Create(context, "merge");

	builder.CreateCondBr(cond, branch_true, branch_false);

	f->getBasicBlockList().push_back(branch_true);
	builder.SetInsertPoint(branch_true);
	visit(i->branch_true);
	builder.CreateBr(merge);

	f->getBasicBlockList().push_back(branch_false);
	builder.SetInsertPoint(branch_false);
	if (i->branch_false)
		visit(i->branch_false);
	builder.CreateBr(merge);

	f->getBasicBlockList().push_back(merge);
	builder.SetInsertPoint(merge);

	return 0;
}

Value *node_codegen::visit_whilestatement(whilestatement *w) {
	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *loop = BasicBlock::Create(context, "loop");
	BasicBlock *cond = BasicBlock::Create(context, "cond");
	BasicBlock *after = BasicBlock::Create(context, "after");

	builder.CreateBr(cond);

	BasicBlock *save_loop = current_loop, *save_end = current_end;
	current_loop = cond; current_end = after;

	f->getBasicBlockList().push_back(cond);
	builder.SetInsertPoint(cond);
	Value *b = visit(w->cond); // todo: get a real out of it
	Value *c = builder.CreateFCmpOGT(
		b, ConstantFP::get(Type::getDoubleTy(context), 0.5)
	);
	builder.CreateCondBr(c, loop, after);

	f->getBasicBlockList().push_back(loop);
	builder.SetInsertPoint(loop);
	visit(w->stmt);
	builder.CreateBr(cond);

	f->getBasicBlockList().push_back(after);
	builder.SetInsertPoint(after);

	current_loop = save_loop; current_end = save_end;

	return 0;
}

Value *node_codegen::visit_dostatement(dostatement *d) {
	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *loop = BasicBlock::Create(context, "loop");
	BasicBlock *cond = BasicBlock::Create(context, "cond");
	BasicBlock *after = BasicBlock::Create(context, "after");

	builder.CreateBr(loop);

	BasicBlock *save_loop = current_loop, *save_end = current_end;
	current_loop = cond; current_end = after;

	f->getBasicBlockList().push_back(loop);
	builder.SetInsertPoint(loop);
	visit(d->stmt);
	builder.CreateBr(cond);

	f->getBasicBlockList().push_back(cond);
	builder.SetInsertPoint(cond);
	Value *b = visit(d->cond); // todo: get a real out of it
	Value *c = builder.CreateFCmpOLE(
		b, ConstantFP::get(Type::getDoubleTy(context), 0.5)
	);
	builder.CreateCondBr(c, loop, after);

	f->getBasicBlockList().push_back(after);
	builder.SetInsertPoint(after);

	current_loop = save_loop; current_end = save_end;

	return 0;
}

Value *node_codegen::visit_repeatstatement(repeatstatement *r) {
	Value *start = ConstantFP::get(Type::getDoubleTy(context), 0);
	Value *end = visit(r->expr);

	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *loop = BasicBlock::Create(context, "loop");
	BasicBlock *after = BasicBlock::Create(context, "after");

	BasicBlock *init = builder.GetInsertBlock();
	builder.CreateBr(loop);

	BasicBlock *save_loop = current_loop, *save_end = current_end;
	current_loop = loop; current_end = after;

	f->getBasicBlockList().push_back(loop);
	builder.SetInsertPoint(loop);

	// phi node on start
	PHINode *inc = builder.CreatePHI(Type::getDoubleTy(context), 2, "inc");
	inc->addIncoming(start, init);

	visit(r->stmt);

	// phi node on continue
	Value *next = builder.CreateFAdd(
		inc, ConstantFP::get(Type::getDoubleTy(context), 1)
	);
	BasicBlock *last = builder.GetInsertBlock();
	inc->addIncoming(next, last);

	// todo: check with GM's rounding behavior
	Value *done = builder.CreateFCmpOGE(next, end);
	builder.CreateCondBr(done, loop, after);

	f->getBasicBlockList().push_back(after);
	builder.SetInsertPoint(after);

	current_loop = save_loop; current_end = save_end;

	return 0;
}

Value *node_codegen::visit_forstatement(forstatement *f) {
	visit(f->init);

	Function *fn = builder.GetInsertBlock()->getParent();
	BasicBlock *loop = BasicBlock::Create(context, "loop");
	BasicBlock *cond = BasicBlock::Create(context, "cond");
	BasicBlock *inc = BasicBlock::Create(context, "inc");
	BasicBlock *after = BasicBlock::Create(context, "after");

	builder.CreateBr(cond);

	BasicBlock *save_loop = current_loop, *save_end = current_end;
	current_loop = inc; current_end = after;

	fn->getBasicBlockList().push_back(cond);
	builder.SetInsertPoint(cond);
	Value *b = visit(f->cond); // todo: get a real out of it
	Value *c = builder.CreateFCmpOGT(
		b, ConstantFP::get(Type::getDoubleTy(context), 0.5)
	);
	builder.CreateCondBr(c, loop, after);

	fn->getBasicBlockList().push_back(loop);
	builder.SetInsertPoint(loop);
	visit(f->stmt);
	builder.CreateBr(inc);

	fn->getBasicBlockList().push_back(inc);
	builder.SetInsertPoint(inc);
	visit(f->inc);
	builder.CreateBr(cond);

	fn->getBasicBlockList().push_back(after);
	builder.SetInsertPoint(after);

	current_loop = save_loop; current_end = save_end;

	return 0;
}

Value *node_codegen::visit_switchstatement(switchstatement *s) {
	return 0;
}

Value *node_codegen::visit_withstatement(withstatement *w) {
	return 0;
}

Value *node_codegen::visit_jump(jump *j) {
	switch (j->type) {
	default: return 0;

	case kw_exit:
		return builder.CreateCall(get_function("exit_event", 0));

	case kw_break: builder.CreateBr(current_end); break;
	case kw_continue: builder.CreateBr(current_loop); break;
	}

	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *cont = BasicBlock::Create(context, "cont", f);
	builder.SetInsertPoint(cont);

	return 0;
}

Value *node_codegen::visit_returnstatement(returnstatement *r) {
	Value *ret = visit(r->expr);
	builder.CreateRet(ret);

	return 0;
}

Value *node_codegen::visit_casestatement(casestatement *c) {
	return 0;
}

Function *node_codegen::get_function(const char *name, int args) {
	Function *function = module.getFunction(name);
	if (!function) {
		Type *variant = module.getTypeByName("variant");
		std::vector<Type*> vargs(args, variant);
		FunctionType *type = FunctionType::get(variant, vargs, false);

		function = Function::Create(type, Function::ExternalLinkage, name, &module);
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
