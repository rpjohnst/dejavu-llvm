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
import java.util.HashMap;

public class Writer {
	private GmFile file;
	private game out;
	private ProgressPane log;

	public Writer(GmFile f, ProgressPane l) {
		file = f;
		out = new game();
		log = l;
	}

	// todo: use JNI directly to avoid all this excessive copying
	public game write() {
		out.setVersion(file.format != null ? file.format.getVersion() : -1);
		out.setName(file.uri != null ? file.uri.toString() : "<untitled>");

		writeLibraries();

		writeScripts();
		writeObjects();

		return out;
	}

	// todo: do this on-demand, eventually keep libs compiled on installation
	private HashMap<LibAction, action_type> actionTypes = new HashMap<LibAction, action_type>();
	private void writeLibraries() {
		int actionCount = 0;
		for (Library lib : LibManager.libs) {
			actionCount += lib.libActions.size();
		}

		actionTypeArray actionsOut = new actionTypeArray(actionCount);
		int i = 0;
		for (Library lib : LibManager.libs) {
			for (LibAction act : lib.libActions) {
				action_type a = new action_type();
				a.setId(act.id);
				a.setParent(act.parentId);
				a.setKind(act.actionKind);
				a.setQuestion(act.question);
				a.setRelative(act.allowRelative);
				a.setExec(act.execType);
				a.setCode(act.execInfo);

				actionsOut.setitem(i, a);
				actionTypes.put(act, a);
				i++;
			}
		}
		out.setNactions(actionCount);
		out.setActions(actionsOut.move());
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
		out.setScripts(scriptsOut.move());
	}

	private void writeObjects() {
		ResourceList<GmObject> objects = file.resMap.getList(GmObject.class);
		objectArray objectsOut = new objectArray(objects.size());
		int i = 0;
		for (GmObject obj : objects) {
			object o = new object();
			o.setId(obj.getId());
			o.setName(obj.getName());

			o.setSprite(toId(obj.get(PGmObject.SPRITE), -1));
			o.setMask(toId(obj.get(PGmObject.MASK), -1));
			o.setParent(toId(obj.get(PGmObject.PARENT), -100));

			o.setSolid((Boolean)obj.get(PGmObject.SOLID));
			o.setVisible((Boolean)obj.get(PGmObject.VISIBLE));
			o.setPersistent((Boolean)obj.get(PGmObject.PERSISTENT));

			o.setDepth((Integer)obj.get(PGmObject.DEPTH));

			writeEvents(obj, o);

			objectsOut.setitem(i, o);
			i++;
		}
		out.setNobjects(objects.size());
		out.setObjects(objectsOut.move());
	}

	private void writeEvents(GmObject obj, object out) {
		int eventCount = 0;
		for (MainEvent me : obj.mainEvents) {
			eventCount += me.events.size();
		}

		eventArray eventsOut = new eventArray(eventCount);
		int i = 0;
		for (int mid = 0; mid < obj.mainEvents.size(); mid++) {
			for (Event evt : obj.mainEvents.get(mid).events) {
				event e = new event();
				e.setMain_id(mid);
				e.setSub_id(mid == MainEvent.EV_COLLISION ? toId(evt.other, -1) : evt.id);

				writeActions(evt, e);

				eventsOut.setitem(i, e);
				i++;
			}
		}
		out.setNevents(eventCount);
		out.setEvents(eventsOut.move());
	}

	private void writeActions(Event evt, event out) {
		actionArray actionsOut = new actionArray(evt.actions.size());
		int i = 0;
		for (Action act : evt.actions) {
			LibAction type = act.getLibAction();
			if (type == null) {
				log.append("unsupported action: " + act.toString() + "\n");
				continue;
			}

			action a = new action();
			a.setType(actionTypes.get(type));
			a.setRelative(act.isRelative());
			a.setInv(act.isNot());
			a.setTarget(GmObject.refAsInt(act.getAppliesTo()));

			writeArguments(act, a);

			actionsOut.setitem(i, a);
			i++;
		}
		out.setNactions(evt.actions.size());
		out.setActions(actionsOut.move());
	}

	private void writeArguments(Action act, action out) {
		argArray argsOut = new argArray(act.getArguments().size());
		int i = 0;
		for (Argument arg : act.getArguments()) {
			argument a = new argument();
			a.setKind(arg.kind);
			a.setVal(arg.getVal());
			a.setResource(toId(arg.getRes(), -1));

			argsOut.setitem(i, a);
			i++;
		}
		out.setNargs(act.getArguments().size());
		out.setArgs(argsOut.move());
	}

	private static int toId(Object obj, int def) {
		Resource<?, ?> res = deRef((ResourceReference<?>)obj);
		if (res != null) return ((InstantiableResource<?, ?>)res).getId();
		return def;
	}
}
