#include <dejavu/compiler/codegen.h>
#include <dejavu/system/string.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <tuple>
#include <sstream>

using namespace llvm;

node_codegen::node_codegen(
	LLVMContext &context, const Module &runtime, error_stream &e
) : context(context), runtime(runtime), dl(&runtime),
	builder(context), module("", context), errors(e) {

	real_type = builder.getDoubleTy();
	string_type = runtime.getTypeByName("struct.string")->getPointerTo();
	variant_type = runtime.getTypeByName("struct.variant");
	var_type = runtime.getTypeByName("struct.var");
	scope_type = runtime.getTypeByName("struct.scope")->getPointerTo();

	// todo: create a gml calling convention for the runtime
	to_real = Function::Create(
		runtime.getFunction("to_real")->getFunctionType(),
		Function::ExternalLinkage, "to_real", &module
	);
	to_string = Function::Create(
		runtime.getFunction("to_string")->getFunctionType(),
		Function::ExternalLinkage, "to_string", &module
	);
	intern = Function::Create(
		runtime.getFunction("intern")->getFunctionType(),
		Function::ExternalLinkage, "intern", &module
	);
	access = Function::Create(
		runtime.getFunction("access")->getFunctionType(),
		Function::ExternalLinkage, "access", &module
	);
	retain = Function::Create(
		runtime.getFunction("retain")->getFunctionType(),
		Function::ExternalLinkage, "retain", &module
	);
	release = Function::Create(
		runtime.getFunction("release")->getFunctionType(),
		Function::ExternalLinkage, "release", &module
	);
	retain_var = Function::Create(
		runtime.getFunction("retain_var")->getFunctionType(),
		Function::ExternalLinkage, "retain_var", &module
	);
	release_var = Function::Create(
		runtime.getFunction("release_var")->getFunctionType(),
		Function::ExternalLinkage, "release_var", &module
	);
	insert_globalvar = Function::Create(
		runtime.getFunction("insert_globalvar")->getFunctionType(),
		Function::ExternalLinkage, "insert_globalvar", &module
	);
	lookup_default = Function::Create(
		runtime.getFunction("lookup_default")->getFunctionType(),
		Function::ExternalLinkage, "lookup_default", &module
	);
	lookup = Function::Create(
		runtime.getFunction("lookup")->getFunctionType(),
		Function::ExternalLinkage, "lookup", &module
	);

	// todo: implement these
	/*with_begin = Function::Create(
		runtime.getFunction("with_begin")->getFunctionType(),
		Function::ExternalLinkage, "with_begin", &module
	);
	with_inc = Function::Create(
		runtime.getFunction("with_inc")->getFunctionType(),
		Function::ExternalLinkage, "with_inc", &module
	);*/
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

Function *node_codegen::add_function(
	node *body, const char *name, size_t nargs, bool var
) {
	Function *function = get_function(name, nargs, var);
	if (!function->empty()) {
		errors.error(redefinition_error(name));
		return function;
	}

	// this is not reentrant. it would need to save the state of:
	// return, scopes, insertion point, and symbol table
	Function::arg_iterator ai = function->arg_begin();
	self_scope = ai;
	other_scope = ++ai;

	BasicBlock *entry = BasicBlock::Create(context);

	scope.clear();
	alloca_point = new BitCastInst(
		builder.getInt32(0), builder.getInt32Ty(), "alloca", entry
	);

	return_value = alloc(variant_type);

	function->getBasicBlockList().push_back(entry);
	builder.SetInsertPoint(entry);

	if (var) {
		Value *arg_count = ++ai;
		Value *arg_array = ++ai;

		scope["argument_count"] = make_local("argument_count", get_real(
			builder.CreateUIToFP(arg_count, builder.getDoubleTy())
		));
		scope["argument"] = make_local(
			"argument", arg_count, builder.getInt16(1), arg_array
		);

		// todo: accessors for argument# (also for builtin locals)
	}

	visit(body);

	for (
		std::unordered_map<std::string, Value*>::iterator it = scope.begin();
		it != scope.end(); ++it
	) {
		if (it->first == "argument_count" || it->first == "argument")
			continue;

		builder.CreateCall(release_var, it->second);
	}

	Value *ret = builder.CreateLoad(return_value);
	builder.CreateRet(ret);

	return function;
}

Value *node_codegen::visit_value(value *v) {
	switch (v->t.type) {
	default: return 0;

	case v_name: {
		std::string name(v->t.string.data, v->t.string.length);
		Value *var = scope.find(name) != scope.end() ? scope[name] :
			do_lookup_default(
				builder.CreateCall(
					to_string,
					get_string(StringRef(v->t.string.data, v->t.string.length))
				),
				lvalue
			);
		return builder.CreateCall4(
			access, var,
			builder.getInt16(0), builder.getInt16(0), builder.getInt1(lvalue)
		);
	}

	case v_real: return get_real(v->t.real);
	case v_string:
		return get_string(StringRef(v->t.string.data, v->t.string.length));

	case kw_self: return get_real(-1);
	case kw_other: return get_real(-2);
	case kw_all: return get_real(-3);
	case kw_noone: return get_real(-4);
	case kw_global: return get_real(-5);
	case kw_local: return get_real(-6);

	case kw_true: return get_real(1);
	case kw_false: return get_real(0.0);
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
	builder.CreateMemCpy(operand, visit(u->right), dl.getTypeStoreSize(variant_type), 0);

	CallInst *call = builder.CreateCall(get_operator(name, 1), operand);

	builder.CreateStore(call, result);
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
			builder.CreateCall(
				to_string,
				get_string(StringRef(name.string.data, name.string.length))
			),
			lvalue
		);
		return builder.CreateCall4(
			access, var,
			builder.getInt16(0), builder.getInt16(0), builder.getInt1(lvalue)
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

	case ampamp: name = "log_and"; break;
	case pipepipe: name = "log_or"; break;
	case caretcaret: name = "log_xor"; break;

	case bit_and: name = "bit_and"; break;
	case bit_or: name = "bit_or"; break;
	case bit_xor: name = "bit_xor"; break;
	case shift_left: name = "shift_left"; break;
	case shift_right: name = "shift_right"; break;

	case kw_div: name = "div_"; break; // get around c
	case kw_mod: name = "mod"; break;
	}

	Value *left = alloc(variant_type);
	Value *right = alloc(variant_type);
	Value *result = alloc(variant_type);
	builder.CreateMemCpy(left, visit(b->left), dl.getTypeStoreSize(variant_type), 0);
	builder.CreateMemCpy(right, visit(b->right), dl.getTypeStoreSize(variant_type), 0);

	CallInst *call = builder.CreateCall2(get_operator(name, 2), left, right);

	builder.CreateStore(call, result);
	return result;
}

Value *node_codegen::visit_subscript(subscript *s) {
	Value *var;
	switch (s->array->type) {
	default: return 0;

	case value_node: {
		value *v = static_cast<value*>(s->array);
		std::string name(v->t.string.data, v->t.string.length);
		var = scope.find(name) != scope.end() ? scope[name] : do_lookup_default(
			builder.CreateCall(
				to_string,
				get_string(StringRef(v->t.string.data, v->t.string.length))
			),
			lvalue
		);
		break;
	}

	case binary_node: {
		binary *left = static_cast<binary*>(s->array);
		token &name = static_cast<value*>(left->right)->t;
		var = do_lookup(
			builder.CreateCall(to_real, visit(left->left)),
			builder.CreateCall(
				to_string,
				get_string(StringRef(name.string.data, name.string.length))
			),
			lvalue
		);
		break;
	}
	}

	std::vector<Value*> indices(2, builder.getInt16(0));
	for (size_t i = 0; i < s->indices.size(); i++) {
		Value *index = builder.CreateFPToUI(
			builder.CreateCall(to_real, visit(s->indices[i])),
			builder.getInt16Ty()
		);
		indices[i] = index;
	}

	return builder.CreateCall4(
		access, var, indices[0], indices[1], builder.getInt1(lvalue)
	);
}

Value *node_codegen::visit_call(call *c) {
	StringRef name(c->function->t.string.data, c->function->t.string.length);
	bool var = scripts.find(name) != scripts.end();

	Function *function = get_function(name, var ? 0 : c->args.size(), var);

	std::vector<Value*> args;
	args.reserve(c->args.size() + 4);

	// todo: make & use new GML LLVM cc
	Value *result = alloc(variant_type, name + "_ret");

	args.push_back(self_scope);
	args.push_back(other_scope);
	if (var) args.push_back(builder.getInt16(c->args.size()));

	Value *array = alloc(
		variant_type, builder.getInt32(c->args.size()), name + "_args"
	);
	for (size_t i = 0; i < c->args.size(); i++) {
		Value *indices[] = { builder.getInt32(i) };
		Value *arg = builder.CreateInBoundsGEP(array, indices);
		builder.CreateMemCpy(
			arg, visit(c->args[i]), dl.getTypeStoreSize(variant_type), 0
		);

		if (!var) args.push_back(arg);
	}
	if (var) args.push_back(array);

	CallInst *call = builder.CreateCall(function, args);
	call->addAttribute(1, Attribute::StructRet);

	builder.CreateStore(call, result);
	return result;
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

	Value *l;
	{
		save_context<bool> save(lvalue);
		lvalue = true;
		l = visit(a->lvalue);
	}

	builder.CreateCall(release, l);
	builder.CreateMemCpy(l, r, dl.getTypeStoreSize(variant_type), 0);
	builder.CreateCall(retain, l);
	return 0;
}

Value *node_codegen::visit_invocation(invocation* i) {
	visit(i->c);
	return 0;
}

Value *node_codegen::visit_declaration(declaration *d) {
	for (
		std::vector<value*>::iterator it = d->names.begin();
		it != d->names.end(); ++it
	) {
		std::string name((*it)->t.string.data, (*it)->t.string.length);

		if (name == "argument" || name == "argument_count") {
			errors.error(redefinition_error(name));
			continue;
		}
		if (d->type.type == kw_globalvar) {
			builder.CreateCall(
				insert_globalvar,
				builder.CreateCall(to_string, get_string(name))
			);
			continue;
		}

		if (scope.find(name) != scope.end()) {
			builder.CreateCall(release_var, scope[name]);
		}

		scope[name] = make_local(
			name, Constant::getNullValue(variant_type->getPointerTo())
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
	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *branch_true = BasicBlock::Create(context, "then");
	BasicBlock *branch_false = BasicBlock::Create(context, "else");
	BasicBlock *merge = BasicBlock::Create(context, "merge");

	builder.CreateCondBr(to_bool(i->cond), branch_true, branch_false);

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

	f->getBasicBlockList().push_back(cond);
	builder.SetInsertPoint(cond);
	builder.CreateCondBr(to_bool(w->cond), loop, after);

	f->getBasicBlockList().push_back(loop);
	builder.SetInsertPoint(loop);
	{
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

	case kw_exit: builder.CreateRet(builder.CreateLoad(return_value)); break;
	case kw_break:
		if (current_end) builder.CreateBr(current_end);
		else builder.CreateRet(builder.CreateLoad(return_value));
		break;
	case kw_continue:
		if (current_loop) builder.CreateBr(current_loop);
		else builder.CreateRet(builder.CreateLoad(return_value));
		break;
	}

	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *cont = BasicBlock::Create(context, "cont", f);
	builder.SetInsertPoint(cont);

	return 0;
}

Value *node_codegen::visit_returnstatement(returnstatement *r) {
	builder.CreateRet(builder.CreateLoad(visit(r->expr)));

	Function *f = builder.GetInsertBlock()->getParent();
	BasicBlock *cont = BasicBlock::Create(context, "cont", f);
	builder.SetInsertPoint(cont);

	return 0;
}

Function *node_codegen::get_operator(StringRef name, int args) {
	Function *function = module.getFunction(name);
	if (function) return function;

	std::vector<Type*> vargs(args, variant_type->getPointerTo());
	FunctionType *type = FunctionType::get(variant_type, vargs, false);
	function = Function::Create(type, Function::ExternalLinkage, name, &module);

	return function;
}

Function *node_codegen::get_function(StringRef name, int args, bool var) {
	Function *function = module.getFunction(name);
	if (function) return function;

	std::vector<Type*> vargs(
		args + (var ? 4 : 2), variant_type->getPointerTo()
	);
	vargs[0] = vargs[1] = scope_type;
	if (var) vargs[2] = builder.getInt16Ty();

	FunctionType *type = FunctionType::get(variant_type, vargs, false);
	function = Function::Create(type, Function::ExternalLinkage, name, &module);

	Function::arg_iterator ai = function->arg_begin();
	ai->setName("self");
	ai++; ai->setName("other");
	if (var) {
		ai++; ai->setName("argc");
		ai++; ai->setName("argv");
	}

	return function;
}

Value *node_codegen::get_real(double val) {
	Constant *contents[] = {
		builder.getInt8(0), ConstantFP::get(builder.getDoubleTy(), val),
		UndefValue::get(ArrayType::get(
			builder.getInt8Ty(), std::max(-union_diff, 0)
		))
	};
	Constant *variant = ConstantStruct::getAnon(contents);
	GlobalVariable *global = new GlobalVariable(
		module, variant->getType(), true, GlobalValue::InternalLinkage, variant
	);
	global->setUnnamedAddr(true);

	return builder.CreateBitCast(global, variant_type->getPointerTo());
}

Value *node_codegen::get_real(Value *val) {
	Value *variant = alloc(variant_type, "real");

	Value *tindices[] = { builder.getInt32(0), builder.getInt32(0) };
	Value *type = builder.CreateInBoundsGEP(variant, tindices);
	builder.CreateStore(builder.getInt8(0), type);

	Value *rindices[] = { builder.getInt32(0), builder.getInt32(1) };
	Value *real = builder.CreateBitCast(
		builder.CreateInBoundsGEP(variant, rindices),
		builder.getDoubleTy()->getPointerTo()
	);
	builder.CreateStore(val, real);

	return variant;
}

Value *node_codegen::get_string(StringRef val) {
	GlobalVariable *literal;
	if (string_literals.find(val) != string_literals.end()) {
		literal = string_literals[val];
	}
	else {
		Type *size_type = builder.getIntPtrTy(&dl);

		Constant *contents[] = {
			ConstantInt::get(size_type, 0, false), // refcount
			ConstantPointerNull::get(builder.getInt8PtrTy()), // pool
			ConstantInt::get( // hash
				size_type, string::compute_hash(val.size(), val.data()), false
			),
			ConstantInt::get(size_type, val.size()), // length
			ConstantDataArray::getString(context, val, false) // data
		};
		Constant *s = ConstantStruct::getAnon(contents);
		literal = new GlobalVariable(
			module, s->getType(), false, GlobalValue::PrivateLinkage, s
		);
		string_literals[val] = literal;
	}

	Value *variant = alloc(variant_type, "string");

	Value *tindices[] = { builder.getInt32(0), builder.getInt32(0) };
	Value *type = builder.CreateInBoundsGEP(variant, tindices);
	builder.CreateStore(builder.getInt8(1), type);

	Value *sindices[] = { builder.getInt32(0), builder.getInt32(1) };
	Value *string = builder.CreateBitCast(
		builder.CreateInBoundsGEP(variant, sindices),
		string_type->getPointerTo()
	);
	builder.CreateStore(builder.CreateCall(
		intern, builder.CreateBitCast(literal, string_type)
	), string);

	return variant;
}

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

Value *node_codegen::make_local(StringRef name, Value *value) {
	return make_local(name, builder.getInt16(1), builder.getInt16(1), value);
}

Value *node_codegen::make_local(
	StringRef name, Value *x, Value *y, Value *values
) {
	Value *l = alloc(var_type, name);

	Value *xindices[] = { builder.getInt32(0), builder.getInt32(0) };
	Value *xptr = builder.CreateInBoundsGEP(l, xindices);
	builder.CreateStore(x, xptr);

	Value *yindices[] = { builder.getInt32(0), builder.getInt32(1) };
	Value *yptr = builder.CreateInBoundsGEP(l, yindices);
	builder.CreateStore(y, yptr);

	Value *vindices[] = { builder.getInt32(0), builder.getInt32(2) };
	Value *vptr = builder.CreateInBoundsGEP(l, vindices);
	builder.CreateStore(values, vptr);

	builder.CreateCall(retain_var, l);

	return l;
}

Value *node_codegen::do_lookup(Value *left, Value *right, bool lvalue) {
	return builder.CreateCall5(
		lookup, self_scope, other_scope, left, right, builder.getInt1(lvalue)
	);
}

Value *node_codegen::do_lookup_default(Value *right, bool lvalue) {
	return builder.CreateCall4(
		lookup_default, self_scope, other_scope, right, builder.getInt1(lvalue)
	);
}
