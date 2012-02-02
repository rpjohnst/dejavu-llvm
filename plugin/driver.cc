#include "driver.h"
#include "dejavu/linker/linker.h"
#include "dejavu/linker/game.h"
#include "dejavu/compiler/error_stream.h"

#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ManagedStatic.h"

#include <cstdio>
#include <sstream>

class error_printer : public error_stream {
public:
	error_printer(build_log &log) : log(log) {}

	void set_context(const std::string &c) {
		context = c;
		errors = 0;
	}

	void push_back(const unexpected_token_error &e) {
		std::ostringstream s;
		s	<< context << ":" << e.unexpected.row << ":" << e.unexpected.col << ": "
			<< "error: unexpected '" << e.unexpected << "'; expected ";
		if (e.expected) s << e.expected;
		else s << e.expected_token;
		s << "\n";

		log.append(s.str().c_str());
		errors++;
	}

	void push_back(const std::string &e) {
		log.append(e.c_str());
		errors++;
	}

	int count() { return errors; }

private:
	build_log &log;
	int errors = 0;

	std::string context = "<untitled>";
};

llvm::llvm_shutdown_obj y;

void compile(const char *target, game &source, build_log &log) {
	error_printer errors(log);

	llvm::InitializeNativeTarget();
	linker(source, llvm::sys::getHostTriple(), errors).build(target);
}
