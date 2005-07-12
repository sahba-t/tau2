package edu.uoregon.tau.paraprof;

import java.awt.*;
import java.awt.event.*;
import java.util.Iterator;
import java.util.List;

import javax.swing.*;

import edu.uoregon.tau.dms.dss.Function;
import edu.uoregon.tau.paraprof.enums.ValueType;
import edu.uoregon.tau.paraprof.enums.VisType;
import edu.uoregon.tau.vis.Plot;
import edu.uoregon.tau.vis.VisRenderer;

/**
 * This is the control panel for the ThreeDeeWindow.
 *    
 * TODO : ...
 *
 * <P>CVS $Id: ThreeDeeControlPanel.java,v 1.8 2005/07/12 18:07:02 amorris Exp $</P>
 * @author	Alan Morris
 * @version	$Revision: 1.8 $
 */
public class ThreeDeeControlPanel extends JPanel implements ActionListener {

    private ThreeDeeSettings settings;

    private ThreeDeeWindow window;
    private ParaProfTrial ppTrial;

    private JComboBox heightValueBox, heightMetricBox;
    private JComboBox colorValueBox, colorMetricBox;

    private JPanel subPanel;
    private VisRenderer visRenderer;

    private JTextField heightValueField = new JTextField("");
    private JTextField colorValueField = new JTextField("");

    private int selectedTab;
    private JTabbedPane tabbedPane; // keep a handle to remember the selected tab

    public ThreeDeeControlPanel(ThreeDeeWindow window, ThreeDeeSettings settings, ParaProfTrial ppTrial,
            VisRenderer visRenderer) {
        this.settings = settings;
        this.window = window;
        this.ppTrial = ppTrial;
        this.visRenderer = visRenderer;

        this.setLayout(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);

        gbc.fill = GridBagConstraints.NONE;
        gbc.anchor = GridBagConstraints.WEST;
        gbc.weightx = 0;
        gbc.weighty = 0;

        ButtonGroup group = new ButtonGroup();
        JRadioButton jrb = new JRadioButton(VisType.TRIANGLE_MESH_PLOT.toString(),
                settings.getVisType() == VisType.TRIANGLE_MESH_PLOT);
        jrb.addActionListener(this);
        group.add(jrb);
        addCompItem(this, jrb, gbc, 0, 0, 1, 1);

        jrb = new JRadioButton(VisType.BAR_PLOT.toString(), settings.getVisType() == VisType.BAR_PLOT);
        jrb.addActionListener(this);
        group.add(jrb);
        addCompItem(this, jrb, gbc, 0, 2, 1, 1);

        jrb = new JRadioButton(VisType.SCATTER_PLOT.toString(), settings.getVisType() == VisType.SCATTER_PLOT);
        jrb.addActionListener(this);
        group.add(jrb);
        addCompItem(this, jrb, gbc, 0, 3, 1, 1);

        createSubPanel();

    }

    private void createSubPanel() {
        if (subPanel != null) {
            this.remove(subPanel);
        }
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);

        if (settings.getVisType() == VisType.SCATTER_PLOT) {
            subPanel = createScatterPanel();
        } else {
            subPanel = createFullDataPanel();
        }
        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        addCompItem(this, subPanel, gbc, 0, 5, 1, 1);
        revalidate();
        validate();
        this.setPreferredSize(this.getMinimumSize());
    }

    private JPanel createScatterSelectionPanel(String name, final int index) {
        JPanel panel = new JPanel();
        panel.setLayout(new GridBagLayout());
        //        panel.setBorder(BorderFactory.createLoweredBevelBorder());
        GridBagConstraints gbc = new GridBagConstraints();

        gbc.fill = GridBagConstraints.NONE;
        gbc.anchor = GridBagConstraints.WEST;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;

        //addCompItem(panel, new JLabel(name), gbc, 0, 0, 1, 2);

        final JTextField functionField;

        String fname = "   <none>";
        if (settings.getScatterFunctions()[index] != null) {
            fname = settings.getScatterFunctions()[index].getName();
        }

        functionField = new JTextField(fname);
        functionField.setEditable(false);
        functionField.setBorder(BorderFactory.createLoweredBevelBorder());
        functionField.setCaretPosition(0);

        String[] items = new String[ppTrial.getNumberOfMetrics()];
        for (int i = 0; i < ppTrial.getNumberOfMetrics(); i++) {
            items[i] = ppTrial.getMetric(i).getName();
        }

        final JComboBox valueBox = new JComboBox(ValueType.VALUES);
        final JComboBox metricBox = new JComboBox(items);
        valueBox.setSelectedItem(settings.getScatterValueTypes()[index]);
        metricBox.setSelectedIndex(settings.getScatterMetricIDs()[index]);

        ActionListener metricSelector = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                try {
                    settings.setScatterValueType((ValueType) valueBox.getSelectedItem(), index);
                    settings.setScatterMetricID(metricBox.getSelectedIndex(), index);
                    window.redraw();
                } catch (Exception e) {
                    ParaProfUtils.handleException(e);
                }
            }
        };

        valueBox.addActionListener(metricSelector);
        metricBox.addActionListener(metricSelector);

        JButton functionButton = new JButton("...");
        functionButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                try {

                    FunctionSelectorDialog fSelector = new FunctionSelectorDialog(window, true,
                            ppTrial.getDisplayedFunctions().iterator(), settings.getScatterFunctions()[index], true);

                    if (fSelector.choose()) {
                        Function selectedFunction = (Function) fSelector.getSelectedObject();
                        settings.setScatterFunction(selectedFunction, index);

                        String fname = "   <none>";
                        if (settings.getScatterFunctions()[index] != null) {
                            fname = ParaProfUtils.getFunctionName(settings.getScatterFunctions()[index]);
                        }
                        functionField.setText(fname);
                        window.redraw();
                    }

                } catch (Exception e) {
                    ParaProfUtils.handleException(e);
                }
            }
        });

        JPanel subPanel = new JPanel();
        subPanel.setLayout(new GridBagLayout());

        gbc.insets = new Insets(1, 1, 1, 1);

        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        addCompItem(subPanel, functionField, gbc, 0, 0, 1, 1);
        gbc.weightx = 0.0;
        gbc.weighty = 0.0;
        gbc.fill = GridBagConstraints.NONE;
        addCompItem(subPanel, functionButton, gbc, 1, 0, 1, 1);

        gbc.insets = new Insets(5, 5, 5, 5);

        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        addCompItem(panel, subPanel, gbc, 1, 0, 2, 1);
        gbc.weightx = 0.5;
        gbc.weighty = 0.5;
        addCompItem(panel, valueBox, gbc, 1, 1, 1, 1);

        addCompItem(panel, metricBox, gbc, 2, 1, 1, 1);

        return panel;
    }

    private JPanel createScatterPanel() {

        JPanel panel = new JPanel();
        panel.setLayout(new GridBagLayout());
        panel.setBorder(BorderFactory.createRaisedBevelBorder());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);

        gbc.fill = GridBagConstraints.NONE;
        gbc.anchor = GridBagConstraints.WEST;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;

        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.WEST;
        gbc.weightx = 0.1;
        gbc.weighty = 0.1;

        addCompItem(panel, new JLabel("Width"), gbc, 0, 0, 1, 1);
        addCompItem(panel, createScatterSelectionPanel("Width", 0), gbc, 1, 0, 1, 1);
        addCompItem(panel, new JLabel("Depth"), gbc, 0, 1, 1, 1);
        addCompItem(panel, createScatterSelectionPanel("Depth", 1), gbc, 1, 1, 1, 1);
        addCompItem(panel, new JLabel("Height"), gbc, 0, 2, 1, 1);
        addCompItem(panel, createScatterSelectionPanel("Height", 2), gbc, 1, 2, 1, 1);
        addCompItem(panel, new JLabel("Color"), gbc, 0, 3, 1, 1);
        addCompItem(panel, createScatterSelectionPanel("Color", 3), gbc, 1, 3, 1, 1);

        tabbedPane = new JTabbedPane();

        Plot plot = window.getPlot();
        tabbedPane.addTab(plot.getName(), plot.getControlPanel(visRenderer));
        tabbedPane.addTab("Axes", plot.getAxes().getControlPanel(visRenderer));
        tabbedPane.addTab("ColorScale", window.getColorScale().getControlPanel(visRenderer));
        tabbedPane.addTab("Render", visRenderer.getControlPanel());
        tabbedPane.setMinimumSize(new Dimension(300, 160));
        tabbedPane.setSelectedIndex(selectedTab);

        gbc.fill = GridBagConstraints.BOTH;
        gbc.weightx = 0.5;
        gbc.weighty = 0.5;

        addCompItem(panel, tabbedPane, gbc, 0, 4, 2, 1);

        return panel;

    }

    
    private JPanel createSelectorPanel(int min, int max, final List names, final int index) {

        JPanel panel = new JPanel();
        panel.setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);

        final JScrollBar scrollBar = new JScrollBar(JScrollBar.HORIZONTAL, settings.getSelections()[index], 1,
                min, max);
        scrollBar.setBlockIncrement((max-min) / 10);

        final JTextField textField = new JTextField("<none>");

        textField.setHorizontalAlignment(JTextField.CENTER);

        if (settings.getSelections()[index] >= 0) {
            if (names != null) {
                textField.setText((String) names.get(settings.getSelections()[index]));
            }
        }

        textField.setEditable(false);
        textField.setCaretPosition(0);

        
        scrollBar.addAdjustmentListener(new AdjustmentListener() {
            public void adjustmentValueChanged(AdjustmentEvent e) {
                int selection = scrollBar.getValue();
                settings.setSelection(index, selection);
                if (selection >= 0 && names != null) {
                    textField.setText((String) names.get(selection));
                } else {
                    textField.setText("<none>");
                }
                textField.setCaretPosition(0);

                heightValueField.setText(window.getSelectedHeightValue());
                colorValueField.setText(window.getSelectedColorValue());

                window.redraw();
            }
        });

        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.WEST;
        gbc.weightx = 0.5;
        gbc.weighty = 0.5;

        ParaProfUtils.addCompItem(panel, textField, gbc, 1, 0, 1, 1);
        ParaProfUtils.addCompItem(panel, scrollBar, gbc, 1, 1, 1, 1);

        return panel;
    }

    
    private JPanel createFullDataPanel() {

        JPanel regularPanel = new JPanel();
        regularPanel.setLayout(new GridBagLayout());
        regularPanel.setBorder(BorderFactory.createLoweredBevelBorder());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);

        ActionListener metricChanger = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                try {

                    Plot plot = window.getPlot();

                    settings.setHeightMetricID(heightMetricBox.getSelectedIndex());
                    settings.setColorMetricID(colorMetricBox.getSelectedIndex());
                    settings.setHeightValue((ValueType) heightValueBox.getSelectedItem());
                    settings.setColorValue((ValueType) colorValueBox.getSelectedItem());

                    settings.setSize((int) plot.getWidth(), (int) plot.getDepth(), (int) plot.getHeight());
                    settings.setRegularAim(visRenderer.getAim());
                    settings.setRegularEye(visRenderer.getEye());

                    
                    heightValueField.setText(window.getSelectedHeightValue());
                    colorValueField.setText(window.getSelectedColorValue());

                    window.redraw();

                } catch (Exception e) {
                    ParaProfUtils.handleException(e);
                }
            }

        };

        String[] items = new String[ppTrial.getNumberOfMetrics()];
        for (int i = 0; i < ppTrial.getNumberOfMetrics(); i++) {
            items[i] = ppTrial.getMetric(i).getName();
        }

        heightValueBox = new JComboBox(ValueType.VALUES);
        heightValueBox.setSelectedItem(settings.getHeightValue());
        heightValueBox.addActionListener(metricChanger);

        heightMetricBox = new JComboBox(items);
        heightMetricBox.setSelectedIndex(settings.getHeightMetricID());
        heightMetricBox.addActionListener(metricChanger);

        colorValueBox = new JComboBox(ValueType.VALUES);
        colorValueBox.setSelectedItem(settings.getColorValue());
        colorValueBox.addActionListener(metricChanger);

        items = new String[ppTrial.getNumberOfMetrics()];
        for (int i = 0; i < ppTrial.getNumberOfMetrics(); i++) {
            items[i] = ppTrial.getMetric(i).getName();
        }

        colorMetricBox = new JComboBox(items);
        colorMetricBox.setSelectedIndex(settings.getColorMetricID());
        colorMetricBox.addActionListener(metricChanger);

        tabbedPane = new JTabbedPane();
        Plot plot = window.getPlot();
        tabbedPane.setTabLayoutPolicy(JTabbedPane.SCROLL_TAB_LAYOUT);
        tabbedPane.addTab(plot.getName(), plot.getControlPanel(visRenderer));
        tabbedPane.addTab("Axes", plot.getAxes().getControlPanel(visRenderer));
        tabbedPane.addTab("ColorScale", window.getColorScale().getControlPanel(visRenderer));
        tabbedPane.addTab("Render", visRenderer.getControlPanel());
        tabbedPane.setMinimumSize(new Dimension(290, 160));
        tabbedPane.setSelectedIndex(selectedTab);

        JPanel functionSelectorPanel = createSelectorPanel(-1, window.getFunctionNames().size(),
                window.getFunctionNames(), 0);
        JPanel nodeSelectorPanel = createSelectorPanel(0, ppTrial.getDataSource().getNumThreads(),
                window.getThreadNames(), 1);

        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.anchor = GridBagConstraints.WEST;
        gbc.weightx = 0.0;
        gbc.weighty = 0.0;

        addCompItem(regularPanel, new JLabel("Height Metric"), gbc, 0, 0, 2, 1);
        addCompItem(regularPanel, heightValueBox, gbc, 0, 1, 1, 1);
        addCompItem(regularPanel, heightMetricBox, gbc, 1, 1, 1, 1);
        addCompItem(regularPanel, new JLabel("Color Metric"), gbc, 0, 2, 2, 1);
        addCompItem(regularPanel, colorValueBox, gbc, 0, 3, 1, 1);
        addCompItem(regularPanel, colorMetricBox, gbc, 1, 3, 1, 1);

        JPanel selectionPanel = new JPanel();
        selectionPanel.setLayout(new GridBagLayout());
        //        selectionPanel.setBorder(BorderFactory.createLoweredBevelBorder());

        heightValueField.setEditable(false);
        colorValueField.setEditable(false);
        

        gbc.fill = GridBagConstraints.NONE;
        gbc.weightx = 0.0;
        addCompItem(selectionPanel, new JLabel("Function"), gbc, 0, 0, 1, 1);
        addCompItem(selectionPanel, new JLabel("Thread"), gbc, 0, 1, 1, 1);
        addCompItem(selectionPanel, new JLabel("Height value"), gbc, 0, 2, 1, 1);
        addCompItem(selectionPanel, new JLabel("Color value"), gbc, 0, 3, 1, 1);

        gbc.fill = GridBagConstraints.BOTH;
        gbc.weightx = 1.0;
        addCompItem(selectionPanel, functionSelectorPanel, gbc, 1, 0, 1, 1);
        addCompItem(selectionPanel, nodeSelectorPanel, gbc, 1, 1, 1, 1);
        addCompItem(selectionPanel, heightValueField, gbc, 1, 2, 1, 1);
        addCompItem(selectionPanel, colorValueField, gbc, 1, 3, 1, 1);

        addCompItem(regularPanel, selectionPanel, gbc, 0, 4, 2, 1);

        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.weightx = 0.5;
        gbc.weighty = 0.5;

        addCompItem(regularPanel, tabbedPane, gbc, 0, 5, 2, 1);

        return regularPanel;
    }

   
    public void actionPerformed(ActionEvent evt) {
        try {
            Object EventSrc = evt.getSource();

            if (EventSrc instanceof JRadioButton) {

                selectedTab = tabbedPane.getSelectedIndex();

                String arg = evt.getActionCommand();
                Plot plot = window.getPlot();

                if (settings.getVisType() == VisType.BAR_PLOT
                        || settings.getVisType() == VisType.TRIANGLE_MESH_PLOT) {
                    settings.setSize((int) plot.getWidth(), (int) plot.getDepth(), (int) plot.getHeight());
                    settings.setRegularAim(visRenderer.getAim());
                    settings.setRegularEye(visRenderer.getEye());
                } else if (settings.getVisType() == VisType.SCATTER_PLOT) {
                    //                    settings.setSize((int) plot.getWidth(), (int) plot.getDepth(), (int) plot.getHeight());
                    settings.setScatterAim(visRenderer.getAim());
                    settings.setScatterEye(visRenderer.getEye());
                }

                if (arg.equals(VisType.BAR_PLOT.toString())) {
                    settings.setVisType(VisType.BAR_PLOT);
                } else if (arg.equals(VisType.TRIANGLE_MESH_PLOT.toString())) {
                    settings.setVisType(VisType.TRIANGLE_MESH_PLOT);
                } else if (arg.equals(VisType.SCATTER_PLOT.toString())) {
                    settings.setVisType(VisType.SCATTER_PLOT);
                }

                window.redraw();
                createSubPanel();
            }
        } catch (Exception e) {
            ParaProfUtils.handleException(e);
        }
    }

    public void dataChanged() {
        window.redraw();
        createSubPanel();
        heightValueField.setText(window.getSelectedHeightValue());
        colorValueField.setText(window.getSelectedColorValue());
    }

    private void addCompItem(JPanel jPanel, Component c, GridBagConstraints gbc, int x, int y, int w, int h) {
        gbc.gridx = x;
        gbc.gridy = y;
        gbc.gridwidth = w;
        gbc.gridheight = h;
        jPanel.add(c, gbc);
    }

}
