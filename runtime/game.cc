#include <dejavu/runtime/variant.h>
#include <dejavu/runtime/scope.h>

extern "C" variant scr_0(scope *self, scope *other, short, variant args[]);

string_pool strings;

int main(int argc, char *argv[]) {
	variant *args = new variant[argc];
	for (int i = 0; i < argc; i++) {
		args[i].type = 1;

		string *ptr = strings.intern(argv[i]);
		ptr->retain();

		args[i].string = ptr;
	}

	scope self, other;
	variant *foo = new variant[1];
	foo[0] = strings.intern("foo");
	self[foo->string] = var{1, 1, foo};

	scr_0(&self, &other, argc, args);

	for (int i = 0; i < argc; i++) {
		args[i].string->release();
	}
	delete[] args;

	return 0;
}
