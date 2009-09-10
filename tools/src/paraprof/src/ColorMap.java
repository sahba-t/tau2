/*
 * Created on Mar 4, 2005
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
package edu.uoregon.tau.paraprof;

import java.awt.Color;
import java.awt.Component;
import java.io.Serializable;
import java.util.*;

import javax.swing.JColorChooser;

import edu.uoregon.tau.perfdmf.Function;

public class ColorMap extends Observable implements Serializable {

    private Map colors = new HashMap();

    public Map getMap() {
        return colors;
    }

    public void setMap(Map map) {
        if (map == null) {
            colors.clear();
        } else {
            colors = map;
            if (ParaProf.paraProfManagerWindow != null) {
                Vector trials = ParaProf.paraProfManagerWindow.getLoadedTrials();
                for (Iterator it = trials.iterator(); it.hasNext();) {
                    ParaProfTrial ppTrial = (ParaProfTrial) it.next();
                    ParaProf.colorChooser.setColors(ppTrial, -1);
                    ppTrial.updateRegisteredObjects("colorEvent");
                }
                setChanged();
                notifyObservers("colorMap");
            }
        }
    }

    public Color getColor(Function f) {
        return (Color) colors.get(f.getName());
    }

    public Color getColor(String functionName) {
        return (Color) colors.get(functionName);
    }

    void putColor(Function f, Color c) {
        colors.put(f.getName(), c);
        reassignColors();
        setChanged();
        notifyObservers("colorMap");
    }

    void removeColor(Function f) {
        removeColor(f.getName());
    }

    void removeColor(String functionName) {
        colors.remove(functionName);
        reassignColors();
        setChanged();
        notifyObservers("colorMap");
    }

    void assignColor(Component component, Function f) {
        Color color = f.getColor();
        color = JColorChooser.showDialog(component, "Please select a new color", color);
        if (color != null) {
            this.putColor(f, color);
        }
    }

    public Iterator getFunctions() {
        return colors.keySet().iterator();
    }

    public void removeAll() {
        colors.clear();
        reassignColors();
        setChanged();
        notifyObservers("colorMap");
    }

    public void reassignColors() {
        Vector trials = ParaProf.paraProfManagerWindow.getLoadedTrials();
        for (Iterator it = trials.iterator(); it.hasNext();) {
            ParaProfTrial ppTrial = (ParaProfTrial) it.next();
            ParaProf.colorChooser.setColors(ppTrial, -1);
            ppTrial.updateRegisteredObjects("colorEvent");
        }

    }

    public void assignColorsFromTrial(ParaProfTrial ppTrial) {
        for (Iterator it = ppTrial.getDataSource().getFunctions(); it.hasNext();) {
            Function f = (Function) it.next();
            colors.put(f.getName(), f.getColor());
        }

        reassignColors();
        setChanged();
        notifyObservers("colorMap");
    }

    public void showColorMap(Component invoker) {
        ColorMapWindow colorMapWindow = new ColorMapWindow(invoker);
        colorMapWindow.setVisible(true);
        this.addObserver(colorMapWindow);
    }

}
