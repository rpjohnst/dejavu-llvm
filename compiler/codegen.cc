#include "dejavu/compiler/codegen.h"
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <tuple>
#include <sstream>

using namespace llvm;

node_codegen::node_codegen(const DataLayout *dl, error_stream &e)
	: builder(context), module("", context), dl(dl), errors(e) {
	real_type = builder.getDoubleTy();

	Type *string[] = { builder.getInt32Ty(), builder.getInt8PtrTy() };
	string_type = StructType::create(string, "struct.string");

	union_diff = dl->getTypeAllocSize(real_type) - dl->getTypeAllocSize(string_type);
	Type *union_type = union_diff > 0 ? real_type : string_type;

	Type *variant[] = { builder.getInt8Ty(), union_type };
	variant_type = StructType::create(variant, "struct.variant");

	Type *dim = Type::getInt16Ty(context), *contents = variant_type->getPointerTo();
	Type *var[] = { dim, dim, contents };
	var_type = StructType::create(var, "struct.var");

	scope_type = StructType::create(context, "scope")->getPointerTo();

	// todo: move to a separate runtime?
	Type *lookup_args[] = { scope_type, scope_type, real_type, string_type };
	Type *lookup_ret = var_type->getPointerTo();
	FunctionType *lookup_type = FunctionType::get(lookup_ret, lookup_args, false);
	lookup = Function::Create(lookup_type, Function::ExternalLinkage, "lookup", &module);

	Type *access_args[] = { var_type->getPointerTo(), real_type, real_type };
	Type *access_ret = variant_type->getPointerTo();
	FunctionType *access_type = FunctionType::get(access_ret, access_args, false);
	access = Function::Create(access_type, Function::ExternalLinkage, "access", &module);

	FunctionType *to_real_ty = FunctionType::get(real_type, variant_type->getPointerTo(), false);
	to_real = Function::Create(to_real_ty, Function::ExternalLinkage, "to_real", &module);

	FunctionType *to_str_ty = FunctionType::get(string_type, variant_type->getPointerTo(), false);
	to_string = Function::Create(to_str_ty, Function::ExternalLinkage, "to_string", &module);

	// todo: should there be a separate with_iterator type?
	Type *with_begin_args[] = { scope_type, scope_type, variant_type->getPointerTo() };
	FunctionType *with_begin_ty = FunctionType::get(scope_type, with_begin_args, false);
	with_begin = Function::Create(with_begin_ty, Function::ExternalLinkage, "with_begin", &module);

	Type *with_inc_args[] = { scope_type, scope_type, variant_type->getPointerTo(), scope_type };
	FunctionType *with_inc_ty = FunctionType::get(scope_type, with_inc_args, false);
	with_inc = Function::Create(with_inc_ty, Function::ExternalLinkage, "with_inc", &module);
}

// fixme: this can only be called one at a time, not nested for e.g. closures
// it would have to save args, return_value, {self,other}_scope, insertion point
Function *node_codegen::add_function(node *body, const char *name, size_t nargs, bool var) {
	Function *function = get_function(name, nargs, var);
	if (!function->empty()) {
		std::ostringstream ss;
		ss << "error: redefinition of function " << name;
		errors.error(ss.str().c_str());
		return function;
	}

	Function::arg_iterator ai = function->arg_begin();
	return_value = ai;
	self_scope = ++ai;
	other_scope = ++ai;
	passed_args = var ? ++ai : 0;

	BasicBlock *entry = BasicBlock::Create(context);

	scope.clear();
	alloca_point = new BitCastInst(builder.getInt32(0), builder.getInt32Ty(), "alloca", entry);

	function->getBasicBlockList().push_back(entry);
	builder.SetInsertPoint(entry);
	visit(body);

	builder.CreateRetVoid();

	return function;
}

Value *node_codegen::visit_value(value *v) {
	switch (v->t.type) {
	default: return 0;

	case name: {
		std::string name(v->t.string.data, v->t.string.length);
		Value *var = scope.find(name) != scope.end() ? scope[name] : do_lookup(
			ConstantFP::get(builder.getDoubleTy(), -1),
			builder.CreateCall(to_string, get_string(v->t.string.length, v->t.string.data))
		);
		Value *indices[] = { builder.getInt32(0), builder.getInt32(2) };
		Value *ptr = builder.CreateInBoundsGEP(var, indices);
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
	case exclaim: name = "not_"; break; // get around c
	case tilde: name = "inv"; break;
	case minus: name = "neg"; break;
	case plus: name = "pos"; break;
	}

	Value *result = alloc(variant_type);
	Value *operand = alloc(variant_type);
	builder.CreateMemCpy(operand, visit(u->right), dl->getTypeStoreSize(variant_type), 0);

	CallInst *call = builder.CreateCall2(get_operator(name, 1), result, operand);
	call->addAttribute(1, Attribute::StructRet);
	call->addAttribute(2, Attribute::ByVal);

	return result;
}

Value *node_codegen::visit_binary(binary *b) {
	const char *name;
	switch (b->op) {
	default: return 0;

	case dot: {
		token &name = static_cast<value*>(b->right)->t;
		Value *var = do_lookup(
			builder.CreateCall(to_real, visit(b->left)),
			builder.CreateCall(to_string, get_string(name.string.length, name.string.data))
		);
		Value *indices[] = { builder.getInt32(0), builder.getInt32(2) };
		Value *ptr = builder.CreateInBoundsGEP(var, indices);
		return builder.CreateLoad(ptr);
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

	case ampamp: name = "log_and"; break;
	case pipepipe: name = "log_or"; break;
	case caretcaret: name = "log_xor"; break;

	case bit_and: name = "bit_and"; break;
	case bit_or: name = "bit_or"; break;
	case bit_xor: name = "bit_xor"; break;
	case shift_left: name = "shift_left"; break;
	case shift_right: name = "shift_right"; break;

	case kw_div: name = "div"; break;
	case kw_mod: name = "mod"; break;
	}

	Value *left = alloc(variant_type);
	Value *right = alloc(variant_type);
	Value *result = alloc(variant_type);
	builder.CreateMemCpy(left, visit(b->left), dl->getTypeStoreSize(variant_type), 0);
	builder.CreateMemCpy(right, visit(b->right), dl->getTypeStoreSize(variant_type), 0);

	CallInst *call = builder.CreateCall3(get_operator(name, 2), result, left, right);
	call->addAttribute(1, Attribute::StructRet);
	call->addAttribute(2, Attribute::ByVal);
	call->addAttribute(3, Attribute::ByVal);

	return result;
}

// todo: {bounds,type,syntax} checking
Value *node_codegen::visit_subscript(subscript *s) {
	// todo: should this be in a get_lvalue function?
	Value *var;
	switch (s->array->type) {
	default: return 0;

	case value_node: {
		value *v = static_cast<value*>(s->array);
		std::string name(v->t.string.data, v->t.string.length);
		var = scope.find(name) != scope.end() ? scope[name] : do_lookup(
			ConstantFP::get(builder.getDoubleTy(), -1),
			builder.CreateCall(to_string, get_string(v->t.string.length, v->t.string.data))
		);
		break;
	}

	case binary_node: {
		binary *left = static_cast<binary*>(s->array);
		token &name = static_cast<value*>(left->right)->t;
		var = do_lookup(
			builder.CreateCall(to_real, visit(left->left)),
			builder.CreateCall(to_string, get_string(name.string.length, name.string.data))
		);
		break;
	}
	}

	Value *indices[2];
	size_t i = 2 - s->indices.size();
	for (size_t j = 0; j < i; j++) {
		indices[j] = ConstantFP::get(builder.getDoubleTy(), 0);
	}
	for (
		std::vector<expression*>::iterator it = s->indices.begin();
		it != s->indices.end();
		++it, ++i
	) {
		Value *index = builder.CreateCall(to_real, visit(*it));
		indices[i] = index;
	}

	return builder.CreateCall3(access, var, indices[0], indices[1]);
}

Value *node_codegen::visit_call(call *c) {
	std::string name(c->function->t.string.data, c->function->t.string.length);
	bool var = scripts.find(name) != scripts.end();

	Function *function = get_function(
		name.c_str(), var ? 0 : c->args.size(), var
	);

	std::vector<Value*> args;
	args.reserve(c->args.size() + 4);

	args.push_back(alloc(variant_type));
	args.push_back(self_scope);
	args.push_back(other_scope);
	if (var) {
		args.push_back(
			ConstantInt::get(IntegerType::get(context, 8), c->args.size())
		);
	}

	for (
		std::vector<expression*>::iterator it = c->args.begin();
		it != c->args.end(); ++it
	) {
		Value *arg = alloc(variant_type);
		builder.CreateMemCpy(arg, visit(*it), dl->getTypeStoreSize(variant_type), 0);
		args.push_back(arg);
	}

	CallInst *call = builder.CreateCall(function, args);
	call->addAttribute(1, Attribute::StructRet);

	return args[0];
}

Value *node_codegen::visit_assignment(assignment *a) {
	Value *r;
	if (a->op == equals) {
		r = visit(a->rvalue);
	}
	else {
		token_type op = unexpected;
		switch (a->op) {
		case plus_equals: op = plus; break;
		case minus_equals: op = minus; break;
		case times_equals: op = times; break;
		case div_equals: op = divide; break;
		case and_equals: op = bit_and; break;
		case or_equals: op = bit_or; break;
		case xor_equals: op = bit_xor; break;
		default: /* should've already errored */ break;
		}
		binary b(op, a->lvalue, a->rvalue);
		r = visit_binary(&b);
	}

	builder.CreateMemCpy(
		visit(a->lvalue), r, dl->getTypeStoreSize(variant_type), 0
	);
	return 0;
}

Value *node_codegen::visit_invocation(invocation* i) {
	visit(i->c);
	return 0;
}

Value *node_codegen::visit_declaration(declaration *d) {
	for (std::vector<value*>::iterator it = d->names.begin(); it != d->names.end(); ++it) {
		std::string name((*it)->t.string.data, (*it)->t.string.length);
		scope[name] = alloc(var_type);
	
		Value *x = builder.getInt16(1), *xindices[] = { builder.getInt32(0), builder.getInt32(0) };
		Value *xptr = builder.CreateInBoundsGEP(scope[name], xindices);
		builder.CreateStore(x, xptr);

		Value *y = builder.getInt16(1), *yindices[] = { builder.getInt32(0), builder.getInt32(1) };
		Value *yptr = builder.CreateInBoundsGEP(scope[name], yindices);
		builder.CreateStore(y, yptr);

		Value *variant = alloc(variant_type, name);
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

// todo: can this be refactored to reuse the forstatement codegen?
Value *node_codegen::visit_withstatement(withstatement *w) {
	Function *fn = builder.GetInsertBlock()->getParent();
	BasicBlock *loop = BasicBlock::Create(context, "loop");
	BasicBlock *cond = BasicBlock::Create(context, "cond");
	BasicBlock *inc = BasicBlock::Create(context, "inc");
	BasicBlock *after = BasicBlock::Create(context, "after");

	Value *with_expr = visit(w->expr);
	Value *instance = alloc(scope_type);
	Value *init = builder.CreateCall3(with_begin, self_scope, other_scope, with_expr);
	builder.CreateStore(init, instance);
	builder.CreateBr(cond);

	fn->getBasicBlockList().push_back(cond);
	builder.SetInsertPoint(cond);
	Value *with_end = ConstantPointerNull::get(scope_type);
	Value *with_cond = builder.CreateICmpNE(builder.CreateLoad(instance), with_end);
	builder.CreateCondBr(with_cond, loop, after);

	fn->getBasicBlockList().push_back(loop);
	builder.SetInsertPoint(loop);
	{
		save_context<BasicBlock*, BasicBlock*, Value*, Value*> save(
			current_loop, current_end, self_scope, other_scope
		);
		current_loop = inc;
		current_end = after;
		other_scope = self_scope;
		self_scope = builder.CreateLoad(instance);
		visit(w->stmt);
	}
	builder.CreateBr(inc);

	fn->getBasicBlockList().push_back(inc);
	builder.SetInsertPoint(inc);
	Value *with_next = builder.CreateCall4(
		with_inc, self_scope, other_scope, with_expr, builder.CreateLoad(instance)
	);
	builder.CreateStore(with_next, instance);
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
	Function::iterator switch_cond = builder.GetInsertBlock();

	fn->getBasicBlockList().push_back(dead);
	builder.SetInsertPoint(dead);
	{
		save_context<Value*, BasicBlock*, Function::iterator, BasicBlock*> save(
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
	builder.CreateMemCpy(return_value, visit(r->expr), dl->getTypeStoreSize(variant_type), 0);
	builder.CreateRetVoid();

	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *cont = BasicBlock::Create(context, "cont", f);
	builder.SetInsertPoint(cont);

	return 0;
}

Function *node_codegen::get_operator(const char *name, int args) {
	Function *function = module.getFunction(name);
	if (function) return function;

	std::vector<Type*> vargs(args + 1, variant_type->getPointerTo());
	FunctionType *type = FunctionType::get(builder.getVoidTy(), vargs, false);
	function = Function::Create(type, Function::ExternalLinkage, name, &module);

	Function::arg_iterator ai = function->arg_begin();
	Attribute::AttrKind attrs[] = { Attribute::NoAlias, Attribute::StructRet };
	ai->addAttr(AttributeSet::get(context, ai->getArgNo() + 1, attrs));
	for (++ai; ai != function->arg_end(); ++ai) {
		Attribute::AttrKind attr[] = { Attribute::ByVal };
		ai->addAttr(AttributeSet::get(context, ai->getArgNo() + 1, attr));
	}

	return function;
}

Function *node_codegen::get_function(const char *name, int args, bool var) {
	Function *function = module.getFunction(name);
	if (function) return function;

	std::vector<Type*> vargs(args + (var ? 4 : 3), variant_type->getPointerTo());
	vargs[1] = vargs[2] = scope_type;
	if (var) vargs[3] = IntegerType::get(context, 8);
	FunctionType *type = FunctionType::get(builder.getVoidTy(), vargs, var);
	function = Function::Create(type, Function::ExternalLinkage, name, &module);

	Function::arg_iterator ai = function->arg_begin();
	Attribute::AttrKind attrs[] = { Attribute::NoAlias, Attribute::StructRet };
	ai->addAttr(AttributeSet::get(context, ai->getArgNo() + 1, attrs));

	// TODO: argument#, argument[#], argument_relative

	return function;
}

Value *node_codegen::get_real(double val) {
	Constant *contents[] = {
		builder.getInt8(0), ConstantFP::get(builder.getDoubleTy(), val),
		UndefValue::get(ArrayType::get(builder.getInt8Ty(), union_diff < 0 ? -union_diff : 0))
	};
	Constant *variant = ConstantStruct::getAnon(contents);
	GlobalVariable *global = new GlobalVariable(
		module, variant->getType(), true, GlobalValue::InternalLinkage, variant, "real"
	);
	global->setUnnamedAddr(true);

	return builder.CreateBitCast(global, variant_type->getPointerTo());
}

Value *node_codegen::get_string(int length, const char *data) {
	Constant *array = ConstantDataArray::getString(
		context, StringRef(data, length), false
	);
	GlobalVariable *str = new GlobalVariable(
		module, array->getType(), true, GlobalValue::InternalLinkage, array, ".str"
	);
	str->setUnnamedAddr(true);

	Value *indices[] = { builder.getInt32(0), builder.getInt32(0) };
	Constant *string[] = {
		builder.getInt32(length), cast<Constant>(builder.CreateInBoundsGEP(str, indices))
	};

	Constant *contents[] = {
		builder.getInt8(1), ConstantStruct::get(string_type, ArrayRef<Constant*>(string)),
		UndefValue::get(ArrayType::get(builder.getInt8Ty(), union_diff > 0 ? union_diff : 0))
	};
	Constant *variant = ConstantStruct::getAnon(contents);
	GlobalVariable *global = new GlobalVariable(
		module, variant->getType(), true, GlobalValue::InternalLinkage, variant, "string"
	);
	global->setUnnamedAddr(true);

	return builder.CreateBitCast(global, variant_type->getPointerTo());
}

// todo: combine these next two

Value *node_codegen::to_bool(node *cond) {
	Value *expr = builder.CreateCall(to_real, visit(cond));
	return builder.CreateFCmpUGT(expr, ConstantFP::get(builder.getDoubleTy(), 0.5));
}

Value *node_codegen::is_equal(Value *a, Value *b) {
	Value *res = alloc(variant_type);
	builder.CreateCall3(get_operator("is_equals", 2), res, a, b);
	Value *expr = builder.CreateCall(to_real, res);
	return builder.CreateFCmpUGT(expr, ConstantFP::get(builder.getDoubleTy(), 0.5));
}

AllocaInst *node_codegen::alloc(Type *type, const Twine &name) {
	return new AllocaInst(type, 0, name, alloca_point);
}

Value *node_codegen::do_lookup(Value *left, Value *right) {
	return builder.CreateCall4(lookup, self_scope, other_scope, left, right);
}
