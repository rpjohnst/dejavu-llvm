#include "dejavu/linker/build.h"
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

#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetData.h"

#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/ToolOutputFile.h"

using namespace llvm;

llvm_shutdown_obj y;

void build(const char *target, game &source, error_stream &errors) {
	InitializeNativeTarget();

	std::string triple = sys::getHostTriple(), error;
	std::unique_ptr<TargetMachine> machine(
		TargetRegistry::lookupTarget(triple, error)->createTargetMachine(triple, "", "")
	);
	const TargetData *td = machine->getTargetData();

	node_codegen compiler(td);

	for (unsigned int i = 0; i < source.nscripts; i++) {
		printf("%s\n", source.scripts[i].name);

		buffer code(strlen(source.scripts[i].code), source.scripts[i].code);
		token_stream tokens(code);
		parser parser(tokens, errors);

		std::unique_ptr<node> program(parser.getprogram());
		compiler.add_function(program.get(), source.scripts[i].name, 0);
	}

	Module &game = compiler.get_module();

	Linker linker("", "", game.getContext());
	linker.LinkInModule(&game);
	bool is_native;
	linker.LinkInFile(sys::Path("runtime.bc"), is_native);
	Module *module = linker.getModule();

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
		errors.push_back(error_info);
		return;
	}

	pm.add(createBitcodeWriterPass(out->os()));
	pm.run(*module);
	out->keep();
}
