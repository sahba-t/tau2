/****************************************************************************
**			TAU Portable Profiling Package			   **
**			http://www.cs.uoregon.edu/research/paracomp/tau    **
*****************************************************************************
**    Copyright 2003  						   	   **
**    Department of Computer and Information Science, University of Oregon **
**    Advanced Computing Laboratory, Los Alamos National Laboratory        **
**    Research Center Juelich, Germany                                     **
****************************************************************************/
/***************************************************************************
**	File 		: tau2vtf.cpp 					  **
**	Description 	: TAU to VTF3 translator                          **
**	Author		: Sameer Shende					  **
**	Contact		: sameer@cs.uoregon.edu 	                  **
***************************************************************************/
#include <TAU_tf.h>
#include <stdio.h>
#include <iostream>
#include <stddef.h>
#include <vtf3.h> /* VTF3 header file */
#include <map>
#include <stack>
using namespace std;
int debugPrint = 0;
#define dprintf if (debugPrint) printf

/* The choice of the following numbers is arbitrary */
#define TAU_SAMPLE_CLASS_TOKEN   71
#define TAU_DEFAULT_COMMUNICATOR 42
/* any unique id */

/* implementation of callback routines */
map< pair<int,int>, int, less< pair<int,int> > > EOF_Trace;
int EndOfTrace = 0;  /* false */

/* Define limits of sample data (user defined events) */
struct {
  unsigned long umin;
  unsigned long umax;
} taulongbounds = { 0, (unsigned long)~(unsigned long)0 };

struct {
  double fmin;
  double fmax;
} taufloatbounds = {-1.0e+300, +1.0e+300};

/* These structures are used in user defined event data */



/* Global data */
int sampgroupid = 0;
int groupid = 0;
int sampclassid = 0; 
stack <unsigned int> *callstack;


/* utilities */
int GlobalId(int localnodeid, int localthreadid)
{
  /* for multithreaded programs, modify this routine */
  return localnodeid; 
  /* ignore localthreadid for now */
}

/* implementation of callback routines */
/***************************************************************************
 * Description: EnterState is called at routine entry by trace input library
 * 		This is a callback routine which must be registered by the 
 * 		trace converter. 
 ***************************************************************************/
int EnterState(void *userData, double time, 
		unsigned int nodeid, unsigned int tid, unsigned int stateid)
{
  dprintf("Entered state %d time %g nid %d tid %d\n", 
		  stateid, time, nodeid, tid);
  int cpuid = GlobalId(nodeid, tid);

  callstack[cpuid].push(stateid);

  VTF3_WriteDownto(userData, time, stateid, cpuid, VTF3_SCLNONE);
  return 0;
}

/***************************************************************************
 * Description: EnterState is called at routine exit by trace input library
 * 		This is a callback routine which must be registered by the 
 * 		trace converter. 
 ***************************************************************************/
int LeaveState(void *userData, double time, unsigned int nid, unsigned int tid)
{
  dprintf("Leaving state time %g nid %d tid %d\n", time, nid, tid);
  int cpuid = GlobalId(nid, tid);
  int stateid = callstack[cpuid].top();
  callstack[cpuid].pop();
  
  VTF3_WriteUpfrom(userData, time, stateid, cpuid, VTF3_SCLNONE);
  return 0;
}

/***************************************************************************
 * Description: ClockPeriod (in microseconds) is specified here. 
 * 		This is a callback routine which must be registered by the 
 * 		trace converter. 
 ***************************************************************************/
int ClockPeriod( void*  userData, double clkPeriod )
{
  dprintf("Clock period %g\n", clkPeriod);
  VTF3_WriteDefclkperiod(userData, clkPeriod);

  return 0;
}

/***************************************************************************
 * Description: DefThread is called when a new nodeid/threadid is encountered.
 * 		This is a callback routine which must be registered by the 
 * 		trace converter. 
 ***************************************************************************/
int DefThread(void *userData, unsigned int nodeToken, unsigned int threadToken,
const char *threadName )
{
  dprintf("DefThread nid %d tid %d, thread name %s\n", 
		  nodeToken, threadToken, threadName);
  EOF_Trace[pair<int,int> (nodeToken,threadToken) ] = 0; /* initialize it */
  return 0;
}

/***************************************************************************
 * Description: EndTrace is called when an EOF is encountered in a tracefile.
 * 		This is a callback routine which must be registered by the 
 * 		trace converter. 
 ***************************************************************************/
int EndTrace( void *userData, unsigned int nodeToken, unsigned int threadToken)
{
  dprintf("EndTrace nid %d tid %d\n", nodeToken, threadToken);
  EOF_Trace[pair<int,int> (nodeToken,threadToken) ] = 1; /* flag it as over */
  /* yes, it is over */
  map < pair<int, int>, int, less< pair<int,int> > >::iterator it;
  EndOfTrace = 1; /* Lets assume that it is over */
  for (it = EOF_Trace.begin(); it != EOF_Trace.end(); it++)
  { /* cycle through all <nid,tid> pairs to see if it really over */
    if ((*it).second == 0)
    {
      EndOfTrace = 0; /* not over! */
      /* If there's any processor that is not over, then the trace is not over */
    }
  }
  return 0;
}

/***************************************************************************
 * Description: DefStateGroup registers a profile group name with its id.
 * 		This is a callback routine which must be registered by the 
 * 		trace converter. 
 ***************************************************************************/
int DefStateGroup( void *userData, unsigned int stateGroupToken, 
		const char *stateGroupName )
{
  dprintf("StateGroup groupid %d, group name %s\n", stateGroupToken, 
		  stateGroupName);

  /* create a default activity (group) */
  VTF3_WriteDefact(userData, stateGroupToken, stateGroupName);
  return 0;
}

/***************************************************************************
 * Description: DefState is called to define a new symbol (event). It uses
 *		the token used to define the group identifier. 
 * 		This is a callback routine which must be registered by the 
 * 		trace converter. 
 ***************************************************************************/
int DefState( void *userData, unsigned int stateToken, const char *stateName, 
		unsigned int stateGroupToken )
{
  dprintf("DefState stateid %d stateName %s stategroup id %d\n",
		  stateToken, stateName, stateGroupToken);

  /* create a state record */
  VTF3_WriteDefstate(userData, stateGroupToken, stateToken, stateName, VTF3_SCLNONE);

  return 0;
}

/***************************************************************************
 * Description: DefUserEvent is called to register the name and a token of the
 *  		user defined event (or a sample event in Vampir terminology).
 * 		This is a callback routine which must be registered by the 
 * 		trace converter. 
 ***************************************************************************/
int DefUserEvent( void *userData, unsigned int userEventToken,
		const char *userEventName )
{

  dprintf("DefUserEvent event id %d user event name %s\n", userEventToken,
		  userEventName);
  /* VTF3_WriteDefsampclass(userData, userEventToken, userEventName);
  */
  int iscpugrpsamp = 1;
  int dodifferentiation = 1; /* for hw counter data */
  VTF3_WriteDefsamp(userData, userEventToken, TAU_SAMPLE_CLASS_TOKEN, 
	iscpugrpsamp, sampgroupid, VTF3_VALUETYPE_UINT, 
	(void *) &taulongbounds, dodifferentiation, VTF3_DATAREPHINT_BEFORE, 
	userEventName, "#/s");

  return 0;
}

/***************************************************************************
 * Description: EventTrigger is called when a user defined event is triggered.
 * 		This is a callback routine which must be registered by the 
 * 		trace converter. 
 ***************************************************************************/
int EventTrigger( void *userData, double time, 
		unsigned int nodeToken,
		unsigned int threadToken,
	       	unsigned int userEventToken,
		long long userEventValue)
{
  dprintf("EventTrigger: time %g, nid %d tid %d event id %d triggered value %lld \n", time, nodeToken, threadToken, userEventToken, userEventValue);

  int type = VTF3_VALUETYPE_UINT;
  int cpuid = GlobalId (nodeToken, threadToken); /* GID */
  int samplearraydim = 1; 
  VTF3_WriteSamp(userData, time, cpuid, samplearraydim, 
		  (const int *) &userEventToken, &type, &userEventValue);

  return 0;
}

/***************************************************************************
 * Description: SendMessage is called when a message is sent by a process.
 * 		This is a callback routine which must be registered by the 
 * 		trace converter. 
 ***************************************************************************/
int SendMessage( void *userData, double time, 
		unsigned int sourceNodeToken,
		unsigned int sourceThreadToken, 
		unsigned int destinationNodeToken,
		unsigned int destinationThreadToken,
		unsigned int messageSize,
		unsigned int messageTag )
{
  dprintf("SendMessage: time %g, source nid %d tid %d, destination nid %d tid %d, size %d, tag %d\n", 
		  time, 
		  sourceNodeToken, sourceThreadToken,
		  destinationNodeToken, destinationThreadToken,
		  messageSize, messageTag);

  int source = GlobalId(sourceNodeToken, sourceThreadToken);
  int dest   = GlobalId(destinationNodeToken, destinationThreadToken);
  VTF3_WriteSendmsg(userData, time, source, dest, TAU_DEFAULT_COMMUNICATOR, 
	messageTag, messageSize, VTF3_SCLNONE);
  return 0;
}

/***************************************************************************
 * Description: RecvMessage is called when a message is received by a process.
 * 		This is a callback routine which must be registered by the 
 * 		trace converter. 
 ***************************************************************************/
int RecvMessage( void *userData, double time,
		unsigned int sourceNodeToken,
		unsigned int sourceThreadToken, 
		unsigned int destinationNodeToken,
		unsigned int destinationThreadToken,
		unsigned int messageSize,
		unsigned int messageTag )
{
  dprintf("RecvMessage: time %g, source nid %d tid %d, destination nid %d tid %d, size %d, tag %d\n", 
		  time, 
		  sourceNodeToken, sourceThreadToken,
		  destinationNodeToken, destinationThreadToken,
		  messageSize, messageTag);

  int source = GlobalId(sourceNodeToken, sourceThreadToken);
  int dest   = GlobalId(destinationNodeToken, destinationThreadToken);

  VTF3_WriteRecvmsg(userData, time, dest, source, TAU_DEFAULT_COMMUNICATOR, 
	messageTag, messageSize, VTF3_SCLNONE);

  return 0;
}

/***************************************************************************
 * Description: To clean up and reset the end of file marker, we invoke this.
 ***************************************************************************/
int ResetEOFTrace(void)
{
  /* mark all entries of EOF_Trace to be false */
  for (map< pair<int,int>, int, less< pair<int,int> > >:: iterator it = 
		  EOF_Trace.begin(); it != EOF_Trace.end(); it++)
  { /* Explicilty mark end of trace to be not over */ 
    (*it).second = 0;
  }
}

/***************************************************************************
 * Description: The main entrypoint. 
 ***************************************************************************/
int main(int argc, char **argv)
{
  Ttf_FileHandleT fh;
  int recs_read, pos;
  char *trace_file;
  char *edf_file;
  char *out_file; 
  int output_format = VTF3_FILEFORMAT_STD_BINARY; /* Binary by default */
  int no_state_flag=0, no_message_flag=0;
  int i; 
  /* main program: Usage app <trc> <edf> [-a] [-nomessage] */
  if (argc < 3)
  {
    printf("Usage: %s <TAU trace> <edf file> <out file> [-a|-fa] [-nomessage]  [-v]\n", 
		    argv[0]);
    exit(1);
  }
  
/***************************************************************************
 * -a stands for ASCII, -fa stands for FAST ASCII and -v is for verbose. 
 ***************************************************************************/
  for (int i = 0; i < argc ; i++)
  {
    switch(i) {
      case 0:
	trace_file = argv[1];
	break;
      case 1:
	edf_file = argv[2];
	break;
      case 2: 
	out_file = argv[3]; 
	break; 
      default:
	if (strcmp(argv[i], "-a")==0)
	{ /* Use ASCII format */
	  output_format = VTF3_FILEFORMAT_STD_ASCII;
	}
	if (strcmp(argv[i], "-fa")==0)
	{ /* Use FAST ASCII format */
	  output_format = VTF3_FILEFORMAT_FST_ASCII;
	}
	if (strcmp(argv[i], "-nomessage")==0)
	{
	  no_message_flag = 1;
	}
	if (strcmp(argv[i], "-v") == 0)
        {
	  debugPrint = 1;
	}
	break;
    }
  }
  /* Finished parsing commandline options, now open the trace file */

  fh = Ttf_OpenFileForInput( argv[1], argv[2]);

  if (!fh)
  {
    printf("ERROR:Ttf_OpenFileForInput fails");
    exit(1);
  }


  /* Open VTF3 Trace file for output */
  (void) VTF3_InitTables();

  int write_unmerged_records ;

  /* Define the file control block for output trace file */
  void *fcb = VTF3_OpenFileOutput(out_file, output_format, 
		  write_unmerged_records = 0);

  /* check and verify that it was opened properly */
  if (fcb == 0)
  {
    perror(out_file);
    exit(1);
  }

  /* Write the trace file header */
  VTF3_WriteDefversion(fcb, VTF3_GetVersionNumber());
  VTF3_WriteDefcreator(fcb, "tau2vtf converter");
  VTF3_WriteDefsampclass(fcb, TAU_SAMPLE_CLASS_TOKEN, "TAU counter data");


  /* in the first pass, we determine the no. of cpus and other group related
   * information. In the second pass, we look at the function entry/exits */ 

  Ttf_CallbacksT firstpass;
  /* In the first pass, we just look for node/thread ids and def records */
  firstpass.UserData = fcb;
  firstpass.DefThread = DefThread;
  firstpass.EndTrace = EndTrace;
  firstpass.DefClkPeriod = ClockPeriod;
  firstpass.DefThread = DefThread;
  firstpass.DefStateGroup = DefStateGroup;
  firstpass.DefState = DefState;
  firstpass.SendMessage = 0; /* Important to declare these as null! */
  firstpass.RecvMessage = 0; /* Important to declare these as null! */
  firstpass.DefUserEvent = 0;
  firstpass.EventTrigger = 0; /* these events are ignored in the first pass */
  firstpass.EnterState = 0;   /* these events are ignored in the first pass */
  firstpass.LeaveState = 0;   /* these events are ignored in the first pass */


  /* Go through all trace records */
  do {
    recs_read = Ttf_ReadNumEvents(fh,firstpass, 1024);
#ifdef DEBUG 
    if (recs_read != 0)
      cout <<"Read "<<recs_read<<" records"<<endl;
#endif 
  }
  while ((recs_read >=0) && (!EndOfTrace));
  /* reset the position of the trace to the first record */
  for (map< pair<int,int>, int, less< pair<int,int> > >:: iterator it = 
		  EOF_Trace.begin(); it != EOF_Trace.end(); it++)
  { /* Explicilty mark end of trace to be not over */ 
    (*it).second = 0;
  }

  int totalnidtids = EOF_Trace.size(); 
  /* This is ok for single threaded programs. For multi-threaded programs
   * we'll need to modify the way we describe the cpus/threads */
  VTF3_WriteDefsyscpunums(fcb, 1, &totalnidtids);

  unsigned int *idarray = new unsigned int[totalnidtids];
  for (i = 0; i < totalnidtids; i++)
  { /* assign i to each entry */
    idarray[i] = i;
  }
  
  /* create a callstack on each thread/process id */
  callstack = new stack<unsigned int> [totalnidtids](); 

  /* Define group ids */
  char name[1024];
  strcpy(name, "TAU sample group name");
  VTF3_WriteDefcpugrp(fcb, sampgroupid, totalnidtids, idarray, name);

  EndOfTrace = 0;
  /* now reset the position of the trace to the first record */ 
  Ttf_CloseFile(fh);
  /* Re-open it for input */
  fh = Ttf_OpenFileForInput( argv[1], argv[2]);

  if (!fh)
  {
    printf("ERROR:Ttf_OpenFileForInput fails the second time");
    exit(1);
  }

  dprintf("Re-analyzing the trace file \n");
 

  Ttf_CallbacksT cb;
  /* Fill the callback struct */
  cb.UserData = fcb;
  cb.DefClkPeriod = 0;
  cb.DefThread = 0;
  cb.DefStateGroup = 0;
  cb.DefState = 0;
  cb.DefUserEvent = DefUserEvent;
  cb.EventTrigger = EventTrigger;
  cb.EndTrace = EndTrace;

  /* should state transitions be displayed? */
  /* Of course! */
  cb.EnterState = EnterState;
  cb.LeaveState = LeaveState;

  /* should messages be displayed? */
  if (no_message_flag)
  {
    cb.SendMessage = 0;
    cb.RecvMessage = 0;
  }
  else
  {
    cb.SendMessage = SendMessage;
    cb.RecvMessage = RecvMessage;
  }
  
  int writtenchars;
  size_t writtenbytes;
  /* Go through each record until the end of the trace file */

  do {
    recs_read = Ttf_ReadNumEvents(fh,cb, 1024);
#ifdef DEBUG  
    if (recs_read != 0)
      cout <<"Read "<<recs_read<<" records"<<endl;
#endif /* DEBUG */
  }
  while ((recs_read >=0) && (!EndOfTrace));

  /* dummy records */
  Ttf_CloseFile(fh);

  /* close VTF file */
  VTF3_Close(fcb);
  return 0;
}

/* EOF tau2vtf.cpp */


/***************************************************************************
 * $RCSfile: tau2vtf.cpp,v $   $Author: sameer $
 * $Revision: 1.2 $   $Date: 2004/08/05 22:49:37 $
 * VERSION_ID: $Id: tau2vtf.cpp,v 1.2 2004/08/05 22:49:37 sameer Exp $
 ***************************************************************************/


