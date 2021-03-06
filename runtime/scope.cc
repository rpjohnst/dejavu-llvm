#include <dejavu/runtime/scope.h>
#include <dejavu/runtime/error.h>

static scope global;

static table<string*, var*> globalvar;
extern "C" void insert_globalvar(string *name) {
	globalvar[name] = &global[name];
}

extern "C" var *lookup(
	scope *self, scope *other, double id, string *name, bool lvalue
) {
	scope *s = 0;
	switch ((int)id) {
	case -1: s = self; break;
	case -2: s = other; break;
	case -5: s = &global; break;

	// todo: check on all.foo
	case -3: case -4:
		show_error(self, other, "variable does not exist", true);
		return 0;
	case -6:
		show_error(self, other, "local is not supported", true);
		return 0;

	// todo: other instance access
	default: return 0;
	}

	if (s->find(name) == s->end()) {
		if (!lvalue) {
			show_error(self, other, "variable does not exist", true);
			return 0;
		}

		s->insert(name);
	}
	return &(*s)[name];
}

extern "C" var *lookup_default(
	scope *self, scope *other, string *name, bool lvalue
) {
	table<string*, var*>::node *n = globalvar.find(name);
	if (n != globalvar.end()) {
		return n->v;
	}

	return lookup(self, other, -1, name, lvalue);
}
