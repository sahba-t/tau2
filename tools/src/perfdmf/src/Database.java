package edu.uoregon.tau.perfdmf;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import edu.uoregon.tau.perfdmf.database.ParseConfig;

public class Database {
    private String name;
    private int id;
    private ParseConfig config;

    private static int idCounter;

    private String appFieldNames[];
    private int appFieldTypes[];
    private String expFieldNames[];
    private int expFieldTypes[];
    private String trialFieldNames[];
    private int trialFieldTypes[];

    public String[] getAppFieldNames() {
        return appFieldNames;
    }

    public void setAppFieldNames(String[] appFieldNames) {
        this.appFieldNames = appFieldNames;
    }

    public int[] getAppFieldTypes() {
        return appFieldTypes;
    }

    public void setAppFieldTypes(int[] appFieldTypes) {
        this.appFieldTypes = appFieldTypes;
    }

    public String[] getExpFieldNames() {
        return expFieldNames;
    }

    public void setExpFieldNames(String[] expFieldNames) {
        this.expFieldNames = expFieldNames;
    }

    public int[] getExpFieldTypes() {
        return expFieldTypes;
    }

    public void setExpFieldTypes(int[] expFieldTypes) {
        this.expFieldTypes = expFieldTypes;
    }

    public String[] getTrialFieldNames() {
        return trialFieldNames;
    }

    public void setTrialFieldNames(String[] trialFieldNames) {
        this.trialFieldNames = trialFieldNames;
    }

    public int[] getTrialFieldTypes() {
        return trialFieldTypes;
    }

    public void setTrialFieldTypes(int[] trialFieldTypes) {
        this.trialFieldTypes = trialFieldTypes;
    }

    public Database(String name, ParseConfig config) {
        this.name = name;
        this.id = idCounter;
        idCounter++;
        this.config = config;
    }

    public Database(String name, String configFilename) {
        this(name, new ParseConfig(configFilename));
    }

    public Database(String configFilename) {
        this("???", new ParseConfig(configFilename));
    }

    public int getID() {
        return id;
    }

    public ParseConfig getConfig() {
        return config;
    }

    private static Database createDatabase(String name, String configFile) {
        ParseConfig config = new ParseConfig(configFile.toString());
        Database database = new Database(name, config);
        return database;
    }

    public static List getDatabases() {
        File paraprofDirectory = new File(System.getProperty("user.home") + "/.ParaProf");
        String[] fileNames = paraprofDirectory.list();
        List perfdmfConfigs = new ArrayList();
        for (int i = 0; i < fileNames.length; i++) {
            if (fileNames[i].compareTo("perfdmf.cfg") == 0) {
                perfdmfConfigs.add(createDatabase("Default", paraprofDirectory + "/" + fileNames[i]));
            } else if (fileNames[i].startsWith("perfdmf.cfg") && !fileNames[i].endsWith("~")) {
                String name = fileNames[i].substring(12);
                perfdmfConfigs.add(createDatabase(name, paraprofDirectory + "/" + fileNames[i]));
            }
        }
        return perfdmfConfigs;
    }

    public String toString() {
        String dbDisplayName;
        dbDisplayName = config.getConnectionString();
        if (dbDisplayName.compareTo("") == 0) {
            dbDisplayName = "none";
        }
        //return "DB - " + name + " (" + dbDisplayName + ")"; 
        return name + " (" + dbDisplayName + ")";
    }

}
