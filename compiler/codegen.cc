#include "dejavu/compiler/codegen.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Target/TargetData.h"
#include <tuple>

using namespace llvm;

node_codegen::node_codegen(const TargetData *td) : builder(context), module("", context), td(td) {
	real_type = builder.getDoubleTy();

	Type *string[] = { builder.getInt32Ty(), builder.getInt8PtrTy() };
	string_type = StructType::create(string, "string");

	Type *union_type = (
		td->getTypeAllocSize(real_type) > td->getTypeAllocSize(string_type)
	) ? real_type : string_type;

	Type *variant[] = { builder.getInt8Ty(), union_type };
	variant_type = StructType::create(variant, "variant");

	Type *dim = Type::getInt16Ty(context), *contents = variant_type->getPointerTo();
	Type *var[] = { dim, dim, contents };
	var_type = StructType::create(var, "var");

	// todo: move to a separate runtime?
	Type *lookup_args[] = { real_type, string_type };
	Type *lookup_ret = variant_type->getPointerTo();
	FunctionType *lookup_type = FunctionType::get(lookup_ret, lookup_args, false);
	lookup = Function::Create(lookup_type, Function::ExternalLinkage, "lookup", &module);

	FunctionType *to_real_type = FunctionType::get(
		real_type, variant_type->getPointerTo(), false
	);
	to_real = Function::Create(to_real_type, Function::ExternalLinkage, "to_real", &module);

	FunctionType *to_string_type = FunctionType::get(
		string_type, variant_type->getPointerTo(), false
	);
	to_string = Function::Create(to_string_type, Function::ExternalLinkage, "to_string", &module);
}

Module &node_codegen::get_module(node *program) {
	// todo: refactor this into a function code generator
	Type *args[] = { variant_type->getPointerTo() };
	FunctionType *type = FunctionType::get(builder.getVoidTy(), args, false);
	Function *function = Function::Create(type, Function::ExternalLinkage, "main", &module);

	Function::arg_iterator ai = function->arg_begin();
	ai->addAttr(Attribute::NoAlias | Attribute::StructRet);
	return_value = ai;
	for (++ai; ai != function->arg_end(); ++ai) {
		ai->addAttr(Attribute::ByVal);
	}

	BasicBlock *entry = BasicBlock::Create(context, "entry", function);
	builder.SetInsertPoint(entry);

	visit(program);

	builder.CreateRetVoid();

	return module;
}

Value *node_codegen::visit_value(value *v) {
	switch (v->t.type) {
	default: return 0;

	case name: {
		std::string name(v->t.string.data, v->t.string.length);
		Value *indices[] = { builder.getInt32(0), builder.getInt32(2) };
		Value *ptr = builder.CreateInBoundsGEP(scope[name], indices);
		return builder.CreateLoad(ptr);
	}

	case real: return get_real(v->t.real);
	case string: return get_string(v->t.string.length, v->t.string.data);

	case kw_self: return get_real(-1);
	case kw_other: return get_real(-2);
	case kw_all: return get_real(-3);
	case kw_noone: return get_real(-4);
	case kw_global: return get_real(-5);
	case kw_local: return get_real(-6);

	case kw_true: return get_real(1);
	case kw_false: return get_real(0);
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

	Value *operand = visit(u->right);
	Value *result = builder.CreateAlloca(variant_type, 0);
	builder.CreateCall2(get_function(name, 1), result, operand);
	return result;
}

Value *node_codegen::visit_binary(binary *b) {
	const char *name;
	switch (b->op) {
	default: return 0;

	case dot: {
		token &name = static_cast<value*>(b->right)->t;
		return builder.CreateCall2(
			lookup,
			builder.CreateCall(to_real, visit(b->left)),
			builder.CreateCall(to_string, get_string(name.string.length, name.string.data))
		);
	}

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

	Value *left = visit(b->left);
	Value *right = visit(b->right);
	Value *result = builder.CreateAlloca(variant_type);
	builder.CreateCall3(get_function(name, 2), result, left, right);
	return result;
}

// todo: {bounds,type,syntax} checking
Value *node_codegen::visit_subscript(subscript *s) {
	std::string name(s->array.string.data, s->array.string.length);

	Value *xindices[] = { builder.getInt32(0), builder.getInt32(0) };
	Value *x16 = builder.CreateLoad(builder.CreateInBoundsGEP(scope[name], xindices));
	Value *x = builder.CreateZExt(x16, builder.getInt32Ty());

	// Value *yindices[] = { builder.getInt32(0), builder.getInt32(1) };
	// Value *y16 = builder.CreateLoad(builder.CreateInBoundsGEP(scope[name], yindices));
	// Value *y = builder.CreateZExt(y16, builder.getInt32Ty());

	Value *aptr[] = { builder.getInt32(0), builder.getInt32(2) };
	Value *variants = builder.CreateLoad(builder.CreateInBoundsGEP(scope[name], aptr));

	std::vector<Value*> is;
	is.reserve(s->indices.size());
	for (std::vector<expression*>::iterator it = s->indices.begin(); it != s->indices.end(); ++it) {
		is.push_back(visit(*it));
	}

	switch (is.size()) {
	default: return 0;
	case 0: {
		return builder.CreateInBoundsGEP(variants, builder.getInt32(0));
	}
	case 1: {
		Value *real = builder.CreateCall(to_real, is[0]);
		Value *index = builder.CreateFPToUI(real, builder.getInt32Ty());
		return builder.CreateInBoundsGEP(variants, index);
	}
	case 2: {
		Value *ireal = builder.CreateCall(to_real, is[0]);
		Value *i = builder.CreateFPToUI(ireal, builder.getInt32Ty());

		Value *jreal = builder.CreateCall(to_real, is[1]);
		Value *j = builder.CreateFPToUI(jreal, builder.getInt32Ty());

		Value *index = builder.CreateNSWAdd(builder.CreateNSWMul(i, x), j);
		return builder.CreateInBoundsGEP(variants, index);
	}
	}
}

Value *node_codegen::visit_call(call *c) {
	std::string name(c->function.string.data, c->function.string.length);
	Function *function = get_function(name.c_str(), c->args.size());

	std::vector<Value*> args;
	args.reserve(c->args.size() + 1);

	args.push_back(builder.CreateAlloca(variant_type, 0));
	for (std::vector<expression*>::iterator it = c->args.begin(); it != c->args.end(); ++it) {
		args.push_back(visit(*it));
	}

	builder.CreateCall(function, args);
	return args[0];
}

// todo: += -= etc.
Value *node_codegen::visit_assignment(assignment *a) {
	builder.CreateMemCpy(visit(a->lvalue), visit(a->rvalue), td->getTypeStoreSize(variant_type), 0);
	return 0;
}

Value *node_codegen::visit_invocation(invocation* i) {
	visit(i->c);
	return 0;
}

Value *node_codegen::visit_declaration(declaration *d) {
	for (std::vector<value*>::iterator it = d->names.begin(); it != d->names.end(); ++it) {
		std::string name((*it)->t.string.data, (*it)->t.string.length);
		scope[name] = builder.CreateAlloca(var_type, 0);
	
		Value *x = builder.getInt16(1), *xindices[] = { builder.getInt32(0), builder.getInt32(0) };
		Value *xptr = builder.CreateInBoundsGEP(scope[name], xindices);
		builder.CreateStore(x, xptr);

		Value *y = builder.getInt16(1), *yindices[] = { builder.getInt32(0), builder.getInt32(1) };
		Value *yptr = builder.CreateInBoundsGEP(scope[name], yindices);
		builder.CreateStore(y, yptr);

		Value *variant = builder.CreateAlloca(variant_type, 0, name);
		Value *vindices[] = { builder.getInt32(0), builder.getInt32(2) };
		Value *vptr = builder.CreateInBoundsGEP(scope[name], vindices);
		builder.CreateStore(variant, vptr);
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
	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *branch_true = BasicBlock::Create(context, "then");
	BasicBlock *branch_false = BasicBlock::Create(context, "else");
	BasicBlock *merge = BasicBlock::Create(context, "merge");

	Value *expr = builder.CreateCall(to_real, visit(i->cond));
	Value *cond = builder.CreateFCmpUGT(expr, ConstantFP::get(builder.getDoubleTy(), 0.5));
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

namespace {
	template <typename... types>
	class save_context {
	public:
		save_context(types&... args) : context(args...), data(args...) {}
		~save_context() { context = data; }

	private:
		std::tuple<types&...> context;
		std::tuple<types...> data;
	};
}

Value *node_codegen::visit_whilestatement(whilestatement *w) {
	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *loop = BasicBlock::Create(context, "loop");
	BasicBlock *cond = BasicBlock::Create(context, "cond");
	BasicBlock *after = BasicBlock::Create(context, "after");

	builder.CreateBr(cond);

	f->getBasicBlockList().push_back(cond);
	builder.SetInsertPoint(cond);
	builder.CreateCondBr(to_bool(w->cond), loop, after);

	f->getBasicBlockList().push_back(loop);
	builder.SetInsertPoint(loop);
	{
		// this is ugly, wish I could pass args to visit...
		save_context<BasicBlock*, BasicBlock*> save(current_loop, current_end);
		current_loop = cond;
		current_end = after;
		visit(w->stmt);
	}
	builder.CreateBr(cond);

	f->getBasicBlockList().push_back(after);
	builder.SetInsertPoint(after);

	return 0;
}

Value *node_codegen::visit_dostatement(dostatement *d) {
	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *loop = BasicBlock::Create(context, "loop");
	BasicBlock *cond = BasicBlock::Create(context, "cond");
	BasicBlock *after = BasicBlock::Create(context, "after");

	builder.CreateBr(loop);

	f->getBasicBlockList().push_back(loop);
	builder.SetInsertPoint(loop);
	{
		save_context<BasicBlock*, BasicBlock*> save(current_loop, current_end);
		current_loop = cond;
		current_end = after;
		visit(d->stmt);
	}
	builder.CreateBr(cond);

	f->getBasicBlockList().push_back(cond);
	builder.SetInsertPoint(cond);
	builder.CreateCondBr(to_bool(d->cond), after, loop);

	f->getBasicBlockList().push_back(after);
	builder.SetInsertPoint(after);

	return 0;
}

Value *node_codegen::visit_repeatstatement(repeatstatement *r) {
	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *loop = BasicBlock::Create(context, "loop");
	BasicBlock *after = BasicBlock::Create(context, "after");
	BasicBlock *init = builder.GetInsertBlock();

	Value *start = ConstantFP::get(builder.getDoubleTy(), 0);
	Value *end = builder.CreateCall(to_real, visit(r->expr));
	builder.CreateBr(loop);

	f->getBasicBlockList().push_back(loop);
	builder.SetInsertPoint(loop);
	PHINode *inc = builder.CreatePHI(builder.getDoubleTy(), 2, "inc");
	{
		inc->addIncoming(start, init);

		save_context<BasicBlock*, BasicBlock*> save(current_loop, current_end);
		current_loop = loop;
		current_end = after;
		visit(r->stmt);

		Value *next = builder.CreateFAdd(inc, ConstantFP::get(builder.getDoubleTy(), 1));
		BasicBlock *last = builder.GetInsertBlock();
		inc->addIncoming(next, last);
	}
	// todo: check with GM's rounding behavior
	Value *done = builder.CreateFCmpULT(inc, end);
	builder.CreateCondBr(done, loop, after);

	f->getBasicBlockList().push_back(after);
	builder.SetInsertPoint(after);

	return 0;
}

Value *node_codegen::visit_forstatement(forstatement *f) {
	Function *fn = builder.GetInsertBlock()->getParent();
	BasicBlock *loop = BasicBlock::Create(context, "loop");
	BasicBlock *cond = BasicBlock::Create(context, "cond");
	BasicBlock *inc = BasicBlock::Create(context, "inc");
	BasicBlock *after = BasicBlock::Create(context, "after");

	visit(f->init);
	builder.CreateBr(cond);

	fn->getBasicBlockList().push_back(cond);
	builder.SetInsertPoint(cond);
	builder.CreateCondBr(to_bool(f->cond), loop, after);

	fn->getBasicBlockList().push_back(loop);
	builder.SetInsertPoint(loop);
	{
		save_context<BasicBlock*, BasicBlock*> save(current_loop, current_end);
		current_loop = inc;
		current_end = after;
		visit(f->stmt);
	}
	builder.CreateBr(inc);

	fn->getBasicBlockList().push_back(inc);
	builder.SetInsertPoint(inc);
	visit(f->inc);
	builder.CreateBr(cond);

	fn->getBasicBlockList().push_back(after);
	builder.SetInsertPoint(after);

	return 0;
}

// todo: switch on a hash rather than generating an if/else chain
Value *node_codegen::visit_switchstatement(switchstatement *s) {
	Function *fn = builder.GetInsertBlock()->getParent();
	BasicBlock *switch_default = BasicBlock::Create(context, "default");
	BasicBlock *dead = BasicBlock::Create(context, "dead");
	BasicBlock *after = BasicBlock::Create(context, "after");

	Value *switch_expr = visit(s->expr);
	Function::BasicBlockListType::iterator switch_cond = builder.GetInsertBlock();

	fn->getBasicBlockList().push_back(dead);
	builder.SetInsertPoint(dead);
	{
		save_context<Value*, BasicBlock*, Function::BasicBlockListType::iterator, BasicBlock*> save(
			current_switch, current_default, current_cond, current_end
		);
		current_switch = switch_expr;
		current_default = switch_default;
		current_cond = switch_cond;
		current_end = after;
		visit(s->stmts);

		builder.SetInsertPoint(current_cond);
		builder.CreateBr(current_default);

		builder.SetInsertPoint(&fn->getBasicBlockList().back());
		if (!current_default->getParent()) {
			builder.CreateBr(current_default);

			fn->getBasicBlockList().push_back(current_default);
			builder.SetInsertPoint(current_default);
		}
	}
	builder.CreateBr(after);

	fn->getBasicBlockList().push_back(after);
	builder.SetInsertPoint(after);

	return 0;
}

Value *node_codegen::visit_casestatement(casestatement *c) {
	Function *fn = builder.GetInsertBlock()->getParent();

	if (!c->expr) {
		builder.CreateBr(current_default);
		fn->getBasicBlockList().push_back(current_default);
		builder.SetInsertPoint(current_default);

		return 0;
	}

	BasicBlock *switch_case = BasicBlock::Create(context, "case");
	BasicBlock *next_cond = BasicBlock::Create(context, "next");

	builder.SetInsertPoint(current_cond);

	Value *case_expr = visit(c->expr);
	Value *cond = is_equal(current_switch, case_expr);
	builder.CreateCondBr(cond, switch_case, next_cond);

	fn->getBasicBlockList().insertAfter(current_cond, next_cond);
	current_cond = next_cond;

	builder.SetInsertPoint(&fn->getBasicBlockList().back());
	builder.CreateBr(switch_case);

	fn->getBasicBlockList().push_back(switch_case);
	builder.SetInsertPoint(switch_case);

	return 0;
}

Value *node_codegen::visit_withstatement(withstatement *w) {
	return 0;
}

Value *node_codegen::visit_jump(jump *j) {
	switch (j->type) {
	default: return 0;

	case kw_exit: builder.CreateRetVoid(); break;
	case kw_break:
		if (current_end) builder.CreateBr(current_end);
		else builder.CreateRetVoid();
		break;
	case kw_continue:
		if (current_loop) builder.CreateBr(current_loop);
		else builder.CreateRetVoid();
		break;
	}

	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *cont = BasicBlock::Create(context, "cont", f);
	builder.SetInsertPoint(cont);

	return 0;
}

Value *node_codegen::visit_returnstatement(returnstatement *r) {
	builder.CreateMemCpy(visit(r->expr), return_value, td->getTypeStoreSize(variant_type), 0);
	builder.CreateRetVoid();

	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *cont = BasicBlock::Create(context, "cont", f);
	builder.SetInsertPoint(cont);

	return 0;
}

Function *node_codegen::get_function(const char *name, int args) {
	Function *function = module.getFunction(name);
	if (!function) {
		std::vector<Type*> vargs(args + 1, variant_type->getPointerTo());
		FunctionType *type = FunctionType::get(builder.getVoidTy(), vargs, false);
		function = Function::Create(type, Function::ExternalLinkage, name, &module);

		Function::arg_iterator ai = function->arg_begin();
		ai->addAttr(Attribute::NoAlias | Attribute::StructRet);
		for (++ai; ai != function->arg_end(); ++ai) {
			ai->addAttr(Attribute::ByVal);
		}
	}
	return function;
}

Value *node_codegen::get_real(double val) {
	Constant *contents[] = { builder.getInt1(0), ConstantFP::get(builder.getDoubleTy(), val) };
	Constant *variant = ConstantStruct::getAnon(contents);
	GlobalVariable *global = new GlobalVariable(
		module, variant->getType(), true, GlobalValue::InternalLinkage, variant, "real"
	);
	return builder.CreateBitCast(global, variant_type->getPointerTo());
}

Value *node_codegen::get_string(int length, const char *data) {
	Constant *array = ConstantArray::get(context, StringRef(data, length), false);
	GlobalVariable *str = new GlobalVariable(
		module, array->getType(), true, GlobalValue::PrivateLinkage, array, ".str"
	);

	Value *indices[] = { builder.getInt32(0), builder.getInt32(0) };
	Constant *string[] = {
		builder.getInt32(length), cast<Constant>(builder.CreateInBoundsGEP(str, indices))
	};
	Constant *val = ConstantStruct::get(string_type, ArrayRef<Constant*>(string));

	Constant *contents[] = { builder.getInt1(1), val };
	Constant *variant = ConstantStruct::getAnon(contents);
	GlobalVariable *global = new GlobalVariable(
		module, variant->getType(), true, GlobalValue::InternalLinkage, variant, "string"
	);
	return builder.CreateBitCast(global, variant_type->getPointerTo());
}

// todo: combine these next two

Value *node_codegen::to_bool(node *cond) {
	Value *expr = builder.CreateCall(to_real, visit(cond));
	return builder.CreateFCmpUGT(expr, ConstantFP::get(builder.getDoubleTy(), 0.5));
}

Value *node_codegen::is_equal(Value *a, Value *b) {
	Value *res = builder.CreateAlloca(variant_type);
	builder.CreateCall3(get_function("is_equal", 2), res, a, b);
	Value *expr = builder.CreateCall(to_real, res);
	return builder.CreateFCmpUGT(expr, ConstantFP::get(builder.getDoubleTy(), 0.5));
}
