#ifndef GAME_H
#define GAME_H

struct script {
	unsigned int id;
	char *name;
	char *code;
};

struct game {
	int version;
	char *name;

	unsigned int nscripts;
	script *scripts;
};

#endif
