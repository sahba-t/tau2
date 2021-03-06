/****************************************************************************
**			TAU Portable Profiling Package			   **
**			http://www.cs.uoregon.edu/research/tau	           **
*****************************************************************************
**    Copyright 1997  						   	   **
**    Department of Computer and Information Science, University of Oregon **
**    Advanced Computing Laboratory, Los Alamos National Laboratory        **
****************************************************************************/
/***************************************************************************
**	File 		: SprocLayer.cpp				  **
**	Description 	: TAU Profiling Package RTS Layer definitions     **
**			  for supporting pthreads 			  **
**	Contact		: tau-team@cs.uoregon.edu 		 	  **
**	Documentation	: See http://www.cs.uoregon.edu/research/tau      **
***************************************************************************/


//////////////////////////////////////////////////////////////////////
// Include Files 
//////////////////////////////////////////////////////////////////////

//#define DEBUG_PROF
#include <abi_mutex.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ulocks.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/sysmp.h>
#include <sys/syssgi.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef TAU_DOT_H_LESS_HEADERS
#include <iostream>
#include <map>
using namespace std;
#else /* TAU_DOT_H_LESS_HEADERS */
#include <map.h>
#include <iostream.h>
#endif /* TAU_DOT_H_LESS_HEADERS */
#include "Profile/Profiler.h"

#define TAU_SYNC_SPROC



/////////////////////////////////////////////////////////////////////////
// Member Function Definitions For class SprocLayer
// This allows us to get thread ids from 0..N-1 instead of long nos 
// as generated by getpid()
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
// Define the static private members of SprocLayer  
/////////////////////////////////////////////////////////////////////////
usptr_t * SprocLayer::tauArena = NULL; 
int 	  SprocLayer::tauThreadCount = 0; 
usema_t * SprocLayer::tauDBMutex = NULL; 
usema_t * SprocLayer::tauEnvMutex = NULL; 
usema_t * SprocLayer::tauThreadCountMutex = NULL; 

map<pid_t, int, less<pid_t> >& TheTauPidSprocMap(void)
{
  static map<pid_t, int, less<pid_t> > TauPidMap;

  return TauPidMap;
}



////////////////////////////////////////////////////////////////////////
// RegisterThread() should be called before any profiling routines are
// invoked. This routine sets the thread id that is used by the code in
// FunctionInfo and Profiler classes. This should be the first routine a 
// thread should invoke from its wrapper. Note: main() thread shouldn't
// call this routine. 
////////////////////////////////////////////////////////////////////////
int SprocLayer::RegisterThread(void)
{

  if (uspsema(tauThreadCountMutex) == -1)
    perror("TAU ERROR: SprocLayer.cpp: SprocLayer::RegisterThread uspsema");
  
  TheTauPidSprocMap()[RtsLayer::getPid()] = RtsLayer::createThread();
  
  if (usvsema(tauThreadCountMutex) == -1)
    perror("TAU ERROR: SprocLayer.cpp: SprocLayer::RegisterThread usvsema");
  

  DEBUGPROFMSG("Thread id "<< tauThreadCount<< " Created! "<<endl;);

  return 0;
}


////////////////////////////////////////////////////////////////////////
// GetThreadId returns an id in the range 0..N-1 by looking at the 
// thread specific data. Since a getspecific has to be preceeded by a 
// setspecific (that all threads besides main do), we get a null for the
// main thread that lets us identify it as thread 0. It is the only 
// thread that doesn't do a SprocLayer::RegisterThread(). 
////////////////////////////////////////////////////////////////////////
int SprocLayer::GetThreadId(void) 
{

  static int initflag = SprocLayer::InitializeThreadData();
  // if its in here the first time, setup mutexes etc.

  if (tauThreadCountMutex == NULL)
    cout <<"THREADING PROBLEM!!! tauThreadCountMutex not initialized!!"<<endl;

  map<pid_t, int, less<pid_t> >::iterator it;
  int pid = RtsLayer::getPid();
  int tid;
  // Each sproc thread has a unique pid (not in 0..N-1 range)
  // This method converts it to that range as follows:

#ifdef TAU_SYNC_SPROC
  if (uspsema(tauThreadCountMutex) == -1)
    perror("TAU ERROR: SprocLayer.cpp: SprocLayer::GetThreadId uspsema");
#endif /* TAU_SYNC_SPROC */

  if ((it = TheTauPidSprocMap().find(pid))== TheTauPidSprocMap().end())
    tid = 0; // Main thread doesn't do RegsiterThread
  else 
    tid = TheTauPidSprocMap()[pid]; // Threads 1..N-1 

#ifdef TAU_SYNC_SPROC
  if (usvsema(tauThreadCountMutex) == -1)
    perror("TAU ERROR: SprocLayer.cpp: SprocLayer::GetThreadId usvsema");
#endif /* TAU_SYNC_SPROC */

  return tid;
 
}

////////////////////////////////////////////////////////////////////////
// InitializeThreadData is called before any thread operations are performed. 
// It sets the default values for static private data members of the 
// SprocLayer class.
////////////////////////////////////////////////////////////////////////
int SprocLayer::InitializeThreadData(void)
{
  static int flag = 0;

  if (flag == 0)
  {
    flag = 1;
    // Initialize the mutex
    tauArena = usinit("/dev/zero");
    if (!tauArena)
      perror("TAU ERROR: SprocLayer.cpp InitializeThreadData(): Arena /dev/zero not initialized!");
    
    tauDBMutex = usnewsema(tauArena, 1);
    tauEnvMutex = usnewsema(tauArena, 1);
    tauThreadCountMutex = usnewsema(tauArena, 1);
    
    
    DEBUGPROFMSG("SprocLayer::Initialize() done! " <<endl;);
  }
  return 1;
}

////////////////////////////////////////////////////////////////////////
int SprocLayer::InitializeDBMutexData(void)
{
  // For locking functionDB 
  
  DEBUGPROFMSG("Initialized the functionDB Mutex data " <<endl;);
  return 1;
}

////////////////////////////////////////////////////////////////////////
// LockDB locks the mutex protecting TheFunctionDB() global database of 
// functions. This is required to ensure that push_back() operation 
// performed on this is atomic (and in the case of tracing this is 
// followed by a GetFunctionID() ). This is used in 
// FunctionInfo::FunctionInfoInit().
////////////////////////////////////////////////////////////////////////
int SprocLayer::LockDB(void)
{
  static int initflag=InitializeThreadData();
  if (uspsema(tauDBMutex) == -1)
    perror("TAU ERROR: SprocLayer.cpp: SprocLayer::LockDB uspsema"); 

  return 1;
}

////////////////////////////////////////////////////////////////////////
// UnLockDB() unlocks the mutex tauDBMutex used by the above lock operation
////////////////////////////////////////////////////////////////////////
int SprocLayer::UnLockDB(void)
{
  // Unlock the functionDB mutex
  if (usvsema(tauDBMutex) == -1)
    perror("TAU ERROR: SprocLayer.cpp: SprocLayer::UnLockDB usvsema"); 
  return 1;
}  

////////////////////////////////////////////////////////////////////////
int SprocLayer::InitializeEnvMutexData(void)
{
  // For locking functionEnv 
  
  DEBUGPROFMSG("Initialized the functionEnv Mutex data " <<endl;);
  return 1;
}

////////////////////////////////////////////////////////////////////////
// LockEnv locks the mutex protecting TheFunctionEnv() global database of 
// functions. This is required to ensure that push_back() operation 
// performed on this is atomic (and in the case of tracing this is 
// followed by a GetFunctionID() ). This is used in 
// FunctionInfo::FunctionInfoInit().
////////////////////////////////////////////////////////////////////////
int SprocLayer::LockEnv(void)
{
  static int initflag=InitializeThreadData();
  if (uspsema(tauEnvMutex) == -1)
    perror("TAU ERROR: SprocLayer.cpp: SprocLayer::LockEnv uspsema"); 

  return 1;
}

////////////////////////////////////////////////////////////////////////
// UnLockEnv() unlocks the mutex tauEnvMutex used by the above lock operation
////////////////////////////////////////////////////////////////////////
int SprocLayer::UnLockEnv(void)
{
  // Unlock the functionEnv mutex
  if (usvsema(tauEnvMutex) == -1)
    perror("TAU ERROR: SprocLayer.cpp: SprocLayer::UnLockEnv usvsema"); 
  return 1;
}  


/***************************************************************************
 * $RCSfile: SprocLayer.cpp,v $   $Author: amorris $
 * $Revision: 1.6 $   $Date: 2009/01/16 00:46:52 $
 * POOMA_VERSION_ID: $Id: SprocLayer.cpp,v 1.6 2009/01/16 00:46:52 amorris Exp $
 ***************************************************************************/


