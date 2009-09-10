package edu.uoregon.tau.perfdmf.loader;

import jargs.gnu.CmdLineParser;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;

import edu.uoregon.tau.perfdmf.DataSource;
import edu.uoregon.tau.perfdmf.DataSourceException;
import edu.uoregon.tau.perfdmf.DatabaseAPI;
import edu.uoregon.tau.perfdmf.DatabaseException;
import edu.uoregon.tau.perfdmf.Experiment;
import edu.uoregon.tau.perfdmf.Trial;
import edu.uoregon.tau.perfdmf.UtilFncs;

public class LoadTrial {

    private String sourceFiles[];
    private String metadataFile;
    private Experiment exp;
    private boolean fixNames;
    private boolean summaryOnly;
    private int expID;
    public int trialID;
    private DataSource dataSource;
    public String trialName;
    public String problemFile;
    public String configuration;
    public boolean terminate = true;

    private DatabaseAPI databaseAPI;
    private Trial trial;

    public static void usage() {
        System.err.println("Usage: perfdmf_loadtrial -a <appName> -x <expName> -n <name> [options] <files>\n\n"
                + "try `perfdmf_loadtrial --help' for more information");
    }

    public static void outputHelp() {

        System.err.println("Usage: perfdmf_loadtrial -a <appName> -x <expName> -n <name> [options] <files>\n\n"
                + "Required Arguments:\n\n" + "  -n, --name <text>               Specify the name of the trial\n"
                + "  -a, --applicationname <string>  Specify associated application name\n"
                + "                                    for this trial\n"
                + "  -x, --experimentname <string>   Specify associated experiment name\n"
                + "                                    for this trial\n" + "               ...or...\n\n"
                + "  -n, --name <text>               Specify the name of the trial\n"
                + "  -e, --experimentid <number>     Specify associated experiment ID\n"
                + "                                    for this trial\n" + "\n" + "Optional Arguments:\n\n"
                + "  -c, --config <name>             Specify the name of the configuration to use\n"
                + "  -g, --configFile <file>         Specify the configuration file to use\n"
                + "                                    (overrides -c)\n"
                + "  -f, --filetype <filetype>       Specify type of performance data, options are:\n"
                + "                                    profiles (default), pprof, dynaprof, mpip,\n"
                + "                                    gprof, psrun, hpm, packed, cube, hpc, ompp,\n"
                + "                                    snap, perixml, gptl, paraver, ipm\n"
                + "  -t, --trialid <number>          Specify trial ID\n"
                + "  -i, --fixnames                  Use the fixnames option for gprof\n"
                + "  -m, --metadata <filename>       XML metadata for the trial\n\n" + "Notes:\n"
                + "  For the TAU profiles type, you can specify either a specific set of profile\n"
                + "files on the commandline, or you can specify a directory (by default the current\n"
                + "directory).  The specified directory will be searched for profile.*.*.* files,\n"
                + "or, in the case of multiple counters, directories named MULTI_* containing\n" + "profile data.\n\n"
                + "Examples:\n\n" + "  perfdmf_loadtrial -e 12 -n \"Batch 001\"\n"
                + "    This will load profile.* (or multiple counters directories MULTI_*) into\n"
                + "    experiment 12 and give the trial the name \"Batch 001\"\n\n"
                + "  perfdmf_loadtrial -e 12 -n \"HPM data 01\" -f hpm perfhpm*\n"
                + "    This will load perfhpm* files of type HPMToolkit into experiment 12 and give\n"
                + "    the trial the name \"HPM data 01\"\n\n"
                + "  perfdmf_loadtrial -a \"NPB2.3\" -x \"parametric\" -n \"64\" par64.ppk\n"
                + "    This will load packed profile par64.ppk into the experiment named\n"
                + "    \"parametric\" under the application named \"NPB2.3\" and give the trial\n"
                + "    the name \"64\".  The application and experiment will be created if not found.\n");
    }

    public LoadTrial(String configFileName, String sourceFiles[]) {
        this.sourceFiles = sourceFiles;

        databaseAPI = new DatabaseAPI();
        try {
            databaseAPI.initialize(configFileName, true);
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(-1);
        }

    }

    public boolean checkForExp(String expid, String appName, String expName) {
        if (expid != null) {
            this.expID = Integer.parseInt(expid);

            try {
                exp = databaseAPI.setExperiment(this.expID);
            } catch (Exception e) {}
            if (exp == null) {
                System.err.println("Experiment id " + expid + " not found,  please enter a valid experiment ID.");
                System.exit(-1);
                return false;
            } else {
                return true;
            }
        } else {
            Experiment exp = databaseAPI.getExperiment(appName, expName, true);
            this.expID = exp.getID();
            return true;
        }
    }

    public boolean checkForTrialName(String trialName) {
        Trial tmpTrial = databaseAPI.setTrial(trialName,false);
        if (tmpTrial == null)
            return false;
        else
            return true;
    }

    public boolean checkForTrial(String trialid) {
        Trial tmpTrial = databaseAPI.setTrial(Integer.parseInt(trialid),false);
        if (tmpTrial == null)
            return false;
        else
            return true;
    }

    public void loadTrial(int fileType) {

        File[] files = new File[sourceFiles.length];
        for (int i = 0; i < sourceFiles.length; i++) {
            files[i] = new File(sourceFiles[i]);
        }

        try {
            dataSource = UtilFncs.initializeDataSource(files, fileType, fixNames);
        } catch (DataSourceException e) {
            e.printStackTrace();
            System.err.println("Error: Unable to initialize datasource!");
            return;
        }

        trial = new Trial();
        trial.setDataSource(dataSource);

        // set the metadata file name before loading the data, because
        // aggregateData() is called at the end of the dataSource.load()
        // and this file has to be set before then.
        try {
            if (metadataFile != null) {
                dataSource.setMetadataFile(metadataFile);
            }
        } catch (Exception e) {
            System.err.println("Error Loading metadata:");
            e.printStackTrace();
            System.exit(1);
        }

        try {
            dataSource.load();
        } catch (Exception e) {
            System.err.println("Error Loading Trial:");
            e.printStackTrace();
        }

        // set the meta data from the datasource
	trial.setMetaData(dataSource.getMetaData());
	trial.setUncommonMetaData(dataSource.getUncommonMetaData());

        if (trialID == 0) {
            saveTrial();
        } else {
            appendToTrial();
        }
        if (this.terminate) {
            databaseAPI.terminate();
        }
    }

    public void saveTrial() {
        trial.setName(trialName);

        System.err.println("TrialName: " + trialName);
        trial.setExperimentID(expID);
        try {
            trialID = databaseAPI.uploadTrial(trial, summaryOnly);
        } catch (DatabaseException e) {
            e.printStackTrace();
            Exception e2 = e.getException();
            System.err.println("from: ");
            e2.printStackTrace();
            System.exit(-1);
        }
        System.err.println("Done saving trial!");
    }

    public void appendToTrial() {
        // set some things in the trial
        trial.setID(this.trialID);
        try {
            databaseAPI.saveTrial(trial, null);
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(-1);
        }
        System.err.println("Done adding metric to trial!");
    }

    public String getProblemString() {
        // if the file wasn't passed in, this is an existing trial.
        if (problemFile == null)
            return new String("");

        // open the file
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(problemFile));
        } catch (Exception e) {
            System.err.println("Problem file not found!  Exiting...");
            System.exit(0);
        }
        // read the file, one line at a time, and do some string
        // substitution to make sure that we don't blow up our
        // SQL statement. ' characters aren't allowed...
        StringBuffer problemString = new StringBuffer();
        String line;
        while (true) {
            try {
                line = reader.readLine();
            } catch (Exception e) {
                line = null;
            }
            if (line == null)
                break;
            problemString.append(line.replaceAll("'", "\'"));
        }

        // close the problem file
        try {
            reader.close();
        } catch (Exception e) {}

        // return the string
        return problemString.toString();
    }

    static public void main(String[] args) {
        // 	for (int i=0; i<args.length; i++) {
        // 	    System.out.println ("args[" + i + "]: " + args[i]);
        // 	}

        CmdLineParser parser = new CmdLineParser();
        CmdLineParser.Option helpOpt = parser.addBooleanOption('h', "help");
        CmdLineParser.Option experimentidOpt = parser.addStringOption('e', "experimentid");
        CmdLineParser.Option nameOpt = parser.addStringOption('n', "name");
        //CmdLineParser.Option problemOpt = parser.addStringOption('p',
        // "problemfile");
        CmdLineParser.Option configOpt = parser.addStringOption('c', "config");
        CmdLineParser.Option configFileOpt = parser.addStringOption('g', "configFile");
        CmdLineParser.Option trialOpt = parser.addStringOption('t', "trialid");
        CmdLineParser.Option typeOpt = parser.addStringOption('f', "filetype");
        CmdLineParser.Option fixOpt = parser.addBooleanOption('i', "fixnames");
        CmdLineParser.Option metadataOpt = parser.addStringOption('m', "metadata");
        CmdLineParser.Option appNameOpt = parser.addStringOption('a', "applicationname");
        CmdLineParser.Option expNameOpt = parser.addStringOption('x', "experimentname");
        CmdLineParser.Option summaryOpt = parser.addBooleanOption('s', "summaryonly");

        try {
            parser.parse(args);
        } catch (CmdLineParser.OptionException e) {
            System.err.println(e.getMessage());
            LoadTrial.usage();
            System.exit(-1);
        }

        Boolean help = (Boolean) parser.getOptionValue(helpOpt);
        //String sourceFile = (String)parser.getOptionValue(sourcefileOpt);
        String configName = (String) parser.getOptionValue(configOpt);
        String configFile = (String) parser.getOptionValue(configFileOpt);
        String experimentID = (String) parser.getOptionValue(experimentidOpt);
        String trialName = (String) parser.getOptionValue(nameOpt);
        String appName = (String) parser.getOptionValue(appNameOpt);
        String expName = (String) parser.getOptionValue(expNameOpt);

        //String problemFile = (String)parser.getOptionValue(problemOpt);
        String trialID = (String) parser.getOptionValue(trialOpt);
        String fileTypeString = (String) parser.getOptionValue(typeOpt);
        Boolean fixNames = (Boolean) parser.getOptionValue(fixOpt);
        String metadataFile = (String) parser.getOptionValue(metadataOpt);
        Boolean summaryOnly = (Boolean) parser.getOptionValue(summaryOpt);

        if (help != null && help.booleanValue()) {
            LoadTrial.outputHelp();
            System.exit(-1);
        }

        if (configFile == null) {
            if (configName == null)
                configFile = System.getProperty("user.home") + "/.ParaProf/perfdmf.cfg";
            else
                configFile = System.getProperty("user.home") + "/.ParaProf/perfdmf.cfg." + configName;
        }

        if (trialName == null) {
            System.err.println("Error: Missing trial name\n");
            LoadTrial.usage();
            System.exit(-1);
        } else if (experimentID == null && expName == null) {
            System.err.println("Error: Missing experiment id or name\n");
            LoadTrial.usage();
            System.exit(-1);
        } else if (expName != null && appName == null) {
            System.err.println("Error: Missing application name\n");
            LoadTrial.usage();
            System.exit(-1);
        }

        String sourceFiles[] = parser.getRemainingArgs();

        int fileType = DataSource.TAUPROFILE;
        if (fileTypeString != null) {
            if (fileTypeString.equals("profiles")) {
                fileType = DataSource.TAUPROFILE;
            } else if (fileTypeString.equals("pprof")) {
                fileType = DataSource.PPROF;
            } else if (fileTypeString.equals("dynaprof")) {
                fileType = DataSource.DYNAPROF;
            } else if (fileTypeString.equals("mpip")) {
                fileType = DataSource.MPIP;
            } else if (fileTypeString.equals("hpm")) {
                fileType = DataSource.HPM;
            } else if (fileTypeString.equals("gprof")) {
                fileType = DataSource.GPROF;
            } else if (fileTypeString.equals("psrun")) {
                fileType = DataSource.PSRUN;
            } else if (fileTypeString.equals("packed")) {
                fileType = DataSource.PPK;
            } else if (fileTypeString.equals("cube")) {
                fileType = DataSource.CUBE;
            } else if (fileTypeString.equals("hpc")) {
                fileType = DataSource.HPCTOOLKIT;
            } else if (fileTypeString.equals("gyro")) {
                fileType = DataSource.GYRO;
            } else if (fileTypeString.equals("gamess")) {
                fileType = DataSource.GAMESS;
            } else if (fileTypeString.equals("gptl")) {
                fileType = DataSource.GPTL;
            } else if (fileTypeString.equals("ipm")) {
                fileType = DataSource.IPM;
            } else if (fileTypeString.equals("ompp")) {
                fileType = DataSource.OMPP;
            } else if (fileTypeString.equals("snap")) {
                fileType = DataSource.SNAP;
            } else if (fileTypeString.equals("paraver")) {
                fileType = DataSource.PARAVER;
            } else if (fileTypeString.equals("perixml")) {
                fileType = DataSource.PERIXML;
            } else {
                System.err.println("Please enter a valid file type.");
                LoadTrial.usage();
                System.exit(-1);
            }
        } else {
            if (sourceFiles.length >= 1) {
                fileType = UtilFncs.identifyData(new File(sourceFiles[0]));
            }
        }


        if (trialName == null) {
            trialName = "";
        }

        if (fixNames == null) {
            fixNames = new Boolean(false);
        }
        if (summaryOnly == null) {
            summaryOnly = new Boolean(false);
        }
        LoadTrial trans = new LoadTrial(configFile, sourceFiles);
        trans.checkForExp(experimentID, appName, expName);
        if (trialID != null) {
            trans.checkForTrial(trialID);
            trans.trialID = Integer.parseInt(trialID);
        }

        trans.trialName = trialName;
        //trans.problemFile = problemFile;
        trans.fixNames = fixNames.booleanValue();
        trans.metadataFile = metadataFile;
        trans.summaryOnly = summaryOnly.booleanValue();
        trans.loadTrial(fileType);
        // the trial will be saved when the load is finished (update is called)
    }

    public void setMetadataFile(String metadataFile) {
        this.metadataFile = metadataFile;
    }

    public void setFixNames(boolean fixNames) {
        this.fixNames = fixNames;
    }

    public void setSummaryOnly(boolean summaryOnly) {
        this.summaryOnly = summaryOnly;
    }

    public DatabaseAPI getDatabaseAPI() {
        return databaseAPI;
    }

}
