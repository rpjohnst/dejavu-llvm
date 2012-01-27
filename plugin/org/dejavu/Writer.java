package org.dejavu;

import org.dejavu.backend.*;
import org.lateralgm.main.LGM;
import org.lateralgm.file.*;
import org.lateralgm.resources.*;
import org.lateralgm.resources.GmObject.*;
import org.lateralgm.resources.sub.*;
import org.lateralgm.resources.library.*;
import static org.lateralgm.main.Util.deRef;
import java.util.List;

public class Writer {
	private GmFile file;
	private game out;
	private ProgressPane log;

	public Writer(GmFile f, ProgressPane l) {
		file = f;
		out = new game();
		log = l;
	}

	public game write() {
		out.setVersion(file.format != null ? file.format.getVersion() : -1);
		out.setName(file.uri != null ? file.uri.toString() : "<untitled>");

		writeScripts();
		return out;
	}

	private void writeScripts() {
		ResourceList<Script> scripts = file.resMap.getList(Script.class);
		scriptArray scriptsOut = new scriptArray(scripts.size());
		int i = 0;
		for (Script scr : scripts) {
			script s = new script();
			s.setId(scr.getId());
			s.setName(scr.getName());
			s.setCode(scr.getCode());
			scriptsOut.setitem(i, s);
			i++;
		}
		out.setNscripts(scripts.size());
		out.setScripts(scriptsOut.cast());
	}

}
