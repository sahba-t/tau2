package edu.uoregon.tau.vis;

import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;

import javax.swing.*;
import javax.swing.plaf.basic.BasicComboPopup;
import javax.swing.plaf.basic.ComboPopup;
import javax.swing.plaf.metal.MetalComboBoxUI;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.text.DecimalFormat;

public class HeatMapWindow extends JFrame implements ActionListener{

	private SteppedComboBox pathSelector = null;
	private SteppedComboBox figureSelector = null;
	private JPanel mainPanel = null;
	private Map/*<String, double[][][]>*/ maps = null;
	private Map/*<String, double[]>*/ maxs = null;
	private Map/*<String, double[]>*/ mins = null;
	private final static String allPaths = "All Paths";
	private final static String CALLS = "NUMBER OF CALLS";
	private final static String MAX = "MAX MESSAGE BYTES";
	private final static String MIN = "MIN MESSAGE BYTES";
	private final static String MEAN = "MEAN MESSAGE BYTES";
	private final static String STDDEV = "MESSAGE BYTES STDDEV";
	private final static String VOLUME = "TOTAL VOLUME BYTES";
	private final static String[] figures = {CALLS, MAX, MIN, MEAN, STDDEV, VOLUME};
	private String currentPath = allPaths;
	private String currentFigure = CALLS;
	private final static String filenamePrefix = "HeatMap";
	private int size = 0;
	public final static int viewSize = 512;  // the size of the heatmap in the interface
	public final static int maxCells = 256;   // the number of heatmap cells, max, to show
	public final static int viewRatio = 2;   // the ratio between those two
	private HeatMap heatMap = null;

	public HeatMapWindow(String title, Map maps, Map maxs, Map mins, int size) {
		super(title);
		this.maps = maps;
		this.maxs = maxs;
		this.mins = mins;
		this.size = size;
		pathSelector = new SteppedComboBox(maps.keySet().toArray());
		Dimension d = pathSelector.getPreferredSize();
	    pathSelector.setPreferredSize(new Dimension(50, d.height));
	    pathSelector.setPopupWidth(d.width);
		figureSelector = new SteppedComboBox(figures);
		d = figureSelector.getPreferredSize();
	    figureSelector.setPreferredSize(new Dimension(50, d.height));
	    figureSelector.setPopupWidth(d.width);
		this.setResizable(false);
		drawFigures(true);
	}

	private void drawFigures(boolean centerWindow) {
		mainPanel = new JPanel(new GridBagLayout());
		this.getContentPane().add(mainPanel);
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.BOTH;
		c.anchor = GridBagConstraints.CENTER;
		c.weightx = 0.99;
		c.insets = new Insets(2,2,2,2);

		c.gridx = 0;
		c.gridy = 0;
		int dataIndex = 0;
		for (dataIndex = 0 ; dataIndex < figures.length ; dataIndex++) {
			if (figures[dataIndex].equals(currentFigure)) {
				break;
			}
		}
		mainPanel.add(buildMapPanel(dataIndex, currentFigure),c);
		c.weightx = 0.01;
		c.gridx = 1;
		mainPanel.add(buildOptionPanel("DISPLAY OPTIONS"),c);
		
        if (centerWindow) {
	        Toolkit tk = Toolkit.getDefaultToolkit();
	        Dimension screenDimension = tk.getScreenSize();
	        int screenHeight = screenDimension.height;
	        int screenWidth = screenDimension.width;
	        //Window Stuff.
	        int windowWidth = 1000;
	        int windowHeight = 800;
	        //Find the center position with respect to this window.
	        int xPosition = (screenWidth - windowWidth) / 2;
	        int yPosition = (screenHeight - windowHeight) / 2;
	        setLocation(xPosition, yPosition);
        }

		this.pack();
	}

	private Component buildOptionPanel(String label) {
		JPanel panel = new JPanel(new GridBagLayout());
		panel.setBorder(BorderFactory.createLineBorder(Color.BLACK));
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.BOTH;
		c.anchor = GridBagConstraints.CENTER;
		c.weightx = 0.01;
		c.insets = new Insets(2,2,2,2);

		// title across the top
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = 5;
		JLabel title = new JLabel(label, JLabel.CENTER);
		title.setFont(new Font("PE", title.getFont().getStyle(), title.getFont().getSize()*2));
		panel.add(title,c);

		this.pathSelector.setSelectedItem(currentPath);
		this.pathSelector.addActionListener(this);
		this.pathSelector.addKeyListener(this.heatMap.getScanner());
		c.gridy = 1;
		panel.add(new JLabel("Callpath:"),c);
		c.gridy = 2;
		panel.add(this.pathSelector,c);
		
		this.figureSelector.setSelectedItem(currentPath);
		this.figureSelector.addActionListener(this);
		this.figureSelector.addKeyListener(this.heatMap.getScanner());
		c.gridy = 3;
		panel.add(new JLabel("Dataset:"),c);
		c.gridy = 4;
		panel.add(this.figureSelector,c);

		return panel;
	}

	private JPanel buildMapPanel(int index, String label) {
		JPanel panel = new JPanel(new GridBagLayout());
		panel.setBorder(BorderFactory.createLineBorder(Color.BLACK));
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.BOTH;
		c.anchor = GridBagConstraints.CENTER;
		c.weightx = 0.01;
		c.insets = new Insets(2,2,2,2);
		DecimalFormat f = new DecimalFormat("0.00E0");

		// title across the top
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = 5;
		JLabel title = new JLabel(label, JLabel.CENTER);
		title.setFont(new Font("PE", title.getFont().getStyle(), title.getFont().getSize()*2));
		panel.add(title,c);
		JLabel path = new JLabel(currentPath, JLabel.CENTER);
		c.gridy = 1;
		panel.add(path,c);

		// the x axis and the top of the legend
		c.gridwidth = 1;
		c.gridy = 2;
		c.gridx = 1;
		panel.add(new JLabel("0", JLabel.CENTER),c);
		c.gridx = 2;
		c.weightx = 0.99;
		panel.add(new JLabel("RECEIVER", JLabel.CENTER),c);
		c.weightx = 0.01;
		c.gridx = 3;
		panel.add(new JLabel(Integer.toString(size-1), JLabel.CENTER),c);

		// the y axis and the map and the legend
		c.gridx = 0;
		c.gridy = 3;
		panel.add(new JLabel("0", JLabel.CENTER),c);
		c.gridy = 4;
		c.weighty = 0.99;
		JLabel vertical = new JLabel("SENDER", JLabel.CENTER);
		vertical.setUI(new VerticalLabelUI(false));
		panel.add(vertical,c);
		c.weighty = 0.01;
		c.gridx = 1;
		c.gridy = 3;
		c.gridwidth = 3;
		c.gridheight = 3;
		double[][][] map = (double[][][])(maps.get(currentPath));
		double[] max = (double[])(maxs.get(currentPath));
		double[] min = (double[])(mins.get(currentPath));
		this.heatMap = new HeatMap(map[index], size, max[index], min[index], filenamePrefix);
		JScrollPane scroller = new JScrollPane(heatMap);
		scroller.setPreferredSize(new Dimension(viewSize,viewSize));
	    panel.add(scroller, c);
		c.gridwidth = 1;
		c.gridheight = 1;
		c.gridy = 3;
		c.gridx = 4;
		panel.add(new JLabel(f.format(max[index]), JLabel.CENTER),c);
		c.gridy = 4;
		c.weighty = 0.99;
	    panel.add(new HeatLegend(), c);
	    panel.add(new JPanel(), c);
		c.weighty = 0.01;

		// the bottom of the y axis and the bottom of the legend
		c.gridx = 0;
		c.gridy = 5;
		panel.add(new JLabel(Integer.toString(size-1), JLabel.CENTER),c);
		c.gridx = 4;
		panel.add(new JLabel(f.format(min[index]), JLabel.CENTER),c);
		return panel;
	}

	public void actionPerformed(ActionEvent actionEvent) {
		try {
			Object eventSrc = actionEvent.getSource();
			if (eventSrc.equals(this.pathSelector)) {
				String newPath = (String)this.pathSelector.getSelectedItem();
				if (!newPath.equals(currentPath)) {
					currentPath = newPath;
					this.remove(mainPanel);
					mainPanel = null;
					drawFigures(false);
					// new heatmap, new scanner, so add listeners
					this.figureSelector.addKeyListener(this.heatMap.getScanner());
					this.figureSelector.addKeyListener(this.heatMap.getScanner());
					this.heatMap.requestFocus();
				}
			}
			if (eventSrc.equals(this.figureSelector)) {
				String newFigure = (String)this.figureSelector.getSelectedItem();
				if (!newFigure.equals(currentFigure)) {
					currentFigure = newFigure;
					this.remove(mainPanel);
					mainPanel = null;
					drawFigures(false);
					// new heatmap, new scanner, so add listeners
					this.figureSelector.addKeyListener(this.heatMap.getScanner());
					this.figureSelector.addKeyListener(this.heatMap.getScanner());
					this.heatMap.requestFocus();
				}
			}
		} catch (Exception e) {
			System.err.println("actionPerformed Exception: " + e.getMessage());
			e.printStackTrace();
		} 
	}

	/**
	 * @return the heatMap
	 */
	public HeatMap getHeatMap() {
		return heatMap;
	}

}
