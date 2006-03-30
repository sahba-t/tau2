package edu.uoregon.tau.paraprof.enums;

import edu.uoregon.tau.paraprof.*;
import edu.uoregon.tau.perfdmf.*;

/**
 * type-safe enum pattern for type of Valueing
 *    
 * TODO : nothing, this class is complete
 *
 * <P>CVS $Id: ValueType.java,v 1.4 2006/03/30 03:03:54 amorris Exp $</P>
 * @author  Alan Morris
 * @version $Revision: 1.4 $
 */

public abstract class ValueType {

    private final String name;
    private final String classname;
    
    public abstract double getValue(FunctionProfile functionProfile, int metric);

    public abstract double getThreadMaxValue(edu.uoregon.tau.perfdmf.Thread thread, int metric);

    public abstract String getSuffix(int units, Metric metric);

    public abstract int getUnits(int units, Metric metric);

    private ValueType(String name, String classname) {
        this.name = name;
        this.classname = classname;
    }

    public String toString() {
        return name;
    }
    public String getClassName() {
        return classname;
    }

    private static String timeUnits(int units) {
        String string;
        switch (units) {
        case 0:
            string = " microseconds";
            break;
        case 1:
            string = " milliseconds";
            break;
        case 2:
            string = " seconds";
            break;
        case 3:
            string = " hh:mm:ss";
            break;

        default:
            throw new ParaProfException("Unexpected unit type: " + units);
        }
        return string;
    }

    public static boolean isTimeUnits(ValueType value) {
        if (value == EXCLUSIVE || value == INCLUSIVE || value == INCLUSIVE_PER_CALL || value == EXCLUSIVE_PER_CALL) {
            return true;
        }
        return false;
    }

    public static final ValueType EXCLUSIVE = new ValueType("Exclusive", "ValueType.EXCLUSIVE") {

        public double getValue(FunctionProfile functionProfile, int metric) {
            if (functionProfile.getFunction().isPhase()) {
                return functionProfile.getInclusive(metric);
            } else {
                return functionProfile.getExclusive(metric);
            }
        }

        public double getThreadMaxValue(edu.uoregon.tau.perfdmf.Thread thread, int metric) {
            return thread.getMaxExclusive(metric);
        }

        public String getSuffix(int units, Metric metric) {
            if (!metric.isTimeMetric())
                return " counts";
            return timeUnits(units);
        }

        public int getUnits(int units, Metric metric) {
            if (metric.isTimeMetric())
                return units;
            return 0;
        }
    };

    public static final ValueType EXCLUSIVE_PERCENT = new ValueType("Exclusive percent", "ValueType.EXCLUSIVE_PERCENT") {
        public double getValue(FunctionProfile functionProfile, int metric) {
            if (functionProfile.getFunction().isPhase()) {
                return functionProfile.getInclusivePercent(metric);
            } else {
                return functionProfile.getExclusivePercent(metric);
            }
        }

        public double getThreadMaxValue(edu.uoregon.tau.perfdmf.Thread thread, int metric) {
            return thread.getMaxExclusivePercent(metric);
        }

        public String getSuffix(int units, Metric metric) {
            return " %";
        }

        public int getUnits(int units, Metric metric) {
            return 0;
        }
    };

    public static final ValueType INCLUSIVE = new ValueType("Inclusive", "ValueType.INCLUSIVE") {
        public double getValue(FunctionProfile functionProfile, int metric) {
            return functionProfile.getInclusive(metric);
        }

        public double getThreadMaxValue(edu.uoregon.tau.perfdmf.Thread thread, int metric) {
            return thread.getMaxInclusive(metric);
        }

        public String getSuffix(int units, Metric metric) {
            if (!metric.isTimeMetric())
                return " counts";
            return timeUnits(units);
        }

        public int getUnits(int units, Metric metric) {
            if (metric.isTimeMetric())
                return units;
            return 0;
        }
    };

    public static final ValueType INCLUSIVE_PERCENT = new ValueType("Inclusive percent", "ValueType.INCLUSIVE_PERCENT") {
        public double getValue(FunctionProfile functionProfile, int metric) {
            return functionProfile.getInclusivePercent(metric);
        }

        public double getThreadMaxValue(edu.uoregon.tau.perfdmf.Thread thread, int metric) {
            return thread.getMaxInclusivePercent(metric);
        }

        public String getSuffix(int units, Metric metric) {
            return " %";
        }
        public int getUnits(int units, Metric metric) {
            
            return 0;
        }
    };

    public static final ValueType NUMCALLS = new ValueType("Number of Calls", "ValueType.NUMCALLS") {
        public double getValue(FunctionProfile functionProfile, int metric) {
            return functionProfile.getNumCalls();
        }

        public double getThreadMaxValue(edu.uoregon.tau.perfdmf.Thread thread, int metric) {
            return thread.getMaxNumCalls();
        }

        public String getSuffix(int units, Metric metric) {
            return " calls";
        }

        public int getUnits(int units, Metric metric) {
            return 0;
        }
    };

    public static final ValueType NUMSUBR = new ValueType("Number of Child Calls", "ValueType.NUMSUBR") {
        public double getValue(FunctionProfile functionProfile, int metric) {
            return functionProfile.getNumSubr();
        }

        public double getThreadMaxValue(edu.uoregon.tau.perfdmf.Thread thread, int metric) {
            return thread.getMaxNumSubr();
        }

        public String getSuffix(int units, Metric metric) {
            return " calls";
        }

        public int getUnits(int units, Metric metric) {
            return 0;
        }
    };

    public static final ValueType INCLUSIVE_PER_CALL = new ValueType("Inclusive per Call", "ValueType.INCLUSIVE_PER_CALL") {
        public double getValue(FunctionProfile functionProfile, int metric) {
            return functionProfile.getInclusivePerCall(metric);
        }

        public double getThreadMaxValue(edu.uoregon.tau.perfdmf.Thread thread, int metric) {
            return thread.getMaxInclusivePerCall(metric);
        }

        public String getSuffix(int units, Metric metric) {
            if (!metric.isTimeMetric())
                return " counts per call";
            return timeUnits(units) + " per call";
        }

        public int getUnits(int units, Metric metric) {
            if (metric.isTimeMetric())
                return units;
            return 0;
        }
    };

    public static final ValueType EXCLUSIVE_PER_CALL = new ValueType("Exclusive per Call", "ValueType.EXCLUSIVE_PER_CALL") {
        public double getValue(FunctionProfile functionProfile, int metric) {
            return functionProfile.getExclusivePerCall(metric);
        }

        public double getThreadMaxValue(edu.uoregon.tau.perfdmf.Thread thread, int metric) {
            return thread.getMaxExclusivePerCall(metric);
        }

        public String getSuffix(int units, Metric metric) {
            if (!metric.isTimeMetric())
                return " counts per call";
            return timeUnits(units) + " per call";
        }

        public int getUnits(int units, Metric metric) {
            if (metric.isTimeMetric())
                return units;
            return 0;
        }
    };

    public static ValueType[] VALUES = { EXCLUSIVE, INCLUSIVE, EXCLUSIVE_PERCENT, INCLUSIVE_PERCENT, NUMCALLS, NUMSUBR,
            INCLUSIVE_PER_CALL, EXCLUSIVE_PER_CALL };

}
