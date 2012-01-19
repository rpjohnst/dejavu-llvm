package org.dejavu;

import org.lateralgm.main.LGM;
import java.io.File;

public class Driver {
	public static void compile(File target, ProgressPane progress) {
		compile(target, false, progress);
	}

	public static native void compile(File target, boolean debug, ProgressPane progress);

	static {
		try {
			File dejavu = new File(
				Driver.class.getProtectionDomain().getCodeSource().getLocation().toURI()
			);
			System.load(new File(dejavu.getParent(), "dejavu.so").getPath());
		}
		catch (Exception e) {
			e.printStackTrace();
		}
	}
}
