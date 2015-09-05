#ifndef LINKER_H
#define LINKER_H

#include <dejavu/compiler/codegen.h>

struct game;
struct error_stream;

namespace llvm {
	class DataLayout;
}

class linker {
public:
	linker(
		const char *output, game&, error_stream&,
		const std::string &triple, llvm::LLVMContext &context
	);
	bool build(const char *target, bool debug);

private:
	bool link(const char *target, bool debug);

	void build_libraries();
	void build_scripts();
	void build_objects();

	void add_function(
		size_t length, const char *code,
		const std::string &name, int args, bool var
	);

	llvm::LLVMContext &context;
	std::unique_ptr<llvm::Module> runtime;
	const char *output;

	game &source;
	error_stream &errors;
	node_codegen compiler;
};

#endif
