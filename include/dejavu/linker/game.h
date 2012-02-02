#ifndef GAME_H
#define GAME_H

// todo: replace char* with std::string to reduce strlen() (and copying?)

struct script {
	unsigned int id;
	char *name;
	char *code;
};

struct argument {
	enum {
		arg_expr, arg_string, arg_both, arg_bool, arg_menu, arg_color, arg_fontstr,
		arg_sprite, arg_sound, arg_background, arg_path, arg_script, arg_object,
		arg_room, arg_font, arg_timeline
	} kind;
	char *val;
	unsigned int resource;
};

struct action_type {
	int id, parent;

	enum {
		act_normal, act_begin, act_end, act_else, act_exit, act_repeat, act_variable,
		act_code, act_placeholder, act_separator, act_label
	} kind;

	bool question, relative;

	enum { exec_none, exec_function, exec_code } exec;
	char *code;

};

struct action {
	action_type *type;

	bool relative, inv;

	enum { self = -1, other = -2, all = -3, noone = -4, global = -5, local = -6 };
	int target;

	unsigned int nargs;
	argument *args;
};

struct event {
	unsigned int main_id, sub_id;
	unsigned int nactions;
	action *actions;
};

struct object {
	unsigned int id;
	char *name;

	int sprite, mask, parent;
	bool solid, visible, persistent;
	int depth;

	unsigned int nevents;
	event *events;
};

struct game {
	int version;
	char *name;

	unsigned int nactions;
	action_type *actions;

	unsigned int nscripts;
	script *scripts;

	unsigned int nobjects;
	object *objects;
};

#endif
