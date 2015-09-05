package org.dejavu;

import org.dejavu.backend.*;
import org.lateralgm.main.LGM;
import org.lateralgm.components.GmTreeGraphics;
import org.lateralgm.components.impl.CustomFileFilter;
import java.awt.event.*;
import java.awt.*;
import javax.swing.*;
import java.net.*;
import java.io.*;
import java.nio.file.Files;
import java.util.*;

public class Runner implements ActionListener, LGM.ReloadListener {
	private JMenuItem compileItem, runItem, debugItem;
	private JButton compileButton, runButton, debugButton;

	private ProgressPane progress;

	public Runner() {
		LGM.mdi.setBackground(Color.LIGHT_GRAY);

		// progress pane
		progress = new ProgressPane();
		progress.setMinimumSize(new Dimension(0, 150));

		JSplitPane main = (JSplitPane)LGM.content.getParent();
		JSplitPane split = new JSplitPane(JSplitPane.VERTICAL_SPLIT, true, LGM.content, progress);
		split.setResizeWeight(1.0);
		split.setOneTouchExpandable(true);
		main.setRightComponent(split);
		main.setDividerLocation(200);

		// menu
		JMenu runMenu = new JMenu("Run");

		compileItem = createMenuItem("Toolbar.COMPILE", "Create Executable");
		runMenu.add(compileItem);

		runItem = createMenuItem("Toolbar.RUN", "Run");
		runItem.setAccelerator(KeyStroke.getKeyStroke(116, 0));
		runMenu.add(runItem);

		debugItem = createMenuItem("Toolbar.DEBUG", "Debug");
		debugItem.setAccelerator(KeyStroke.getKeyStroke(117, 0));
		runMenu.add(debugItem);

		LGM.frame.getJMenuBar().add(runMenu, 3);

		// buttons
		LGM.tool.add(new JToolBar.Separator(), 4);

		compileButton = createButton("Toolbar.COMPILE", "Compile a stand-alone executable");
		LGM.tool.add(compileButton, 5);

		runButton = createButton("Toolbar.RUN", "Run the game");
		LGM.tool.add(runButton, 6);

		debugButton = createButton("Toolbar.DEBUG", "Run the game in debug mode");
		LGM.tool.add(debugButton, 7);

		// ui hooks
		LGM.addReloadListener(this);
	}

	private JMenuItem createMenuItem(String key, String title) {
		JMenuItem item = new JMenuItem(title);
		Icon icon = getIconForKey(key);
		item.setIcon(icon != null ? icon : GmTreeGraphics.getBlankIcon());
		item.addActionListener(this);
		return item;
	}

	private JButton createButton(String key, String tooltip) {
		JButton button = new JButton();
		Icon icon = getIconForKey(key);
		button.setIcon(icon != null ? icon : GmTreeGraphics.getBlankIcon());
		button.setToolTipText(tooltip);
		button.addActionListener(this);
		return button;
	}

	private static Icon getIconForKey(String key) {
		Properties icons = new Properties();
		InputStream is = Runner.class.getClassLoader().getResourceAsStream(
			"org/dejavu/icons.properties"
		);

		try {
			icons.load(is);
		}
		catch (IOException e) {
			System.err.println("Unable to read icons.properties");
		}

		String name = icons.getProperty(key, "");
		if (name.isEmpty()) return null;

		URL url = Runner.class.getClassLoader().getResource("org/dejavu/icons/" + name);
		if (url == null) return null;

		return new ImageIcon(url);
	}

	private synchronized boolean build(File output, File target, boolean debug) {
		progress.reset();
		progress.message("writing game data");
		LGM.commitAll();
		game source = new Writer(LGM.currentFile, progress).write();

		progress.percent(10);
		progress.message("building game");
		progress.append(
			"building " + source.getName() + " (" + source.getVersion() + ")" +
			" to " + target.getPath() + "\n"
		);
		boolean success = dejavu.compile(
			output.getPath(), target.getPath(),
			source, progress.new Log(), debug
		);

		if (success) {
			progress.percent(100);
			progress.message("finished");
		}
		else {
			progress.percent(0);
			progress.message("failed");
		}

		return success;
	}

	private void compile() {
		String ext = ""; // todo: this is based on platform

		JFileChooser save = new JFileChooser();
		save.setFileFilter(new CustomFileFilter(ext, "Executable files"));
		if (save.showSaveDialog(LGM.frame) != JFileChooser.APPROVE_OPTION) return;

		File file = save.getSelectedFile();
		if (!file.getName().endsWith(ext)) file = new File(file.getPath() + ext);

		// todo: factor this out and do it per-project
		final File output;
		try {
			output = Files.createTempDirectory("djv").toFile();
		}
		catch (IOException e) {
			e.printStackTrace();
			return;
		}

		final File target = file;
		new Thread() { public void run() {
			build(output, target, false);
		} }.start();
	}

	private void run() {
		final File output;
		try {
			output = Files.createTempDirectory("djv").toFile();
		}
		catch (IOException e) {
			e.printStackTrace();
			return;
		}

		final File target = new File(
			output.getPath() + File.separatorChar + "game"
		);

		new Thread() { public void run() {
			boolean success = build(output, target, false);
			if (success) progress.append("run " + target.getPath() + "\n");
		} }.start();
	}

	private void debug() {
		final File output;
		try {
			output = Files.createTempDirectory("djv").toFile();
		}
		catch (IOException e) {
			e.printStackTrace();
			return;
		}

		final File target = new File(
			output.getPath() + File.separatorChar + "game"
		);

		new Thread() { public void run() {
			boolean success = build(output, target, true);
			if (success) progress.append("debug " + target.getPath() + "\n");
		} }.start();
	}

	@Override
	public void actionPerformed(ActionEvent event) {
		Object source = event.getSource();
		if (source == compileItem || source == compileButton) compile();
		if (source == runItem || source == runButton) run();
		if (source == debugItem || source == debugButton) debug();
	}

	@Override
	public void reloadPerformed(boolean newRoot) {
		progress.reset();
		progress.clear();
	}
}
