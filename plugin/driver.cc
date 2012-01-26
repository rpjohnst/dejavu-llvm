#include "driver.h"
#include "dejavu/linker/game.h"

#include <cstdio>
#include <sstream>

void compile(const char *target, game &source, build_log &log) {
	log.append(static_cast<std::ostringstream&>(
		std::ostringstream()
			<< "building " << source.name << " (" << source.version << ") to " << target << "\n"
	).str().c_str());

	for (int i = 0; i < source.nscripts; i++) {
		log.append(static_cast<std::ostringstream&>(
			std::ostringstream() << source.scripts[i].name << "\n"
		).str().c_str());
	}
}
