/* 
   ParaProfManager.java

   Title:      ParaProf
   Author:     Robert Bell
   Description:
   Notes:  I make heavy use of the TreeWillExpandListener listener to populate
           the tree nodes.  Before a node is expanded, the node is re-populated
	   with nodes.  This ensures that all the user has to do to update the
	   tree is collapse and expand the nodes.  Care is taken to ensure that
	   DefaultMutableTreeNode references are cleaned when a node is collased.
*/

package edu.uoregon.tau.paraprof;

import java.util.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import javax.swing.*;
import javax.swing.text.*;
import javax.swing.border.*;
import javax.swing.event.*;
import javax.swing.tree.*;
import javax.swing.table.*;
import edu.uoregon.tau.dms.dss.*;

public class ParaProfManager extends JFrame implements ActionListener, TreeSelectionListener, TreeWillExpandListener{
    public ParaProfManager(){
	
	try{
	    //####################################
	    //Window Stuff.
	    //####################################
	    int windowWidth = 800;
	    int windowHeight = 515;
	    
	    //Grab the screen size.
	    Toolkit tk = Toolkit.getDefaultToolkit();
	    Dimension screenDimension = tk.getScreenSize();
	    int screenHeight = screenDimension.height;
	    int screenWidth = screenDimension.width;
	    
	    
	    //Find the center position with respect to this window.
	    int xPosition = (screenWidth - windowWidth) / 2;
	    int yPosition = (screenHeight - windowHeight) / 2;

	    //Offset a little so that we do not interfere too much with the
	    //main window which comes up in the centre of the screen.
	    if(xPosition>50)
		xPosition = xPosition-50;
	    if(yPosition>50)
		yPosition = yPosition-50;
	    
	    this.setLocation(xPosition, yPosition);
	    setSize(new java.awt.Dimension(windowWidth, windowHeight));
	    setTitle("ParaProf Manager");
	    
	    //Add some window listener code
	    addWindowListener(new java.awt.event.WindowAdapter() {
		    public void windowClosing(java.awt.event.WindowEvent evt) {
			thisWindowClosing(evt);
		    }
		});
	    //####################################
	    //End - Window Stuff.
	    //####################################
      
	    //####################################
	    //Code to generate the menus.
	    //####################################
	    JMenuBar mainMenu = new JMenuBar();
      
	    //######
	    //File menu.
	    //######
	    JMenu fileMenu = new JMenu("File");

	    //Add a menu item.
	    JMenuItem menuItem = new JMenuItem("Database Configuration");
	    menuItem.addActionListener(this);
	    fileMenu.add(menuItem);

	    //Save menu.
	    JMenu subMenu = new JMenu("Save ...");
	  
	    menuItem = new JMenuItem("ParaProf Preferrences");
	    menuItem.addActionListener(this);
	    subMenu.add(menuItem);
	  
	    fileMenu.add(subMenu);
	    //End - Save menu.

	    //Add a menu item.
	    menuItem = new JMenuItem("Close This Window");
	    menuItem.addActionListener(this);
	    fileMenu.add(menuItem);
      
	    //Add a menu item.
	    menuItem = new JMenuItem("Exit ParaProf!");
	    menuItem.addActionListener(this);
	    fileMenu.add(menuItem);
	    //######
	    //End - File menu.
	    //######

	    //######
	    //Options menu.
	    //######
	    JMenu optionsMenu = new JMenu("Options");
	    
	    showApplyOperationItem = new JCheckBoxMenuItem("Show Apply Operation", false);
	    showApplyOperationItem.addActionListener(this);
	    optionsMenu.add(showApplyOperationItem);
	    //######
	    //End - Options menu.
	    //######

	    //######
	    //Help menu.
	    //######
	    JMenu helpMenu = new JMenu("Help");
      
	    //Add a menu item.
	    JMenuItem aboutItem = new JMenuItem("About ParaProf");
	    aboutItem.addActionListener(this);
	    helpMenu.add(aboutItem);
      
	    //Add a menu item.
	    JMenuItem showHelpWindowItem = new JMenuItem("Show Help Window");
	    showHelpWindowItem.addActionListener(this);
	    helpMenu.add(showHelpWindowItem);
	    //######
	    //End - Help menu.
	    //######
       
	    //Now, add all the menus to the main menu.
	    mainMenu.add(fileMenu);
	    mainMenu.add(optionsMenu);
	    mainMenu.add(helpMenu);
	    setJMenuBar(mainMenu);
	    //####################################
	    //End - Code to generate the menus.
	    //####################################

	    //######
	    //Add items to the first popup menu.
	    //######
	    JMenuItem jMenuItem = new JMenuItem("Add Application");
	    jMenuItem.addActionListener(this);
	    popup1.add(jMenuItem);

	    jMenuItem = new JMenuItem("Add Experiment");
	    jMenuItem.addActionListener(this);
	    popup1.add(jMenuItem);

	    jMenuItem = new JMenuItem("Add Trial");
	    jMenuItem.addActionListener(this);
	    popup1.add(jMenuItem);
	    //######
	    //End - Add items to the first popup menu.
	    //######

	    //######
	    //Add items to the second popup menu.
	    //######
	    jMenuItem = new JMenuItem("Add Experiment");
	    jMenuItem.addActionListener(this);
	    popup2.add(jMenuItem);

	    jMenuItem = new JMenuItem("Delete");
	    jMenuItem.addActionListener(this);
	    popup2.add(jMenuItem);

	    jMenuItem = new JMenuItem("Add Trial");
	    jMenuItem.addActionListener(this);
	    popup2.add(jMenuItem);
	    //######
	    //End - Add items to the second popup menu.
	    //######

	    //######
	    //Add items to the third popup menu.
	    //######
	    jMenuItem = new JMenuItem("Delete");
	    jMenuItem.addActionListener(this);
	    popup3.add(jMenuItem);

	    jMenuItem = new JMenuItem("Add Trial");
	    jMenuItem.addActionListener(this);
	    popup3.add(jMenuItem);
	    //######
	    //End - Add items to the third popup menu.
	    //######

	    //######
	    //Add items to the fourth popup menu.
	    //######
	    jMenuItem = new JMenuItem("Delete");
	    jMenuItem.addActionListener(this);
	    popup4.add(jMenuItem);
	    
	    jMenuItem = new JMenuItem("Upload Trial to DB");
	    jMenuItem.addActionListener(this);
	    popup4.add(jMenuItem);
	    //######
	    //End - Add items to the fourth popup menu.
	    //######

	    //######
	    //Add items to the fifth popup menu.
	    //######
	    jMenuItem = new JMenuItem("Upload Metric to DB");
	    jMenuItem.addActionListener(this);
	    popup5.add(jMenuItem);
	    //######
	    //End - Add items to the fifth popup menu.
	    //######
   
	    //####################################
	    //Create the tree.
	    //####################################
	    //Create the root node.
	    DefaultMutableTreeNode root = new DefaultMutableTreeNode("Applications");
  	    standard = new DefaultMutableTreeNode("Standard Applications");
	    runtime = new DefaultMutableTreeNode("Runtime Applications");
	    dbApps = new DefaultMutableTreeNode("DB Applications");

	    root.add(standard);
	    root.add(runtime);
	    root.add(dbApps);
      
	    treeModel = new DefaultTreeModel(root);
	    treeModel.setAsksAllowsChildren(true);
	    tree = new JTree(treeModel);
	    tree.getSelectionModel().setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
	    ParaProfTreeCellRenderer renderer = new ParaProfTreeCellRenderer();
	    tree.setCellRenderer(renderer);

	    //######
	    //Add a mouse listener for this tree.
	    MouseListener ml = new MouseAdapter() {
		    public void mousePressed(MouseEvent evt) {
			int selRow = tree.getRowForLocation(evt.getX(), evt.getY());
			TreePath path = tree.getPathForLocation(evt.getX(), evt.getY());
			if(path!=null) {
			    if((evt.getModifiers() & InputEvent.BUTTON1_MASK) == 0){
				DefaultMutableTreeNode selectedNode = (DefaultMutableTreeNode) path.getLastPathComponent();
				DefaultMutableTreeNode parentNode = (DefaultMutableTreeNode) selectedNode.getParent();
				Object userObject = selectedNode.getUserObject();
				if((selectedNode==standard)||(selectedNode==dbApps)){
				   clickedOnObject = selectedNode;
				   popup1.show(ParaProfManager.this, evt.getX(), evt.getY());
				}
				else if(userObject instanceof ParaProfApplication){
				    clickedOnObject = userObject;
				    popup2.show(ParaProfManager.this, evt.getX(), evt.getY());
				}
				else if(userObject instanceof ParaProfExperiment){
				    clickedOnObject = userObject;
				    popup3.show(ParaProfManager.this, evt.getX(), evt.getY());
				}
				else if(userObject instanceof ParaProfTrial){
				    clickedOnObject = userObject;
				    popup4.show(ParaProfManager.this, evt.getX(), evt.getY());
				}
				else if(userObject instanceof Metric){
				    clickedOnObject = userObject;
				    popup5.show(ParaProfManager.this, evt.getX(), evt.getY());
				}
			    }
			    else{
				if(evt.getClickCount()==2)
				    metric(path,true);
			    }
			    
			}
		    }
		};
	    tree.addMouseListener(ml);
	    //######
      
	    //######
	    //Add tree listeners.
	    tree.addTreeSelectionListener(this);
	    tree.addTreeWillExpandListener(this);
	    //######
	          
	    //Bung it in a scroll pane.
	    JScrollPane treeScrollPane = new JScrollPane(tree);
	    //####################################
	    //End - Create the tree.
	    //####################################


	    //####################################
	    //Set up the split panes, and add to content pane.
	    //####################################
	    jSplitInnerPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, treeScrollPane, getPanelHelpMessage(0));
	    jSplitInnerPane.setContinuousLayout(true);
	    jSplitOuterPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT, jSplitInnerPane, pPMLPanel);
	    (getContentPane()).add(jSplitOuterPane, "Center");
      
	    //Show before setting dividers.
	    //Components have to be realized on the screen before
	    //the dividers can be set.
	    this.show();
	    jSplitInnerPane.setDividerLocation(0.5);
	    jSplitOuterPane.setDividerLocation(1.0);
	    //####################################
	    //End - Set up the split pane, and add to content pane.
	    //####################################
	}
	catch(Exception e){
	    e.printStackTrace();
	    UtilFncs.systemError(e, null, "PPM01");
	}
    }

    //####################################
    //Interface code.
    //####################################
    
    //######
    //ActionListener.
    //######
    public void actionPerformed(ActionEvent evt){
	try{
	    Object EventSrc = evt.getSource();
	    if(EventSrc instanceof JMenuItem){
		String arg = evt.getActionCommand();
		if(arg.equals("Exit ParaProf!")){
		    setVisible(false);
		    dispose();
		    System.exit(0);
		} 
		else if(arg.equals("Close This Window")){
		    if(!(ParaProf.runHasBeenOpened)){
			setVisible(false);
			dispose();
			System.out.println("Quiting ParaProf!");
			System.exit(0);
		    }
		    else{
			dispose();
		    }
		}
		else if(arg.equals("Database Configuration")){
		    (new DBConfiguration(this)).show();}
		else if(arg.equals("ParaProf Preferrences")){
		    JFileChooser fileChooser = new JFileChooser();
		    //Set the directory to the current directory.
		    fileChooser.setCurrentDirectory(new File(System.getProperty("user.dir")));
		    fileChooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
		    fileChooser.setSelectedFile(new File("pref.dat"));
		    //Bring up the save file chooser.
		    int resultValue = fileChooser.showSaveDialog(this);
		    if(resultValue == JFileChooser.APPROVE_OPTION){
			//Get the file.
			File file = fileChooser.getSelectedFile();
			//Check to make sure that something was obtained.
			if(file != null){
			    try{
				ObjectOutputStream prefsOut = new ObjectOutputStream(new FileOutputStream(file));
				prefsOut.writeObject(ParaProf.savedPreferences);
				prefsOut.close();
			    }
			    catch(Exception e){
				//Display an error
				JOptionPane.showMessageDialog(this, "An error occured whilst trying to save ParaProf preferences.", "Error!"
							      ,JOptionPane.ERROR_MESSAGE);
			    }
			}
			else{
			    //Display an error
			    JOptionPane.showMessageDialog(this, "No filename was given!", "Error!"
							  ,JOptionPane.ERROR_MESSAGE);
			}
		    }
		}
		else if(arg.equals("Show Apply Operation")){
		    if(showApplyOperationItem.isSelected())
			jSplitOuterPane.setDividerLocation(0.75);
		    else
			jSplitOuterPane.setDividerLocation(1.00);
		}
		else if(arg.equals("About ParaProf")){
		    JOptionPane.showMessageDialog(this, ParaProf.getInfoString());
		}
		else if(arg.equals("Show Help Window")){
		    //Show the ParaProf help window.
		    ParaProf.helpWindow.clearText();
		    ParaProf.helpWindow.show();
		    
		    ParaProf.helpWindow.writeText("This is the experiment manager window.");
		    ParaProf.helpWindow.writeText("");
		    ParaProf.helpWindow.writeText("You can create an experiment, and then add separate runs,.");
		    ParaProf.helpWindow.writeText("which may contain one or more metrics (gettimeofday, cache misses, etc.");
		    ParaProf.helpWindow.writeText("You can also derive new metrics in this window.");
		    ParaProf.helpWindow.writeText("");
		    ParaProf.helpWindow.writeText("Please see ParaProf's documentation for more information.");
		}
		else if(arg.equals("Delete")){
		    if(clickedOnObject instanceof ParaProfApplication){
			ParaProfApplication application = (ParaProfApplication) clickedOnObject;
			if(application.dBApplication()){
			    PerfDMFSession perfDMFSession = new PerfDMFSession();
			    perfDMFSession.initialize(ParaProf.savedPreferences.getDatabaseConfigurationFile(), ParaProf.savedPreferences.getDatabasePassword());
			    perfDMFSession.deleteApplication(application.getID());
			    perfDMFSession.terminate();
			    //Remove any loaded trials associated with this application.
			    System.out.println("Removing associated loaded trials from this ParaProf session ...");
			    for(Enumeration e = loadedTrials.elements(); e.hasMoreElements() ;){
				ParaProfTrial loadedTrial = (ParaProfTrial) e.nextElement();
				if(loadedTrial.getApplicationID()==application.getID())
				    loadedTrials.remove(loadedTrial);
			    }
			    System.out.println("Done - Removing associated loaded trials!");
			    treeModel.removeNodeFromParent(application.getDMTN());
			    System.out.println("Application deleted!");
			}
			else{
			    System.out.println("Removing associated loaded trials from this ParaProf session ...");
			    ParaProf.applicationManager.removeApplication(application);
			    System.out.println("Removing associated loaded trials from this ParaProf session ...");
			    treeModel.removeNodeFromParent(application.getDMTN());
			    System.out.println("Application deleted!");
			}
		    }
		    else if(clickedOnObject instanceof ParaProfExperiment){
			ParaProfExperiment experiment = (ParaProfExperiment) clickedOnObject;
			if(experiment.dBExperiment()){
			    PerfDMFSession perfDMFSession = new PerfDMFSession();
			    perfDMFSession.initialize(ParaProf.savedPreferences.getDatabaseConfigurationFile(), ParaProf.savedPreferences.getDatabasePassword());
			    perfDMFSession.deleteExperiment(experiment.getID());
			    perfDMFSession.terminate();
			    System.out.println("Experiment deleted!");
			}
		    }
		    else if(clickedOnObject instanceof ParaProfTrial){
			ParaProfTrial trial = (ParaProfTrial) clickedOnObject;
			if(trial.dBTrial()){
			    PerfDMFSession perfDMFSession = new PerfDMFSession();
			    perfDMFSession.initialize(ParaProf.savedPreferences.getDatabaseConfigurationFile(), ParaProf.savedPreferences.getDatabasePassword());
			    perfDMFSession.deleteTrial(trial.getID());
			    perfDMFSession.terminate();
			    System.out.println("Trial deleted!");
			}
		    }
		}
		else if(arg.equals("Add Application")){
		    if(clickedOnObject == standard){
			ParaProfApplication application = addApplication(false, standard);
			this.expandApplicationType(0,application.getID(),application);
		    }
		    else if(clickedOnObject == dbApps){
			ParaProfApplication application = addApplication(true, dbApps);
			this.expandApplicationType(2,application.getID(),application);
		    }
		}
		else if(arg.equals("Add Experiment")){
		    if(clickedOnObject == standard){
			ParaProfApplication application = addApplication(false, standard);
			ParaProfExperiment experiment = addExperiment(false, application);
			if(application!=null||experiment!=null)
			    this.expandApplication(0,application.getID(),experiment.getID(),application,experiment);
			else
			    System.out.println("There was an error adding the experiment!");
		    }
		    else if(clickedOnObject == dbApps){
			ParaProfApplication application = addApplication(true, dbApps);
			ParaProfExperiment experiment = addExperiment(true, application);
			if(application!=null||experiment!=null)
			    this.expandApplication(2,application.getID(),experiment.getID(),application,experiment);
			else
			    System.out.println("There was an error adding the experiment!");
		    }
		    if(clickedOnObject instanceof ParaProfApplication){
			ParaProfApplication application = (ParaProfApplication) clickedOnObject;
			if(application.dBApplication()){
			    ParaProfExperiment experiment = addExperiment(true, application);
			    if(experiment!=null)
				this.expandApplication(2,application.getID(),experiment.getID(),null,experiment);
			    else
				System.out.println("There was an error adding the experiment!");
			}
			else{
			    ParaProfExperiment experiment = addExperiment(false, application);
			    if(experiment!=null)
				this.expandApplication(2,application.getID(),experiment.getID(),null,experiment);
			    else
				System.out.println("There was an error adding the experiment!");
			}
		    }
		}
		else if(arg.equals("Add Trial")){
		    if(clickedOnObject == standard){
			ParaProfApplication application = addApplication(false, standard);
			if(application!=null){
			    ParaProfExperiment experiment = addExperiment(false, application);
			    if(experiment!=null)
				(new LoadTrialPanel(this, application, experiment, false)).show();
			}
		    }
		    else if(clickedOnObject == dbApps){
			ParaProfApplication application = addApplication(true, dbApps);
			if(application!=null){
			    ParaProfExperiment experiment = addExperiment(true, application);
			    if(experiment!=null)
				(new LoadTrialPanel(this, application, experiment, false)).show();
			}
		    }
		    else if(clickedOnObject instanceof ParaProfApplication){
			ParaProfApplication application = (ParaProfApplication) clickedOnObject;
			if(application.dBApplication()){
			    ParaProfExperiment experiment = addExperiment(true, application);
			    if(experiment!=null)
				(new LoadTrialPanel(this, null, experiment, false)).show();
			}
			else{
			    ParaProfExperiment experiment = addExperiment(false, application);
			    if(experiment!=null)
				(new LoadTrialPanel(this, null, experiment, false)).show();
			}
		    }
		    else if(clickedOnObject instanceof ParaProfExperiment){
			ParaProfExperiment experiment = (ParaProfExperiment) clickedOnObject;
			if(experiment.dBExperiment()){
			    (new LoadTrialPanel(this, null, experiment, true)).show();
			}
			else
			    (new LoadTrialPanel(this, null, experiment, false)).show();
		    }
		}
		else if(arg.equals("Upload Trial to DB")){
		    if(clickedOnObject instanceof ParaProfTrial){
			ParaProfTrial clickedOnTrial = (ParaProfTrial) clickedOnObject;
			int[] array = this.getSelectedDBExperiment();
			if(array!=null){
			    System.out.println("Uploading trial: " + clickedOnTrial.getApplicationID() + "," +
			       clickedOnTrial.getExperimentID() + "," +
			       clickedOnTrial.getID() + " to the database. Please wait ...");
			    PerfDMFSession perfDMFSession = this.getDBSession();
			    if(perfDMFSession!=null){
				Trial trial = new Trial();
				trial.setDataSession(clickedOnTrial.getDataSession());
				trial.setName(clickedOnTrial.getName());
				trial.setExperimentID(array[1]);
				int[] maxNCT = clickedOnTrial.getMaxNCTNumbers();
				trial.setNodeCount(maxNCT[0]+1);
				trial.setNumContextsPerNode(maxNCT[1]+1);
				trial.setNumThreadsPerContext(maxNCT[2]+1);
				perfDMFSession.saveParaProfTrial(trial, -1);
				perfDMFSession.terminate();
			    }
			    System.out.println("Done uploading trial: " + clickedOnTrial.getApplicationID() + "," +
			       clickedOnTrial.getExperimentID() + "," +
			       clickedOnTrial.getID());
			}
		    }
		}
		else if(arg.equals("Upload Metric to DB")){
		    if(clickedOnObject instanceof Metric){
			Metric metric = (Metric) clickedOnObject;
			int[] array = this.getSelectedDBTrial();
			if(array!=null){
			    PerfDMFSession perfDMFSession = this.getDBSession();
			    if(perfDMFSession!=null){
				Trial trial = new Trial();
				trial.setDataSession(metric.getTrial().getDataSession());
				trial.setID(array[2]);
				perfDMFSession.saveParaProfTrial(trial, metric.getID());
				perfDMFSession.terminate();
				
				//Now need to make sure this metric's trial is reloaded when
				//clicked upon.
				for(Enumeration e = loadedTrials.elements(); e.hasMoreElements() ;){
				    ParaProfTrial loadedTrial = (ParaProfTrial) e.nextElement();
				    if((trial.getID()==array[2])&&(trial.getExperimentID()==array[1])
				       &&(trial.getApplicationID()==array[0])){
					loadedTrials.remove(loadedTrial);
				    }
				}
			    }
			}
		    }
		}
	    }
	}
	catch(Exception e){
	    UtilFncs.systemError(e, null, "ELM05");
	}
    }
    //######
    //End - ActionListener.
    //######

    //######
    //TreeSelectionListener
    //######
    public void valueChanged(TreeSelectionEvent event){
	TreePath path = tree.getSelectionPath();
	if(path == null)
	    return;
	if(UtilFncs.debug)
	    System.out.println("In valueChanged - path:" + path.toString());
	DefaultMutableTreeNode selectedNode = (DefaultMutableTreeNode) path.getLastPathComponent();
	DefaultMutableTreeNode parentNode = (DefaultMutableTreeNode) selectedNode.getParent();            
	Object userObject = selectedNode.getUserObject();

	if(selectedNode.isRoot()){
	    jSplitInnerPane.setRightComponent(getPanelHelpMessage(0));
	    jSplitInnerPane.setDividerLocation(0.5);
	}
	else if((parentNode.isRoot())){
	    if(userObject.toString().equals("Standard Applications")){
		jSplitInnerPane.setRightComponent(getPanelHelpMessage(1));
		jSplitInnerPane.setDividerLocation(0.5);
	    }
	    else if(userObject.toString().equals("Runtime Applications")){
		jSplitInnerPane.setRightComponent(getPanelHelpMessage(2));
		jSplitInnerPane.setDividerLocation(0.5);		
	    }
	    else if(userObject.toString().equals("DB Applications")){
		jSplitInnerPane.setRightComponent(getPanelHelpMessage(3));
		jSplitInnerPane.setDividerLocation(0.5);
	    }
	}
	else if(userObject instanceof ParaProfApplication){
	    jSplitInnerPane.setRightComponent(getTable(userObject));
	    jSplitInnerPane.setDividerLocation(0.5);
	}
	else if(userObject instanceof ParaProfExperiment){
	    jSplitInnerPane.setRightComponent(getTable(userObject));
	    jSplitInnerPane.setDividerLocation(0.5);
	}
	else if(userObject instanceof ParaProfTrial){
	    jSplitInnerPane.setRightComponent(getTable(userObject));
	    jSplitInnerPane.setDividerLocation(0.5);
	}
	else if(userObject instanceof Metric)
	    this.metric(path,false);
    }
    //######
    //End - TreeSelectionListener
    //######

    //######
    //TreeWillExpandListener
    //######
    public void treeWillCollapse(TreeExpansionEvent event){
	TreePath path = event.getPath();
	if(path == null)
	    return;
	if(UtilFncs.debug)
	    System.out.println("In treeWillCollapse - path:" + path.toString());
	DefaultMutableTreeNode selectedNode = (DefaultMutableTreeNode) path.getLastPathComponent();
	DefaultMutableTreeNode parentNode = (DefaultMutableTreeNode) selectedNode.getParent();
	Object userObject = selectedNode.getUserObject();
	
	if(selectedNode.isRoot()){
	    int childCount = standard.getChildCount();
	    for(int i=0;i<childCount;i++)
		this.clearDefaultMutableTreeNodes((DefaultMutableTreeNode) standard.getChildAt(i));
	    childCount = runtime.getChildCount();
	    for(int i=0;i<childCount;i++)
		this.clearDefaultMutableTreeNodes((DefaultMutableTreeNode) runtime.getChildAt(i));
	    childCount = dbApps.getChildCount();
	    for(int i=0;i<childCount;i++)
		this.clearDefaultMutableTreeNodes((DefaultMutableTreeNode) dbApps.getChildAt(i));
	}
	else if(parentNode.isRoot()){
	    if(userObject.toString().equals("Standard Applications")){
		int childCount = standard.getChildCount();
		for(int i=0;i<childCount;i++)
		    this.clearDefaultMutableTreeNodes((DefaultMutableTreeNode) standard.getChildAt(i));
	    }
	    else if(userObject.toString().equals("Runtime Applications")){
		int childCount = runtime.getChildCount();
		for(int i=0;i<childCount;i++)
		    this.clearDefaultMutableTreeNodes((DefaultMutableTreeNode) runtime.getChildAt(i));
	    }
	    else if(userObject.toString().equals("DB Applications")){
		int childCount = dbApps.getChildCount();
		for(int i=0;i<childCount;i++)
		    this.clearDefaultMutableTreeNodes((DefaultMutableTreeNode) dbApps.getChildAt(i));
	    }
	}
	else if(userObject instanceof ParaProfTreeNodeUserObject){
	   this.clearDefaultMutableTreeNodes(selectedNode); 
	}
    }
    public void treeWillExpand(TreeExpansionEvent event){
	TreePath path = event.getPath();
	if(path == null)
	    return;
	if(UtilFncs.debug)
	    System.out.println("In treeWillExpand - path:" + path.toString());
	DefaultMutableTreeNode selectedNode = (DefaultMutableTreeNode) path.getLastPathComponent();
	DefaultMutableTreeNode parentNode = (DefaultMutableTreeNode) selectedNode.getParent();            
	Object userObject = selectedNode.getUserObject();
	
	if(selectedNode.isRoot()){
	    //Do not need to do anything here.
	    return;
	}
	else if((parentNode.isRoot())){
	    if(userObject.toString().equals("Standard Applications")){
		jSplitInnerPane.setRightComponent(getPanelHelpMessage(1));
		jSplitInnerPane.setDividerLocation(0.5);
		//Refresh the application list.
		System.out.println("Loading application list ...");
		for(int i=standard.getChildCount(); i>0; i--){
		    treeModel.removeNodeFromParent(((DefaultMutableTreeNode) standard.getChildAt(i-1)));
		}
		ListIterator l = ParaProf.applicationManager.getApplicationList();
		while (l.hasNext()){
		    ParaProfApplication application = (ParaProfApplication)l.next();
		    DefaultMutableTreeNode applicationNode = new DefaultMutableTreeNode(application);
		    application.setDMTN(applicationNode);
		    treeModel.insertNodeInto(applicationNode, standard, standard.getChildCount());
		}
		System.out.println("Done loading application list.");
		return;
	    }
	    else if(userObject.toString().equals("Runtime Applications")){
		jSplitInnerPane.setRightComponent(getPanelHelpMessage(2));
		jSplitInnerPane.setDividerLocation(0.5);
	    }
	    else if(userObject.toString().equals("DB Applications")){
		try{
		    jSplitInnerPane.setRightComponent(getPanelHelpMessage(3));
		    jSplitInnerPane.setDividerLocation(0.5);
		    
		    if(ParaProf.savedPreferences.getDatabaseConfigurationFile()==null||ParaProf.savedPreferences.getDatabasePassword()==null){//Check to see if the user has set configuration information.
			JOptionPane.showMessageDialog(this, "Please set the database configuration information (file menu).",
						      "DB Configuration Error!",
						      JOptionPane.ERROR_MESSAGE);
			return;
		    }
		    else{//Test to see if configurataion file exists.
			File file = new File(ParaProf.savedPreferences.getDatabaseConfigurationFile());
			if(!file.exists()){
			    JOptionPane.showMessageDialog(this, "Specified configuration file does not exist.",
							  "DB Configuration Error!",
							  JOptionPane.ERROR_MESSAGE);
			    return;
			}
		    }
		    //Basic checks done, try to access the db.
		    //Refresh the application list.
		    System.out.println("Loading application list ...");
		    for(int i=dbApps.getChildCount(); i>0; i--){
			treeModel.removeNodeFromParent(((DefaultMutableTreeNode) dbApps.getChildAt(i-1)));
		    }
		    PerfDMFSession perfDMFSession = new PerfDMFSession(); 
		    perfDMFSession.initialize(ParaProf.savedPreferences.getDatabaseConfigurationFile(), ParaProf.savedPreferences.getDatabasePassword());
		    ListIterator l = perfDMFSession.getApplicationList();
		    while (l.hasNext()){
			ParaProfApplication application = new ParaProfApplication((Application)l.next());
			application.setDBApplication(true);
			DefaultMutableTreeNode applicationNode = new DefaultMutableTreeNode(application);
			application.setDMTN(applicationNode);
			treeModel.insertNodeInto(applicationNode, dbApps, dbApps.getChildCount());
		    }
		    perfDMFSession.terminate();
		    System.out.println("Done loading application list.");
		    return;
		}
		catch(Exception e){
		    e.printStackTrace();
		}
	    }
	}
	else if(userObject instanceof ParaProfApplication){
	    try{
		ParaProfApplication application = (ParaProfApplication) userObject;
		if(application.dBApplication()){
		    //Refresh the experiments list.
		    System.out.println("Loading experiment list ...");
		    for(int i=selectedNode.getChildCount(); i>0; i--){
			treeModel.removeNodeFromParent(((DefaultMutableTreeNode) selectedNode.getChildAt(i-1)));
		    }
		    PerfDMFSession perfDMFSession = new PerfDMFSession(); 
		    perfDMFSession.initialize(ParaProf.savedPreferences.getDatabaseConfigurationFile(), ParaProf.savedPreferences.getDatabasePassword());
		    //Set the application.
		    perfDMFSession.setApplication(application.getID());
		    ListIterator l = perfDMFSession.getExperimentList();
		    while (l.hasNext()){
			ParaProfExperiment experiment = new ParaProfExperiment((Experiment)l.next());
			experiment.setDBExperiment(true);
			experiment.setApplication(application);
			DefaultMutableTreeNode experimentNode = new DefaultMutableTreeNode(experiment);
			experiment.setDMTN(experimentNode);
			treeModel.insertNodeInto(experimentNode, selectedNode, selectedNode.getChildCount());
		    }
		    perfDMFSession.terminate();
		    System.out.println("Done loading experiment list.");
		}
		else{
		    System.out.println("Loading experiment list ...");
		    for(int i=selectedNode.getChildCount(); i>0; i--){
			treeModel.removeNodeFromParent(((DefaultMutableTreeNode) selectedNode.getChildAt(i-1)));
		    }
		    ListIterator l = application.getExperimentList();
		    while (l.hasNext()){
			ParaProfExperiment experiment = (ParaProfExperiment)l.next();
			DefaultMutableTreeNode experimentNode = new DefaultMutableTreeNode(experiment);
			experiment.setDMTN(experimentNode);
			treeModel.insertNodeInto(experimentNode, selectedNode, selectedNode.getChildCount());
		    }
		    System.out.println("Done loading experiment list.");
		}
		jSplitInnerPane.setRightComponent(getTable(userObject));
		jSplitInnerPane.setDividerLocation(0.5);
	    }
	    catch(Exception e){
		e.printStackTrace();
	    }
	}
	else if(userObject instanceof ParaProfExperiment){
	    ParaProfExperiment experiment = (ParaProfExperiment) userObject;
	    if(experiment.dBExperiment()){
		//Refresh the trials list.
		System.out.println("Loading trial list ...");
		for(int i=selectedNode.getChildCount(); i>0; i--){
		    treeModel.removeNodeFromParent(((DefaultMutableTreeNode) selectedNode.getChildAt(i-1)));
		}
		//Set the application and experiment.
		PerfDMFSession perfDMFSession = new PerfDMFSession(); 
		perfDMFSession.initialize(ParaProf.savedPreferences.getDatabaseConfigurationFile(), ParaProf.savedPreferences.getDatabasePassword());
		perfDMFSession.setApplication(experiment.getApplicationID());
		perfDMFSession.setExperiment(experiment.getID());
		ListIterator l = perfDMFSession.getTrialList();
		while (l.hasNext()){
		    ParaProfTrial trial = new ParaProfTrial((Trial)l.next(), 4);
		    trial.setExperiment(experiment);
		    trial.setDBTrial(true);
		    DefaultMutableTreeNode trialNode = new DefaultMutableTreeNode(trial);
		    trial.setDMTN(trialNode);
		    treeModel.insertNodeInto(trialNode, selectedNode, selectedNode.getChildCount());
		    trial.setTreePath(new TreePath(trialNode.getPath()));
		    //treeModel.insertNodeInto(trialNode, selectedNode, selectedNode.getChildCount());
		}
		perfDMFSession.terminate();
		System.out.println("Done loading trial list.");
	    }
	    else{
		System.out.println("Loading trial list ...");
		for(int i=selectedNode.getChildCount(); i>0; i--){
		    treeModel.removeNodeFromParent(((DefaultMutableTreeNode) selectedNode.getChildAt(i-1)));
		}
		ListIterator l = experiment.getTrialList();
		while (l.hasNext()){
		    ParaProfTrial trial = (ParaProfTrial)l.next();
		    DefaultMutableTreeNode trialNode = new DefaultMutableTreeNode(trial);
		    trial.setDMTN(trialNode);
		    treeModel.insertNodeInto(trialNode, selectedNode, selectedNode.getChildCount());
		    trial.setTreePath(new TreePath(trialNode.getPath()));
		}
		System.out.println("Done loading trial list.");
	    }
	    jSplitInnerPane.setRightComponent(getTable(userObject));
	    jSplitInnerPane.setDividerLocation(0.5);
	}
	else if(userObject instanceof ParaProfTrial){
	    ParaProfTrial trial = (ParaProfTrial) userObject;
	    if(trial.dBTrial()){
		//Test to see if trial has been loaded already.
		boolean loadedExists = false;
		for(Enumeration e = loadedTrials.elements(); e.hasMoreElements() ;){
		    ParaProfTrial loadedTrial = (ParaProfTrial) e.nextElement();
		    if((trial.getID()==loadedTrial.getID())&&
		       (trial.getExperimentID()==loadedTrial.getExperimentID())
		       &&(trial.getApplicationID()==loadedTrial.getApplicationID())){
			selectedNode.setUserObject(loadedTrial);
			trial = loadedTrial;
			loadedExists = true;
		    }
		}
		if(!loadedExists){
		    //Need to load the trial in from the db.
		    System.out.println("Loading trial ...");
		    PerfDMFSession perfDMFSession = new PerfDMFSession(); 
		    perfDMFSession.initialize(ParaProf.savedPreferences.getDatabaseConfigurationFile(), ParaProf.savedPreferences.getDatabasePassword());
		    perfDMFSession.setApplication(trial.getApplicationID());
		    perfDMFSession.setExperiment(trial.getExperimentID());
		    perfDMFSession.setTrial(trial.getID());
		    trial.initialize(perfDMFSession);
		    //Add to the list of loaded trials.
		    loadedTrials.add(trial);
		}
	    }
	    
	    //At this point, in both the db and non-db cases, the trial
	    //is either loading or not.  Check this before displaying.
	    if(!trial.loading()){
		System.out.println("Loading metric list ...");
		for(int i=selectedNode.getChildCount(); i>0; i--){
		    treeModel.removeNodeFromParent(((DefaultMutableTreeNode) selectedNode.getChildAt(i-1)));
		}
		ListIterator l = trial.getMetricList();
		while (l.hasNext()){
		    Metric metric = (Metric)l.next();
		    DefaultMutableTreeNode metricNode = new DefaultMutableTreeNode(metric, false);
		    
		    metric.setDMTN(metricNode);
		    treeModel.insertNodeInto(metricNode, selectedNode, selectedNode.getChildCount());
		}
		System.out.println("Done loading metric list.");
		
		jSplitInnerPane.setRightComponent(getTable(userObject));
		jSplitInnerPane.setDividerLocation(0.5);
	    }
	    else{
		jSplitInnerPane.setRightComponent(new JScrollPane(this.getLoadingTrialPanel(userObject)));
		jSplitInnerPane.setDividerLocation(0.5);
	    }
	}
    }
    //######
    //TreeWillExpandListener
    //######

    //####################################
    //End - Interface code.
    //####################################

    //####################################
    //Tree selection helpers.
    //####################################

    //Improve this function so that db trials can be passed through.
    private void metric(TreePath path, boolean show){
	DefaultMutableTreeNode selectedNode = (DefaultMutableTreeNode) path.getLastPathComponent();
	DefaultMutableTreeNode parentNode = (DefaultMutableTreeNode) selectedNode.getParent();            
	Object userObject = selectedNode.getUserObject();

	ParaProfTrial trial  =  (ParaProfTrial) parentNode.getUserObject();
	Metric metric = (Metric) userObject;
	jSplitInnerPane.setRightComponent(getTable(userObject));
	jSplitInnerPane.setDividerLocation(0.5);
	if(!trial.dBTrial()){
	    pPMLPanel.setArg2Field(pPMLPanel.getArg1Field());
	    pPMLPanel.setArg1Field(metric.getApplicationID()+":"+metric.getExperimentID()+":"+metric.getTrialID()+":"+metric.getID());
	}
	if(show)
	    this.showMetric(trial, metric);
    }

    public void clearDefaultMutableTreeNodes(DefaultMutableTreeNode defaultMutableTreeNode){
	//Do not want to clear this node's user object's DefaultMutableTreeNode so start the 
	//recursion on its children.
	int count = defaultMutableTreeNode.getChildCount();
	for(int i=0;i<count;i++)
	    clearDefaultMutableTreeNodesHelper((DefaultMutableTreeNode) defaultMutableTreeNode.getChildAt(i));
    }

    public void clearDefaultMutableTreeNodesHelper(DefaultMutableTreeNode defaultMutableTreeNode){
	int count = defaultMutableTreeNode.getChildCount();
	for(int i=0;i<count;i++)
	    this.clearDefaultMutableTreeNodesHelper((DefaultMutableTreeNode) defaultMutableTreeNode.getChildAt(i));
	((ParaProfTreeNodeUserObject)defaultMutableTreeNode.getUserObject()).clearDefaultMutableTreeNodes();
    }

    public int[] getSelectedDBExperiment(){
	if(ParaProf.savedPreferences.getDatabaseConfigurationFile()==null||ParaProf.savedPreferences.getDatabasePassword()==null){//Check to see if the user has set configuration information.
	    JOptionPane.showMessageDialog(this, "Please set the database configuration information (file menu).",
					  "DB Configuration Error!",
					  JOptionPane.ERROR_MESSAGE);
	    return null;
	}

	TreePath path = tree.getSelectionPath();
	boolean error = false;
	if(path == null)
	    error = true;
	else{
	    DefaultMutableTreeNode selectedNode = (DefaultMutableTreeNode) path.getLastPathComponent();
	    Object userObject = selectedNode.getUserObject();
	    if(userObject instanceof ParaProfExperiment){
		ParaProfExperiment paraProfExperiment = (ParaProfExperiment) userObject;
		if(paraProfExperiment.dBExperiment()){
		    int[] array = new int[2];
		    array[0] = paraProfExperiment.getApplicationID();
		    array[1] = paraProfExperiment.getID();
		    return array;
		}
		else
		    error = true;
	    }
	    else
		error = true;
	}
	if(error)
	    JOptionPane.showMessageDialog(this, "Please select an db experiment first!",
						      "DB Upload Error!",
						      JOptionPane.ERROR_MESSAGE);
	return null;
    }

    public int[] getSelectedDBTrial(){
	if(ParaProf.savedPreferences.getDatabaseConfigurationFile()==null||ParaProf.savedPreferences.getDatabasePassword()==null){//Check to see if the user has set configuration information.
	    JOptionPane.showMessageDialog(this, "Please set the database configuration information (file menu).",
					  "DB Configuration Error!",
					  JOptionPane.ERROR_MESSAGE);
	    return null;
	}

	TreePath path = tree.getSelectionPath();
	boolean error = false;
	if(path == null)
	    error = true;
	else{
	    DefaultMutableTreeNode selectedNode = (DefaultMutableTreeNode) path.getLastPathComponent();
	    Object userObject = selectedNode.getUserObject();
	    if(userObject instanceof ParaProfTrial){
		ParaProfTrial trial = (ParaProfTrial) userObject;
		if(trial.dBTrial()){
		    int[] array = new int[3];
		    array[0] = trial.getApplicationID();
		    array[1] = trial.getExperimentID();
		    array[2] = trial.getID();
		    return array;
		}
		else
		    error = true;
	    }
	    else
		error = true;
	}
	if(error)
	    JOptionPane.showMessageDialog(this, "Please select an db trial first!",
						      "DB Upload Error!",
						      JOptionPane.ERROR_MESSAGE);
	return null;
    }
    //####################################
    //End - Tree selection helpers.
    //####################################

    //####################################
    //Component functions.
    //####################################
    private Component getPanelHelpMessage(int type){
	JTextArea jTextArea = new JTextArea();
	jTextArea.setLineWrap(true);
	jTextArea.setWrapStyleWord(true);
	///Set the text.
	switch(type){
	case 0:
            jTextArea.append("ParaProf Manager\n\n");
            jTextArea.append("This window allows you to manage all of ParaProf's loaded data.\n");
            jTextArea.append("Data can be static (ie, not updated at runtime),"
                             + " and loaded either remotely or locally.  You can also specify data to be uploaded at runtime.\n\n");
            break;
	case 1:
	    jTextArea.append("ParaProf Manager\n\n");
	    jTextArea.append("This is the Standard application section:\n\n");
	    jTextArea.append("Standard - The classic ParaProf mode.  Data sets that are loaded at startup are placed"
			  + "under the default application automatically. Please see the ParaProf documentation for mre details.\n");
	    break;
	case 2:
	    jTextArea.append("ParaProf Manager\n\n");
	    jTextArea.append("This is the Runtime application section:\n\n");
	    jTextArea.append("Runtime - A new feature allowing ParaProf to update data at runtime.  Please see"
			  + " the ParaProf documentation if the options are not clear.\n");
	    jTextArea.append("*** THIS FEATURE IS CURRENTLY OFF ***\n");
	    break;
	case 3:
	    jTextArea.append("ParaProf Manager\n\n");
	    jTextArea.append("This is the DB Apps application section:\n\n");
	    jTextArea.append("DB Apps - Another new feature allowing ParaProf to load data from a database.  Again, please see"
			  + " the ParaProf documentation if the options are not clear.\n");
	    break;
	default:
	    break;
	}
	return (new JScrollPane(jTextArea));
    }

    private Component getTable(Object obj){
	return (new JScrollPane(new JTable(new ParaProfManagerTableModel(this, obj, treeModel))));}

    private Component getLoadingTrialPanel(Object obj){
	JPanel jPanel = new JPanel();
	GridBagLayout gbl = new GridBagLayout();
	jPanel.setLayout(gbl);
	GridBagConstraints gbc = new GridBagConstraints();
	gbc.insets = new Insets(0, 0, 0, 0);

	JLabel jLabel = new JLabel("Trial loading ... Please wait!");

 	gbc.fill = GridBagConstraints.NONE;
	gbc.anchor = GridBagConstraints.NORTH;
	gbc.weightx = 0;
	gbc.weighty = 0;
	gbc.gridx = 0;
	gbc.gridy = 0;
	gbc.gridwidth = 1;
	gbc.gridheight = 1;
	jPanel.add(this.getTable(obj),gbc);

	gbc.fill = GridBagConstraints.BOTH;
	gbc.anchor = GridBagConstraints.CENTER;
	gbc.weightx = 0;
	gbc.weighty = 0;
	gbc.gridx = 0;
	gbc.gridy = 1;
	gbc.gridwidth = 1;
	gbc.gridheight = 1;
	jPanel.add(jLabel,gbc);

	return jPanel;
    }

    //####################################
    //End - Component functions.
    //####################################

    public ParaProfApplication addApplication(boolean dBApplication, DefaultMutableTreeNode treeNode){
	ParaProfApplication application = null;
	if(dBApplication){
	    PerfDMFSession perfDMFSession = this.getDBSession();
	    if(perfDMFSession!=null){
		application = new ParaProfApplication();
		application.setDBApplication(true);
		application.setName("New Application");
		application.setID(perfDMFSession.saveApplication(application));
		perfDMFSession.terminate();
	    }
	}
	else{
	    application  = ParaProf.applicationManager.addApplication();
	    application.setName("New Application");
	}
	return application;
    }

    public ParaProfExperiment addExperiment(boolean dBExperiment, ParaProfApplication application){
	ParaProfExperiment experiment = null;
	if(dBExperiment){
	    PerfDMFSession perfDMFSession = this.getDBSession();
	    if(perfDMFSession!=null){
		experiment  = new ParaProfExperiment();
		experiment.setDBExperiment(true);
		experiment.setApplicationID(application.getID());
		experiment.setName("New Experiment");
		experiment.setID(perfDMFSession.saveExperiment(experiment));
		perfDMFSession.terminate();
		
	    }
	}
	else{
	    experiment = application.addExperiment();
	    experiment.setName("New Experiment");
	}
	return experiment;
    }

    public void addTrial(ParaProfApplication application, ParaProfExperiment experiment,
			 File location, String filePrefix, int type){
    	try{
	    ParaProfTrial trial = null;
	    FileList fl = new FileList();
	    Vector v = null;
	    if(type!=-1){
		switch(type){
		case 0:
		    if(filePrefix==null)
			v = fl.getFileList(location, null, type, "pprof", UtilFncs.debug);
		    else
			v = fl.getFileList(location, null, type, filePrefix, UtilFncs.debug);
		    break;
		case 1:
		    if(filePrefix==null)
			v = fl.getFileList(location, null, type, "profile", UtilFncs.debug);
		    else
			v = fl.getFileList(location, null, type, filePrefix, UtilFncs.debug);
		    break;
		case 2:
		    v = fl.getFileList(location, null, type, filePrefix, UtilFncs.debug);
		    break;
		case 5:
		    if(filePrefix==null)
			v = fl.getFileList(location, null, type, "gprof", UtilFncs.debug);
		    else
			v = fl.getFileList(location, null, type, filePrefix, UtilFncs.debug);
		    break;
		default:
		    v = new Vector();
		    System.out.println("Unrecognized file type.");
		    break;
		}
		if(v.size()>0){
		    trial = new ParaProfTrial(null, type);
		    if(experiment.dBExperiment()){
			trial.setUpload(true); //This trial is not set to a db trial until after it has finished loading.
		    }
		    else
			experiment.addTrial(trial);
		    trial.setExperiment(experiment);
		    trial.setApplicationID(experiment.getApplicationID());
		    trial.setExperimentID(experiment.getID());
		    trial.setPaths(fl.getPath());
		    trial.setName(trial.getPathReverse());
		    trial.setLoading(true);
		    trial.initialize(v);

		    
		    if(experiment.dBExperiment()) //Check need to occur on the experiment as trial not yet a recognized db trial.
			this.expandTrial(2,trial.getApplicationID(),trial.getExperimentID(),trial.getID(),
					 application,experiment,trial);
		    else
			this.expandTrial(0,trial.getApplicationID(),trial.getExperimentID(),trial.getID(),
					 application,experiment,trial);
		}
		else{
		    System.out.println("No profile files found in the selected directory.");
		}
	    }
	}
	catch(Exception e){
	    System.out.println("Error adding trial ... aborted.");
	    System.out.println("Location - ParaProfManager.addTrial(...)");
	    if(UtilFncs.debug)
		e.printStackTrace();
	    return;
	}
    }

    private void showMetric(ParaProfTrial trial, Metric metric){
	try{
	    trial.setSelectedMetricID(metric.getID());
	    trial.getSystemEvents().updateRegisteredObjects("dataEvent");
	    trial.showMainWindow();
	}
	catch(Exception e){
	    UtilFncs.systemError(e, null, "PPM04");
	}
    }

    public void populateTrialMetrics(ParaProfTrial trial){
	//Check to see if this trial needs to be uploaded to the database.
	if(trial.upload()){
	    System.out.println("Uploading trial: " + trial.getApplicationID() + "," +
			       trial.getExperimentID() + "," +
			       trial.getID() + " to the database. Please wait ...");
	    PerfDMFSession perfDMFSession = this.getDBSession();
	    if(perfDMFSession!=null){
		trial.setID(perfDMFSession.saveParaProfTrial(trial, -1));
		perfDMFSession.terminate();
	    }
	    trial.setUpload(false);
	    System.out.println("Done uploading trial: " + trial.getApplicationID() + "," +
			       trial.getExperimentID() + "," +
			       trial.getID());
	    //Add to the list of loaded trials.
	    loadedTrials.add(trial);
	    //Now safe to set this to be a dbTrial.
	    trial.setDBTrial(true);
	}

	if(trial.dBTrial())
	    this.expandTrial(2,trial.getApplicationID(),trial.getExperimentID(),trial.getID(),null,null,trial);
	else
	    this.expandTrial(0,trial.getApplicationID(),trial.getExperimentID(),trial.getID(),null,null,trial);
    }

    public DefaultMutableTreeNode expandApplicationType(int type, int applicationID, ParaProfApplication application){
	switch(type){
	case 0:
	    //Test to see if standard is expanded, if not, expand it.
	    if(!(tree.isExpanded(new TreePath(standard.getPath()))))
		tree.expandPath(new TreePath(standard.getPath()));
	    
	    //Try and find the required application node.
	    for(int i=standard.getChildCount(); i>0; i--){
		DefaultMutableTreeNode defaultMutableTreeNode = (DefaultMutableTreeNode) standard.getChildAt(i-1);
		if(applicationID==((ParaProfApplication)defaultMutableTreeNode.getUserObject()).getID())
		    return defaultMutableTreeNode;
	    }
	    //Required application node was not found, try adding it.
	    if(application!=null){
		    DefaultMutableTreeNode applicationNode = new DefaultMutableTreeNode(application);
		    application.setDMTN(applicationNode);
		    treeModel.insertNodeInto(applicationNode, standard, standard.getChildCount());
		    return applicationNode;
	    }
	    return null;
	case 1:
	    break;
	case 2:
	    //Test to see if dbApps is expanded, if not, expand it.
	    if(!(tree.isExpanded(new TreePath(dbApps.getPath()))))
		tree.expandPath(new TreePath(dbApps.getPath()));
	    
	    //Try and find the required application node.
	    for(int i=dbApps.getChildCount(); i>0; i--){
		DefaultMutableTreeNode defaultMutableTreeNode = (DefaultMutableTreeNode) dbApps.getChildAt(i-1);
		if(applicationID==((ParaProfApplication)defaultMutableTreeNode.getUserObject()).getID())
		    return defaultMutableTreeNode;
	    }
	    //Required application node was not found, try adding it.
	    if(application!=null){
		    DefaultMutableTreeNode applicationNode = new DefaultMutableTreeNode(application);
		    application.setDMTN(applicationNode);
		    treeModel.insertNodeInto(applicationNode, dbApps, dbApps.getChildCount());
		    return applicationNode;
	    }
	    return null;
	default:
	    break;
	}
	return null;
    }
    
    //Expands the given application
    public DefaultMutableTreeNode expandApplication(int type, int applicationID, int experimentID,
						   ParaProfApplication application, ParaProfExperiment experiment){
	DefaultMutableTreeNode applicationNode = this.expandApplicationType(type,applicationID,application);
	if(applicationNode!=null){
	    //Expand the application.
	    tree.expandPath(new TreePath(applicationNode.getPath()));

	    //Try and find the required experiment node.
	    tree.expandPath(new TreePath(standard.getPath()));
	    for(int i=applicationNode.getChildCount(); i>0; i--){
		DefaultMutableTreeNode defaultMutableTreeNode = (DefaultMutableTreeNode) applicationNode.getChildAt(i-1);
		if(experimentID==((ParaProfExperiment)defaultMutableTreeNode.getUserObject()).getID())
		    return defaultMutableTreeNode;
	    }
	    //Required experiment node was not found, try adding it.
	    if(experiment!=null){
		DefaultMutableTreeNode experimentNode = new DefaultMutableTreeNode(experiment);
		experiment.setDMTN(experimentNode);
		treeModel.insertNodeInto(experimentNode, applicationNode, applicationNode.getChildCount());
		return experimentNode;
	    }
	    return null;
	}
	return null;
    }

    public DefaultMutableTreeNode expandExperiment(int type, int applicationID, int experimentID, int trialID,
						   ParaProfApplication application, ParaProfExperiment experiment,
						   ParaProfTrial trial){
	DefaultMutableTreeNode experimentNode = this.expandApplication(type,applicationID,experimentID,application,experiment);
	if(experimentNode!=null){
	    //Expand the experiment.
	    tree.expandPath(new TreePath(experimentNode.getPath()));

	    //Try and find the required trial node.
	    for(int i=experimentNode.getChildCount(); i>0; i--){
		DefaultMutableTreeNode defaultMutableTreeNode = (DefaultMutableTreeNode) experimentNode.getChildAt(i-1);
		if(trialID==((ParaProfTrial)defaultMutableTreeNode.getUserObject()).getID())
		    return defaultMutableTreeNode;
	    }
	    //Required trial node was not found, try adding it.
	    if(trial!=null){
		DefaultMutableTreeNode trialNode = new DefaultMutableTreeNode(trial);
		trial.setDMTN(trialNode);
		treeModel.insertNodeInto(trialNode, experimentNode, experimentNode.getChildCount());
		return trialNode;
	    }
	    return null;
	}
	return null;
    }

    public void expandTrial(int type, int applicationID, int experimentID, int trialID,
					      ParaProfApplication application, ParaProfExperiment experiment, ParaProfTrial trial){
	DefaultMutableTreeNode trialNode = this.expandExperiment(type,applicationID,experimentID,trialID,
								 application,experiment,trial);
	//Expand the trial.
	if(trialNode!=null){
	    if(tree.isExpanded(new TreePath(trialNode.getPath())))
		tree.collapsePath(new TreePath(trialNode.getPath()));
	    tree.expandPath(new TreePath(trialNode.getPath()));
	}
    }

    public PerfDMFSession getDBSession(){
	//Check to see if the user has set configuration information.
	if(ParaProf.savedPreferences.getDatabaseConfigurationFile()==null
	   ||ParaProf.savedPreferences.getDatabasePassword()==null){
	    JOptionPane.showMessageDialog(this, "Please set the database configuration information (file menu).",
					  "DB Configuration Error!",
					  JOptionPane.ERROR_MESSAGE);
	    return null;
	}
	else{//Test to see if configurataion file exists.
	    File file = new File(ParaProf.savedPreferences.getDatabaseConfigurationFile());
	    if(!file.exists()){
		JOptionPane.showMessageDialog(this, "Specified configuration file does not exist.",
					      "DB Configuration Error!",
					      JOptionPane.ERROR_MESSAGE);
		return null;
	    }
	}
	//Basic checks done, try to access the db.
	PerfDMFSession perfDMFSession = new PerfDMFSession(); 
	perfDMFSession.initialize(ParaProf.savedPreferences.getDatabaseConfigurationFile(),
				  ParaProf.savedPreferences.getDatabasePassword());
	return perfDMFSession;
    }
    
    //Respond correctly when this window is closed.
    void thisWindowClosing(java.awt.event.WindowEvent e){
	closeThisWindow();
    }
    
    void closeThisWindow(){ 
	try{
	    if(UtilFncs.debug){
		System.out.println("------------------------");
		System.out.println("ParaProfExperiment List Manager Window is closing!");
		System.out.println("Clearing resourses for this window.");
	    }
	    setVisible(false);
	    dispose();
	}
	catch(Exception e){
	    UtilFncs.systemError(e, null, "ELM06");
	}
    }

    private void addCompItem(Component c, GridBagConstraints gbc, int x, int y, int w, int h){
	gbc.gridx = x;
	gbc.gridy = y;
	gbc.gridwidth = w;
	gbc.gridheight = h;
    	getContentPane().add(c, gbc);
    }
  
    //####################################
    //Instance Data.
    //####################################
    private JTree tree = null;
    private DefaultTreeModel treeModel = null;
    private DefaultMutableTreeNode standard = null;
    private DefaultMutableTreeNode runtime = null;
    private DefaultMutableTreeNode dbApps = null;
    private JSplitPane jSplitInnerPane = null;
    private JSplitPane jSplitOuterPane = null;
  
    //A reference to the default trial node.
    private DefaultMutableTreeNode defaultParaProfTrialNode = null;

    private JCheckBoxMenuItem showApplyOperationItem = null;
    private PPMLPanel pPMLPanel = new PPMLPanel(this);

    private Vector loadedTrials = new Vector();

    //######
    //Popup menu stuff.
    //######
    private JPopupMenu popup1 = new JPopupMenu();
    private JPopupMenu popup2 = new JPopupMenu();
    private JPopupMenu popup3 = new JPopupMenu();
    private JPopupMenu popup4 = new JPopupMenu();
    private JPopupMenu popup5 = new JPopupMenu();

    private Object clickedOnObject = null;
    //######
    //End - Popup menu stuff.
    //######
    //####################################
    //End - Instance Data.
    //####################################
}
