#ifndef TAUDB_STRUCTS_H
#define TAUDB_STRUCTS_H 1

#include "time.h"
#include "uthash.h"

#ifndef boolean
#define TRUE  1
#define FALSE 0
typedef int boolean;
#endif

/* forward declarations to ease objects that need to know about each other 
 * and have doubly-linked relationships */

struct taudb_timer_call_data;
struct taudb_timer_value;
struct taudb_timer_callpath;
struct taudb_timer_group;
struct taudb_timer_parameter;
struct taudb_timer;
struct taudb_counter_value;
struct taudb_counter;
struct taudb_primary_metadata;
struct taudb_secondary_metadata;
struct taudb_time_range;
struct taudb_thread;
struct taudb_metric;
struct taudb_trial;
struct perfdmf_experiment;
struct perfdmf_application;

typedef struct taudb_configuration {
  char* jdbc_db_type;  // to identify DBMS vendor. postgresql, mysql, h2, derby, etc.
  char* db_hostname;   // server host name
  char* db_portnum;    // server port number
  char* db_dbname;     // the database name at the server
  char* db_schemaprefix;  // the schema prefix. This is appended to all table names for some DBMSs
  char* db_username;   // the database username
  char* db_password;   // the database password for username
  char* db_schemafile; // full or relative path to the schema file, used for configuration, not used in C API
} TAUDB_CONFIGURATION;

enum taudb_database_schema_version {
  TAUDB_2005_SCHEMA,
  TAUDB_2012_SCHEMA
};

typedef struct taudb_data_source {
 int id;
 char* name;
 char*description;
} TAUDB_DATA_SOURCE;

/* these are the derived thread indexes. */

static int TAUDB_MEAN_WITHOUT_NULLS = -1;
static int TAUDB_TOTAL = -2;
static int TAUDB_STDDEV_WITHOUT_NULLS = -3;
static int TAUDB_MIN = -4;
static int TAUDB_MAX = -5;
static int TAUDB_MEAN_WITH_NULLS = -6;
static int TAUDB_STDDEV_WITH_NULLS = -7;

/* trials are the top level structure */

typedef struct taudb_trial {
 // actual data from the database
 int id;
 char* name;
 struct taudb_data_source* data_source;
 int node_count;             // i.e. number of processes.
 int contexts_per_node;      // rarely used, usually 1.
 int threads_per_context;    // max number of threads per process (can be less on individual processes)
 int total_threads;          // total number of threads
 // array sizes
 int metric_count;           // how many metrics were collected. Usually 1 (Time), can be hardware counters.
 int time_range_count;       // how many time_ranges were collected.
 int thread_count;           // TOTAL number of threads. helpful to know - can be less than nodes*threads_per_node.
 int derived_thread_count;   // number of derived threads. There should be 7, with negative thread indexes.
 int timer_count;            // number of timers. not all timers seen or collected on all threads.
 int timer_group_count;      // number of timer groups. examples include TAU_USER, TAU_DEFAULT, MPI, IO, etc.
 int timer_callpath_count;   // how many nodes are there in the combined callpath tree?
 int timer_call_data_count;  // should be less than timer_callpath_count * thread_count (not all timers seen on all threads)
 int counter_count;          // how many counters did we collect?
 int counter_value_count;    // should be the same as counter_count * thread_count
 int primary_metadata_count; // primary metadata fields
 int secondary_metadata_count; // secondary metadata fields
 // arrays of data for this trial
 struct taudb_metric* metrics;
 struct taudb_thread* threads;
 struct taudb_time_range* time_ranges;
 struct taudb_timer* timers;
 struct taudb_timer_group* timer_groups;
 struct taudb_timer_callpath* timer_callpaths;
 struct taudb_timer_call_data* timer_call_data;
 struct taudb_counter* counters;
 struct taudb_counter_value* counter_values;
 struct taudb_primary_metadata* primary_metadata;
 struct taudb_secondary_metadata* secondary_metadata;
} TAUDB_TRIAL;

/*********************************************/
/* data dimensions */
/*********************************************/

/* thread represents one physical & logical location for a measurement. */

typedef struct taudb_thread {
 int id; // database id, also key to hash
 struct taudb_trial* trial;
 int node_rank;    // which process does this thread belong to?
 int context_rank; // which context? USUALLY 0
 int thread_rank;  // what is this thread's rank in the process
 int index;        // what is this threads OVERALL index? ranges from 0 to trial.thread_count-1
 int secondary_metadata_count;
 struct taudb_secondary_metadata* secondary_metadata;
 UT_hash_handle hh;
} TAUDB_THREAD;

/* metrics are things like TIME, PAPI counters, and derived metrics. */

typedef struct taudb_metric {
 int id; // database value, also key to hash
 char* name;
 boolean derived;  // was this metric measured, or created by a post-processing tool?
 UT_hash_handle hh;
} TAUDB_METRIC;

/* Time ranges are ways to delimit the profile data within time ranges.
   They are also useful for secondary metadata which is associated with
   a specific call to a function. */

typedef struct taudb_time_range {
 int id; // database value, also key to hash
 int iteration_start;
 int iteration_end;
 uint64_t time_start;
 uint64_t time_end;  // was this metric measured, or created by a post-processing tool?
 UT_hash_handle hh;
} TAUDB_TIME_RANGE;

/* timers are interval timers, capturing some interval value.  for callpath or
   phase profiles, the parent refers to the calling function or phase.
   timers can also be sample locations, or phases (dynamic or static), or
   sample aggregations (intermediate) */

typedef struct taudb_timer {
 int id; // database value, also key to hash
 struct taudb_trial* trial;  // pointer back to trial - NOTE: Necessary?
 char* name;                 // the full timer name, can have file, line, etc.
 char* short_name;           // just the function name, for example
 char* source_file;          // what source file does this function live in?
 int line_number;            // what line does the timer start on?
 int line_number_end;        // what line does the timer end on?
 int column_number;          // what column number does the timer start on?
 int column_number_end;      // what column number does the timer end on?
 int group_count;            // how many groups does this timer belong to?
 int parameter_count;        // how many parameters does this timer have?
 struct taudb_timer_group* groups;   // array of groups
 struct taudb_timer_parameter* parameters;   // array of parameters
 UT_hash_handle hh;
} TAUDB_TIMER;

/*********************************************/
/* timer related structures  */
/*********************************************/

/* timer groups are the groups such as tau_default,
   mpi, openmp, tau_phase, tau_callpath, tau_param, etc. 
   this mapping table allows for nxn mappings between timers
   and groups */

typedef struct taudb_timer_group {
 int id; // database reference, and hash key
 char* name;
 int timer_count;    // how many timers are in this group?
 struct taudb_timer* timers;   // array of timers
 UT_hash_handle hh;
} TAUDB_TIMER_GROUP;

/* timer parameters are parameter based profile values. 
   an example is foo (x,y) where x=4 and y=10. in that example,
   timer would be the index of the timer with the
   name 'foo (x,y) <x>=<4> <y>=<10>'. this table would have two
   entries, one for the x value and one for the y value.
   The parameter can also be a phase / iteration index.
*/

typedef struct taudb_timer_parameter {
 int id; // database reference, and hash key
 char* name;
 char* value;
 UT_hash_handle hh;
} TAUDB_TIMER_PARAMETER;

/* callpath objects contain the merged dynamic callgraph tree seen
 * during execution */

typedef struct taudb_timer_callpath {
 int id; // link back to database, and hash key
 struct taudb_timer* timer; // which timer is this?
 struct taudb_timer_callpath *parent; // callgraph parent
 UT_hash_handle hh;
} TAUDB_TIMER_CALLPATH;

/* timer_call_data objects are observations of a node of the callgraph
   for one of the threads. */

typedef struct taudb_timer_call_data {
 int id; // link back to database
 struct taudb_timer_callpath *timer_callpath; // link back to database
 struct taudb_thread *thread; // link back to database, roundabout way
 char *key; // hash table key - constructed as taudb_thread.index:taudb_timer.name
 int calls;  // number of times this timer was seen
 int subroutines;  // number of timers this timer calls
 char* timestamp;  // when was the timer_callpath visited?
 int timer_value_count;  // should be equal to taudb_trial.metric_count
 struct taudb_timer_value* timer_values;
 UT_hash_handle hh;
} TAUDB_TIMER_CALL_DATA;

/* finally, timer_values are specific measurements during one of the
   observations of the node of the callgraph on a thread. */

typedef struct taudb_timer_value {
 struct taudb_metric* metric;   // which metric is this?
 double inclusive;              // the inclusive value of this metric
 double exclusive;              // the exclusive value of this metric
 double inclusive_percentage;   // the inclusive percentage of total time of the application
 double exclusive_percentage;   // the exclusive percentage of total time of the application
 double sum_exclusive_squared;  // how much variance did we see every time we measured this timer?
 char *key; // hash table key - thread:timer_string:metric (all names)
 UT_hash_handle hh;
} TAUDB_TIMER_VALUE;

/*********************************************/
/* counter related structures  */
/*********************************************/

/* counters measure some counted value. An example would be MPI message size
 * for an MPI_Send.  */

typedef struct taudb_counter {
 int id; // database reference
 struct taudb_trial* trial;
 char* name;
} TAUDB_COUNTER;

/* counters are atomic counters, not just interval timers */

typedef struct taudb_counter_value {
 struct taudb_counter* counter; // the counter we are measuring
 struct taudb_thread* thread;   // where this measurement is
 struct taudb_timer_callpath* context; // the calling context (can be null)
 char* timestamp; // timestamp in case we are in a snapshot or something
 int sample_count;          // how many times did we see take this count?
 double maximum_value;      // what was the max value we saw?
 double minimum_value;      // what was the min value we saw?
 double mean_value;         // what was the average value we saw?
 double standard_deviation; // how much variance was there?
} TAUDB_COUNTER_VALUE;

/*********************************************/
/* metadata related structures  */
/*********************************************/

/* primary metadata is metadata that is not nested, does not
   contain unique data for each thread. */

typedef struct taudb_primary_metadata {
 char* name;
 char* value;
 UT_hash_handle hh; // uses the name as the key
} TAUDB_PRIMARY_METADATA;

/* primary metadata is metadata that could be nested, could
   contain unique data for each thread, and could be an array. */

typedef struct taudb_secondary_metadata {
 int id; // link back to database
 struct taudb_timer_call_data* timer_call_data; 
 struct taudb_thread* thread; 
 struct taudb_secondary_metadata* parent; // self-referencing 
 int num_values; // can have arrays of data
 char* name;
 char** value;
 int child_count;
 struct taudb_secondary_metadata* children; // self-referencing 
 char* key;
 UT_hash_handle hh; // uses the key as a compound key
} TAUDB_SECONDARY_METADATA;

/* these are for supporting the older schema */

typedef struct perfdmf_experiment {
 int id;
 char* name;
 int primary_metadata_count;
 struct taudb_primary_metadata* primary_metadata;
} PERFDMF_EXPERIMENT;

typedef struct perfdmf_application {
 int id;
 char* name;
 int primary_metadata_count;
 struct taudb_primary_metadata* primary_metadata;
} PERFDMF_APPLICATION;

#endif // TAUDB_STRUCTS_H
