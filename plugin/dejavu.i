%module(directors="1") dejavu

%{
#include "dejavu/linker/game.h"
#include "driver.h"
%}

%feature("director") build_log;

%include "carrays.i"
%array_class(script, scriptArray);
%array_class(object, objectArray);
%array_class(event, eventArray);
%array_class(action, actionArray);
%array_class(argument, argArray);

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
