#include "driver.h"
#include <dejavu/linker/linker.h>
#include <dejavu/linker/game.h>
#include <dejavu/compiler/error_stream.h>

#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ManagedStatic.h>

#include <cstdio>
#include <sstream>

class error_printer : public error_stream {
public:
	error_printer(build_log &log) : log(log) {}

	void set_context(const std::string &c) { context = c; }
	int count() { return errors; }

	void error(const unexpected_token_error &e) {
		std::ostringstream s;
		s	<< context << ":" << e.unexpected.row << ":" << e.unexpected.col
			<< ": error: unexpected '" << e.unexpected << "'; expected ";
		if (e.expected) s << e.expected;
		else s << e.expected_token;
		s << "\n";

		log.append(s.str().c_str());
		errors++;
	}

	void error(const redefinition_error &e) {
		std::ostringstream s;
		s	<< context << ": error:  redefinition of " << e.name << "\n";
		log.append(s.str().c_str());
		errors++;
	}

	void error(const unsupported_error &e) {
		std::ostringstream s;
		s	<< context << ":" << e.position.row << ":" << e.position.col
			<< ": error: feature is unsupported (" << e.name << ")\n";

		log.append(s.str().c_str());
		errors++;
	}

	void error(const std::string &e) {
		log.append(e.c_str());
		errors++;
	}

	void progress(int i, const std::string &n = "") {
		log.percent(i);
		if (!n.empty()) log.message(n.c_str());
	}

private:
	build_log &log;
	int errors = 0;

	std::string context = "<untitled>";
};

llvm::llvm_shutdown_obj y;

// todo: replace debug with configuration struct
bool compile(
	const char *output, const char *target,
	game &source, build_log &log, bool debug
) {
	error_printer errors(log);

	llvm::InitializeNativeTarget();

	// we need a new context for every compilation or LLVM chokes on types
	llvm::LLVMContext context;

	return linker(
		output, source, errors,
		llvm::sys::getDefaultTargetTriple(), context
	).build(target, debug);
}
