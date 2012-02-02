#include "dejavu/linker/linker.h"
#include "dejavu/linker/game.h"

#include "dejavu/compiler/lexer.h"
#include "dejavu/compiler/parser.h"
#include "dejavu/compiler/codegen.h"
#include "dejavu/system/buffer.h"

#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Linker.h"
#include "llvm/Support/Path.h"
#include "llvm/Bitcode/ReaderWriter.h"

#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetData.h"

#include "llvm/Support/ToolOutputFile.h"

#include <sstream>

using namespace llvm;

const TargetData *get_target(const std::string &triple) {
	std::string error;
	const Target *target = TargetRegistry::lookupTarget(triple, error);
	TargetMachine *machine = target->createTargetMachine(triple, "", "");
	return machine->getTargetData();
}

linker::linker(game &g, const std::string &triple, error_stream &e) :
	source(g), errors(e), td(get_target(triple)), compiler(td) {}

bool linker::build(const char *target) {
	errors.progress(20, "compiling libraries");
	build_libraries();

	errors.progress(30, "compiling scripts");
	build_scripts();

	errors.progress(40, "compiling objects");
	build_objects();

	if (errors.count() > 0) return false;

	errors.progress(60, "linking runtime");
	Module &game = compiler.get_module();

	Linker linker("", "", game.getContext());
	linker.LinkInModule(&game);
	bool is_native;
	linker.LinkInFile(sys::Path("runtime.bc"), is_native);
	Module *module = linker.getModule();

	errors.progress(80, "optimizing game");
	PassManager pm;
	pm.add(new TargetData(*td));

	PassManagerBuilder pmb;
	pmb.OptLevel = 3;
	pmb.Inliner = createFunctionInliningPass(275);
	pmb.populateModulePassManager(pm);
	pmb.populateLTOPassManager(pm, true, true);

	pm.add(createVerifierPass());

	std::string error_info;
	std::unique_ptr<tool_output_file> out(
		new tool_output_file(target, error_info, raw_fd_ostream::F_Binary)
	);
	if (!error_info.empty()) {
		errors.error(error_info);
		return false;
	}

	pm.add(createBitcodeWriterPass(out->os()));
	pm.run(*module);
	out->keep();

	return errors.count() == 0;
}

void linker::build_libraries() {
	for (unsigned int i = 0; i < source.nactions; i++) {
		if (source.actions[i].exec != action_type::exec_code)
			continue;

		std::ostringstream name;
		name << "action_lib";
		if (source.actions[i].parent > -1) name << source.actions[i].parent;
		name << "_" << source.actions[i].id;

		add_function(
			strlen(source.actions[i].code), source.actions[i].code,
			name.str().c_str(), 16 + source.actions[i].relative
		);
	}
}

void linker::build_scripts() {
	for (unsigned int i = 0; i < source.nscripts; i++) {
		add_function(
			strlen(source.scripts[i].code), source.scripts[i].code, source.scripts[i].name, 16
		);
	}
}

std::ostream &operator <<(std::ostream &out, const argument &arg);
void linker::build_objects() {
	for (unsigned int i = 0; i < source.nobjects; i++) {
		object &obj = source.objects[i];
		// todo: output object properties

		for (unsigned int e = 0; e < obj.nevents; e++) {
			event &evt = obj.events[e];
			// todo: output event data

			// todo: move this into compiler
			std::ostringstream code;
			for (unsigned int a = 0; a < evt.nactions; a++) {
				action &act = evt.actions[a];

				// todo: don't do this by unparsing and reparsing
				switch (act.type->kind) {
				case action_type::act_begin: code << "{\n"; break;
				case action_type::act_end: code << "}\n"; break;
				case action_type::act_else: code << "else\n"; break;
				case action_type::act_exit: code << "exit\n"; break;

				case action_type::act_repeat:
					code << "repeat (" << act.args[0].val << ")\n";
					break;
				case action_type::act_variable:
					code
						<< act.args[0].val
						<< (act.relative ? " += " : " = ")
						<< act.args[1].val << "\n";
					break;

				case action_type::act_code: {
					std::ostringstream s;
					s << obj.name << "_" << evt.main_id << "_" << evt.sub_id << "_" << a;
					add_function(strlen(act.args[0].val), act.args[0].val, s.str(), 0);

					code << s.str() << "()\n";
					break;
				}

				case action_type::act_normal: {
					if (act.type->exec == action_type::exec_none) break;

					if (act.target != action::self) code << "with (" << act.target << ")";
					if (act.type->question) code << "if (" << (act.inv ? "!" : "");

					if (act.type->exec == action_type::exec_code) {
						code << "action_lib";
						if (act.type->parent > -1) code << act.type->parent;
						code << "_" << act.type->id;
					}
					else
						code << act.type->code;

					code << '(';
					unsigned int n = 0;
					for (; n < act.nargs; n++) {
						if (i != 0) code << ", ";
						code << act.args[n];
					}
					for (; n < 16; n++) {
						if (n != 0) code << ", ";
						code << 0;
					}
					if (act.type->relative) {
						code << ", " << act.relative;
					}
					code << ")";

					if (act.type->question) code << ")";

					code << '\n';
					break;
				}

				default: /* do nothing */;
				}
			}

			std::ostringstream s;
			s << obj.name << "_" << evt.main_id << "_" << evt.sub_id;
			std::string c = code.str();
			add_function(c.size(), c.c_str(), s.str(), 0);
		}
	}
}

std::ostream &operator <<(std::ostream &out, const argument &arg) {
	switch (arg.kind) {
	case argument::arg_expr:
		return out << arg.val;

	case argument::arg_both:
		if (arg.val[0] == '"' || arg.val[0] == '\'') return out << arg.val;

	// fall through
	case argument::arg_string: {
		std::string val(arg.val);
		while (val.find('"') != std::string::npos) {
			val.replace(val.find('"'), 1, "\"+'\"'+\"");
		}
		return out << '"' << val << '"';
	}

	case argument::arg_bool:
		return out << (arg.val[0] == '0');

	case argument::arg_menu:
		return out << arg.val;

	case argument::arg_color:
		return out << '$' << arg.val;

	default:
		return out << arg.resource;
	}
}

void linker::add_function(size_t length, const char *data, const std::string &name, int args) {
	buffer code(length, data);
	token_stream tokens(code);
	parser parser(tokens, errors);
	errors.set_context(name);

	std::unique_ptr<node> program(parser.getprogram());
	if (errors.count() > 0) return;

	compiler.add_function(program.get(), name.c_str(), args);
}
