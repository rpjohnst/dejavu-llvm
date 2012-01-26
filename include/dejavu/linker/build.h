#ifndef BUILD_H
#define BUILD_H

struct game;
struct error_stream;

void build(const char *target, game&, error_stream&);

#endif
