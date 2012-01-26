package org.dejavu;

import org.dejavu.backend.build_log;
import org.lateralgm.main.LGM;
import javax.swing.*;
import javax.swing.text.*;
import java.awt.*;

public class ProgressPane extends JPanel {
	private JTextPane text;
	private JProgressBar progress;

	public ProgressPane() {
		super(new BorderLayout());

		text = new JTextPane();
		text.setEditable(false);
		add(new JScrollPane(
			text, JScrollPane.VERTICAL_SCROLLBAR_ALWAYS, JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED
		), BorderLayout.CENTER);

		progress = new JProgressBar();
		progress.setStringPainted(true);
		progress.setString(null);
		add(progress, BorderLayout.PAGE_END);
	}

	public void reset() {
		clear();
		percent(0);
		message(null);
	}

	public void append(String line) {
		StyledDocument doc = text.getStyledDocument();
		AttributeSet style = null;

		try {
			doc.insertString(doc.getLength(), line, style);
		}
		catch (BadLocationException e) {}

		text.setCaretPosition(doc.getLength());
	}

	public void clear() {
		text.setText(null);
	}

	public void message(String msg) {
		progress.setStringPainted(msg != null);
		progress.setString(msg);
	}

	public void percent(int pos) {
		progress.setValue(pos);
	}

	// this is to get around Java's lack of MI and/or SWIG's lack of interface generation
	public class Log extends build_log {
		public void append(String line) { ProgressPane.this.append(line); }
		public void message(String msg) { ProgressPane.this.message(msg); }
		public void percent(int pos) { ProgressPane.this.percent(pos); }
	}
}
