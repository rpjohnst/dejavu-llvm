%module(directors="1") dejavu

%{
#include "dejavu/linker/game.h"
#include "driver.h"
%}

%feature("director") build_log;

%include "carrays.i"

%define %array_move(type, array)
%typemap(javacode) array %{
	public type move() {
		swigCMemOwn = false;
		return cast();
	}
%}

%array_class(type, array);
%enddef

%array_move(script, scriptArray);
%array_move(object, objectArray);
%array_move(event, eventArray);
%array_move(action_type, actionTypeArray);
%array_move(action, actionArray);
%array_class(int, argumentTypeArray);
%array_move(argument, argArray);

%include "dejavu/linker/game.h"
%include "driver.h"

%pragma(java) jniclassimports="import java.io.File;"

%pragma(java) jniclasscode=%{
	static {
		try {
			File plugin = new File(
				$imclassname.class.getProtectionDomain().getCodeSource().getLocation().toURI()
			);
			System.load(new File(plugin.getParent(), "dejavu.so").getPath());
		}
		catch (Exception e) {
			System.err.println("Failed to load dejavu.so\n" + e);
			System.exit(1);
		}
	}
%}
