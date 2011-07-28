asdf + foo * 3 + "asdf" div $abc
foo = bar && baz(qux, "foo", $baf)
asdf[3 + 2] - (-foo + 3) / !bar.asdf

/* this is a comment summarizing the script */

var foo, bar, baz, qux;
foo = 3
bar = foo + 3;
baz = foo * (2 - bar) + "asdf"

qux[] = foo
qux[0] = bar
qux[1,2] = baz

self.foo = foo
global.bar = bar
baz.qux = qux

if foo = 3 then
begin
	// comment
	var bla;
	bla = 0 | 3 & $00fAe
	
	if (bar == 6) { bla = ~bla }
}
else if (foo != 3)
begin
	while true {
		function_call(asdf, 33, "asdf")
	end
end
else
	if (false) exit;

for (asdf = 4; asdf < 5; asdf += 1) {
	switch (asdf + 2) {
	case 6: with (asdf) do_something(); break;
	case 7: return;
	default: die();
	}
}
