#include "driver.h"
#include "dejavu/linker/build.h"
#include "dejavu/linker/game.h"
#include "dejavu/compiler/error_stream.h"

#include <cstdio>
#include <sstream>

class error_printer : public error_stream {
public:
	error_printer(build_log &log) : log(log), context(NULL) {}

	void push_back(const unexpected_token_error &e) {
		std::ostringstream s;
		s	<< context << ":" << e.unexpected.row << ":" << e.unexpected.col << ": "
			<< "error: unexpected '" << e.unexpected << "'; expected ";
		if (e.expected) s << e.expected;
		else s << e.expected_token;
		s << "\n";

		log.append(s.str().c_str());
	}

	void push_back(const std::string &e) {
		log.append(e.c_str());
	}

private:
	build_log &log;
	const char *context;
};

void compile(const char *target, game &source, build_log &log) {
	log.percent(10);
	log.append(static_cast<std::ostringstream&>(
		std::ostringstream()
			<< "building " << source.name << " (" << source.version << ") to " << target << "\n"
	).str().c_str());

	error_printer errors(log);
	build(target, source, errors);
}
