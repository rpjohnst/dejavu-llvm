#ifndef DRIVER_H
#define DRIVER_H

#include "dejavu/linker/build.h"
struct game;

// impure-virtual to appease Java
struct build_log {
	virtual ~build_log() {}
	virtual void append(const char*) {}
	virtual void message(const char*) {}
	virtual void percent(int) {}

protected:
	build_log() {}
};

void compile(const char *target, game &source, build_log &log);

#endif
