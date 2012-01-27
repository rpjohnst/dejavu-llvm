package org.dejavu;

import org.dejavu.backend.*;
import org.lateralgm.main.LGM;
import org.lateralgm.components.GmTreeGraphics;
import java.awt.event.*;
import java.awt.*;
import javax.swing.*;
import java.net.*;
import java.io.*;
import java.util.*;

public class Runner implements ActionListener {
	private JMenuItem compileItem, runItem, debugItem;
	private JButton compileButton, runButton, debugButton;

	private ProgressPane progress;

	public Runner() {
		LGM.mdi.setBackground(Color.LIGHT_GRAY);

		progress = new ProgressPane();
		progress.setMinimumSize(new Dimension(0, 150));

		JSplitPane main = (JSplitPane)LGM.content.getParent();
		JSplitPane split = new JSplitPane(JSplitPane.VERTICAL_SPLIT, true, LGM.content, progress);
		split.setResizeWeight(1.0);
		split.setOneTouchExpandable(true);
		main.setRightComponent(split);
		main.setDividerLocation(200);

		// menus
		JMenu fileMenu = LGM.frame.getJMenuBar().getMenu(0);
		fileMenu.add(new JSeparator(), 4);

		compileItem = new JMenuItem("Create Executable");
		compileItem.addActionListener(this);
		fileMenu.add(compileItem, 5);

		JMenu runMenu = new JMenu("Run");

		runItem = new JMenuItem("Run");
		runItem.setAccelerator(KeyStroke.getKeyStroke(116, 0));
		runItem.addActionListener(this);
		runMenu.add(runItem);

		debugItem = new JMenuItem("Debug");
		debugItem.setAccelerator(KeyStroke.getKeyStroke(117, 0));
		debugItem.addActionListener(this);
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
	}

	private void build(final File target) {
		progress.reset();

		new Thread() { public void run() {
			progress.message("writing game data");
			LGM.commitAll();
			game source = new Writer(LGM.currentFile, progress).write();
			dejavu.compile(target.getPath(), source, progress.new Log());
		} }.start();
	}

	private void compile() {
		JFileChooser save = new JFileChooser();
		if (save.showSaveDialog(LGM.frame) != JFileChooser.APPROVE_OPTION) return;

		build(save.getSelectedFile());
	}

	private void run() {
		File target;
		try {
			target = File.createTempFile("djv", "");
		}
		catch (IOException e) {
			e.printStackTrace();
			return;
		}

		build(target);

		progress.append("run " + target.getPath() + "\n");
	}

	private void debug() {
		File target;
		try {
			target = File.createTempFile("djv", "");
		}
		catch (IOException e) {
			e.printStackTrace();
			return;
		}

		build(target);

		progress.append("debug " + target.getPath() + "\n");
	}

	public void actionPerformed(ActionEvent event) {
		Object source = event.getSource();
		if (source == compileItem || source == compileButton) compile();
		if (source == runItem || source == runButton) run();
		if (source == debugItem || source == debugButton) debug();
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
}
