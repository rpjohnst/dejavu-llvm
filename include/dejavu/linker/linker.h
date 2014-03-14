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
	linker(game&, const std::string &triple, error_stream&);
	bool build(const char *target, bool debug);

private:
	void build_libraries();
	void build_scripts();
	void build_objects();

	void add_function(
		size_t length, const char *code,
		const std::string &name, int args, bool var
	);

	game &source;
	error_stream &errors;

	const llvm::DataLayout *dl;
	node_codegen compiler;
};

#endif
