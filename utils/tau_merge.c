/*********************************************************************/
/*                  pC++/Sage++  Copyright (C) 1994                  */
/*  Indiana University  University of Oregon  University of Rennes   */
/*********************************************************************/

/*
 * pcxx_merge.c: merge local traces 
 *
 * (c) 1994 Jerry Manic Saftware
 *
 * Version 3.0
 */

#ifdef __SP1__
# include <Profile/aix.h> /* if its an IBM */
#endif /* __SP1__ */
# include <stdio.h>
# include <stdlib.h>
# include <sys/types.h>
# include <fcntl.h>
# include <unistd.h>

#ifndef NeXT
# include <unistd.h>
#endif

#ifdef FUJITSU
# include <Profile/fujitsu.h>
#endif

# include <string.h>

# define TRACING_ON
# define PCXX_EVENT_SRC

# ifdef __PCXX__
#   include "Profile/pcxx_events_def.h"
# else
#   include "Profile/pcxx_events.h"
# endif
# include "Profile/pcxx_ansi.h"

# ifndef TRUE
#   define FALSE  0
#   define TRUE   1
# endif

# define F_EXISTS    0

# define CONTLEN  (sizeof(PCXX_EV) - sizeof(long int))

# define STDOUT 1

/* -- buffer sizes ------------------ */
# define INMAX    BUFSIZ   /* records */
# define OUTMAX   BUFSIZ   /* chars   */

int dynamic = TRUE ; /* by default events.<node>.edf files exist */
#ifndef TAU_XLC
extern "C" {
#endif /* TAU_XLC */
  int open_edf_file(char *prefix, int nodeid);
  int parse_edf_file(int node);
  int store_merged_edffile(char *filename);
  int GID(int node, long localEventId); 
#ifndef TAU_XLC
} 
#endif /* TAU_XLC */


static struct trcdescr
{
  int     fd;              /* -- input file descriptor                     -- */
  char   *name;            /* -- corresponding file name                   -- */
  int     nid;             /* -- corresponding PTX PID                     -- */
  int     overflows;       /* -- clock overflows in that trace             -- */
  int     contlen;         /* -- length of continuation event buffer       -- */
  long    numrec;          /* -- number of event records already processed -- */
  unsigned long lasttime;  /* -- timestamp of previous event record        -- */
  unsigned long offset;    /* -- offset of timestamp                       -- */

  PCXX_EV  *buffer;    /* -- input buffer                              -- */
  PCXX_EV  *erec;      /* -- current event record                      -- */
  PCXX_EV  *next;      /* -- next available event record in buffer     -- */
  PCXX_EV  *last;      /* -- last event record in buffer               -- */
} *trcdes;

/* -------------------------------------------------------------------------- */
/* -- input buffer handling                                                -- */
/* -------------------------------------------------------------------------- */

static PCXX_EV *get_next_rec(struct trcdescr *tdes)
{
  long no;

  if ( (tdes->last == NULL) || (tdes->next > tdes->last) )
  {
    /* -- input buffer empty: read new records from file -------------------- */
    if ( (no = read (tdes->fd, tdes->buffer, INMAX * sizeof(PCXX_EV)))
         != (INMAX * sizeof(PCXX_EV)) )
    {
      if ( no == 0 )
      {
        /* -- no more event record: ----------------------------------------- */
        close (tdes->fd);
        tdes->fd = -1;
        return ( (PCXX_EV *) NULL);
      }
      else if ( (no % sizeof(PCXX_EV)) != 0 )
      {
        /* -- read error: --------------------------------------------------- */
        fprintf (stderr, "%s: read error\n", tdes->name);
        exit (1);
      }
    }

    /* -- we got some event records ----------------------------------------- */
    tdes->next = tdes->buffer;
    tdes->last = tdes->buffer + (no / sizeof(PCXX_EV)) - 1;
  }
  return (tdes->erec = tdes->next++);
}

/* -------------------------------------------------------------------------- */
/* -- Routines Output*: Simple output buffering                            -- */
/* -------------------------------------------------------------------------- */

static char  outbuffer[OUTMAX];
static int   outidx = 0;
static char *outptr = outbuffer;

static void output_flush(int fd)
{
  if ( outidx > 0 )
  {
    if ( write (fd, outbuffer, outidx) != outidx )
    {
      perror ("write output");
      exit (1);
    }
    outidx = 0;
    outptr = outbuffer;
  }
}

static void output(int fd, char *data, size_t l)
{
  if ( outidx + l >= BUFSIZ )
  {
    /* not enough space for data in output buffer: flush buffer */
    output_flush (fd);
  }

  memcpy (outptr, data, l);
  outptr += l;
  outidx += l;
}

/* -------------------------------------------------------------------------- */
/* -- FILE DESCRIPTOR HANDLING ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

#include <sys/time.h>
#include <sys/resource.h>

int cannot_get_enough_fd(int need)
{
# if defined(__hpux) || defined(sun)
  /* -- system supports get/setrlimit (RLIMIT_NOFILE) -- */
  struct rlimit rlp;

  getrlimit (RLIMIT_NOFILE, &rlp);
  if ( rlp.rlim_max < need )
    return (TRUE);
  else if ( rlp.rlim_cur < need )
  {
    rlp.rlim_cur = need;
    setrlimit (RLIMIT_NOFILE, &rlp);
  }
  return (FALSE);
# else
#   if defined(_SEQUENT_) || defined(sequent)
      /* -- system provides get/setdtablesize -- */
      int max = getdtablesize();
      return ( (max < need) && (setdtablesize (need) != need) );
#   else
      /* -- system provides only getdtablesize -- */
      int max = getdtablesize();
      return ( max < need );
#   endif
# endif
}

/* -------------------------------------------------------------------------- */
/* -- PCXX_MERGE MAIN PROGRAM ----------------------------------------------- */
/* -------------------------------------------------------------------------- */

extern char *optarg;
extern int   optind;

int main(int argc, char *argv[])
{
  int i, active, numtrc, source, outfd, errflag, first;
  int adjust, min_over, reassembly;
  unsigned long min_time, first_time;
  long numrec;
  char *trcfile;
  PCXX_EV *erec;
# ifdef __ksr__
  int *sequence;
  int last_pthread, num_pthreads;
# endif
# if defined(__ksr__) || defined(__CM5__)
  unsigned long last_time;
# endif

  numtrc     = 0;
  numrec     = 0;
  errflag    = FALSE;
  adjust     = FALSE;
  reassembly = TRUE;
  first_time = 0L;

  while ( (i = getopt (argc, argv, "ar")) != EOF )
  {
    switch ( i )
    {
      case 'a': /* -- adjust first time to zero -- */
                adjust = TRUE;
                break;

      case 'r': /* -- do not reassemble long events -- */
                reassembly = FALSE;
                break;

      default : /* -- ERROR -- */
                errflag = TRUE;
                break;
    }
  }

  /* -- check whether enough file descriptors available: -------------------- */
  /* -- max-file-descriptors - 4 (stdin, stdout, stderr, output trace) ------ */
  active = argc - optind - 1;
  if ( cannot_get_enough_fd (active + 4) )
  {
      fprintf (stderr, "%s: too many input traces:\n", argv[0]);
      fprintf (stderr, "  1. merge half of the input traces\n");
      fprintf (stderr, "  2. merge other half and output of step 1\n");
      exit (1);
  }
  trcdes = (struct trcdescr *) malloc (active * sizeof(struct trcdescr));

  for (i=optind; i<argc-1; i++)
  {
    /* -- open input trace -------------------------------------------------- */
    if ( (trcdes[numtrc].fd = open (argv[i], O_RDONLY)) < 0 )
    {
      perror (argv[i]);
      errflag = TRUE;
    }
    else
    {
      trcdes[numtrc].name      = argv[i];
      trcdes[numtrc].buffer    = (PCXX_EV *) malloc (INMAX * sizeof(PCXX_EV));
      trcdes[numtrc].erec      = (PCXX_EV *) NULL;
      trcdes[numtrc].next      = (PCXX_EV *) NULL;
      trcdes[numtrc].last      = (PCXX_EV *) NULL;
      trcdes[numtrc].overflows = 0;

      /* -- read first event record ----------------------------------------- */
      if ( (erec = get_next_rec (trcdes + numtrc)) == NULL )
      {
        /* -- no event record: ---------------------------------------------- */
        fprintf (stderr, "%s: warning: trace empty - ignored\n",
                 trcdes[numtrc].name);
        trcdes[numtrc].numrec = 0L;
        active--;
      }

      /* -- check first event record ---------------------------------------- */
      else if ( (erec->ev != PCXX_EV_INIT) && (erec->ev != PCXX_EV_INITM) )
      {
        fprintf (stderr, "%s: no valid event trace\n", trcdes[numtrc].name);
        exit (1);
      }
      else
      {
        if ( erec->nid > PCXX_MAXPROCS )
          fprintf (stderr,
           "%s: warning: node id %d too big for this machine (max. %d nodes)\n",
           trcdes[numtrc].name, erec->nid, PCXX_MAXPROCS);

        trcdes[numtrc].numrec = 1L;
        if ( erec->ev == PCXX_EV_INIT )
        {
	  if (!dynamic) { /* for dynamic trace, don't change this to INITM */
            erec->ev = PCXX_EV_INITM;
	  }
          trcdes[numtrc].nid = erec->nid;
        }
        else
          trcdes[numtrc].nid = -1;

        trcdes[numtrc].lasttime = erec->ti;
        trcdes[numtrc].offset   = 0L;

	if(dynamic)
	{
	/* parse edf file for this trace */
	  open_edf_file("events", trcdes[numtrc].nid);
	  parse_edf_file(trcdes[numtrc].nid);
	}
        numtrc++;
      }
    }
  }
  if (dynamic)
  {
    /* all edf files have been parsed now - store the final edf merged file */
    store_merged_edffile("tau.edf");
  }

  if ( (numtrc < 1) || errflag )
  {
    fprintf (stderr,
             "usage: %s [-a] [-r] inputtraces* (outputtrace|-)\n", argv[0]);
    fprintf(stderr,
    "Note: %s assumes edf files are named events.<nodeid>.edf and \n", argv[0]);
    fprintf(stderr,"      generates a merged edf file tau.edf\n");

    exit (1);
  }

  /* -- output trace file --------------------------------------------------- */
  if ( strcmp ((argv[argc-1]), "-") == 0 )
  {
    outfd = STDOUT;
  }
  else
  {
    trcfile = (char *) malloc ((strlen(argv[argc-1])+5) * sizeof(char));
    if ( strcmp ((argv[argc-1])+strlen(argv[argc-1])-4, ".trc") == 0 )
      strcpy (trcfile, argv[argc-1]);
    else
      sprintf (trcfile, "%s.trc", argv[argc-1]);

    if ( access (trcfile, F_EXISTS) == 0 && isatty(2) )
    {
      fprintf (stderr, "%s exists; override [y]? ", trcfile);
      if ( getchar() == 'n' ) exit (1);
    }
    if ( (outfd = open (trcfile, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0 )
    {
      perror (trcfile);
      exit (1);
    }
  }

# if defined(__ksr__) && defined(__PCXX__)
  /* ------------------------------------------------------------------------ */
  /* -- determine clock offset for KSR-1 ------------------------------------ */
  /* -- NB: really need a better algorithm here ----------------------------- */
  /* ------------------------------------------------------------------------ */
# define timestamp(i) \
                     (trcdes[sequence[i]].erec->ti + trcdes[sequence[i]].offset)
# define STEP 1

  sequence     = (int *) malloc (numtrc * sizeof(int));
  last_time    = 0L;
  num_pthreads = numtrc;
  last_pthread = numtrc - 1;

  do
  {
    /* -- search for next "in_barrier" event in each trace and -------------- */
    /* -- store sequence number --------------------------------------------- */
    for (i=0; i<numtrc; i++)
    {
      if ( trcdes[i].nid == (PCXX_MAXPROCS-1) )  /* master */
      {
        num_pthreads = numtrc - 1;
        last_pthread = numtrc - 2;
      }
      else
      {
        do
        {
          erec = get_next_rec (trcdes + i);
        }
        while ( (erec != NULL) && (erec->ev != PCXX_IN_BARRIER) );

        if ( erec == NULL )
          break;
        else
          sequence[erec->par] = i;
      }
    }

    if ( erec != NULL )
    {
      /* -- the first at this barrier must at least arrived later than the -- */
      /* -- last at the last barrier ---------------------------------------- */
      if ( timestamp(0) <= last_time )
      {
        trcdes[sequence[0]].offset += last_time - timestamp(0) + STEP;
      }

      for (i=1; i<num_pthreads; i++)
      {
        if ( timestamp(i) <= timestamp(i-1) )
        {
          trcdes[sequence[i]].offset += timestamp(i-1) - timestamp(i) + STEP;
        }
      }
      last_time = timestamp(last_pthread);
    }
  }
  while ( erec != NULL );

  /* -- only on the first worker pthread trace we should run into EndOfTrace  */
  /* -- This is normally trace 0 unless trace 0 is the master pthread trace - */
  if ( i > (0 + (trcdes[0].nid == (PCXX_MAXPROCS - 1))) )
  {
    fprintf (stderr, "%s: missing barrier\n", trcdes[i].name);
    exit (1);
  }

  /* -- report offset found and re-open/re-initialize all traces ------------ */
  for (i=0; i<numtrc; i++)
  {
    if ( trcdes[i].offset )
      printf ("%s: offset %ld\n", trcdes[i].name, trcdes[i].offset);

    /* -- if file was closed during the search for barriers, re-open it ----- */
    if ( trcdes[i].fd = -1 )
    {
      if ( (trcdes[i].fd = open (trcdes[i].name, O_RDONLY)) < 0 )
      {
        perror (argv[i]);
        exit (1);
      }
    }
    /* -- otherwise reset file offset --------------------------------------- */
    else if ( lseek (trcdes[i].fd, 0, SEEK_SET) == -1 )
    {
      fprintf (stderr, "%s: cannot reset trace\n", trcdes[i].name);
      exit (1);
    }
    trcdes[i].erec = NULL;
    trcdes[i].next = NULL;
    trcdes[i].last = NULL;

    erec = get_next_rec (trcdes + i);
    trcdes[i].numrec = 1L;
    if ( erec->ev == PCXX_EV_INIT )
    {
      if (!dynamic) { /* don't change this for dynamic */
        erec->ev = PCXX_EV_INITM;
      }
      trcdes[i].nid = erec->nid;
    }
    else
      trcdes[i].nid = -1;

    trcdes[i].lasttime = erec->ti = erec->ti + trcdes[i].offset;
  }
  printf ("\n");
# else
#   if defined(__CM5__) && defined(__PCXX__)
    /* ---------------------------------------------------------------------- */
    /* -- determine clock offset for TMC CM-5 ------------------------------- */
    /* ---------------------------------------------------------------------- */

    /* -- search for sync marker in all traces ------------------------------ */
    /* -- and determine highest clock value --------------------------------- */
    last_time = 0L;
    for (i=0; i<numtrc; i++)
    {
      do
      {
        erec = get_next_rec (trcdes + i);
      }
      while ( (erec != NULL) && (erec->ev != PCXX_SYNC_MARK) );

      if ( erec == NULL )
      {
        fprintf (stderr, "%s: cannot find sync marker\n", trcdes[i].name);
        exit (1);
      }

      if ( erec->ti > last_time ) last_time = erec->ti;
    }

    /* -- compute and report offset found and re-initialize all traces ------ */
    for (i=0; i<numtrc; i++)
    {
      trcdes[i].offset = last_time - trcdes[i].erec->ti;
      printf ("%s: offset %ld\n", trcdes[i].name, trcdes[i].offset);

      /* -- reset file offset ----------------------------------------------- */
      if ( lseek (trcdes[i].fd, 0, SEEK_SET) == -1 )
      {
        fprintf (stderr, "%s: cannot reset trace\n", trcdes[i].name);
        exit (1);
      }
      trcdes[i].erec = NULL;
      trcdes[i].next = NULL;
      trcdes[i].last = NULL;

      erec = get_next_rec (trcdes + i);
      trcdes[i].numrec = 1L;
      if ( erec->ev == PCXX_EV_INIT )
      {
        erec->ev = PCXX_EV_INITM;
        trcdes[i].nid = erec->nid;
      }
      else
        trcdes[i].nid = -1;

      trcdes[i].lasttime = erec->ti = erec->ti + trcdes[i].offset;
    }
    printf ("\n");
#   endif
# endif

  /* ------------------------------------------------------------------------ */
  /* -- merge files --------------------------------------------------------- */
  /* ------------------------------------------------------------------------ */
  source = 0;

  do
  {
    /* -- compute minimum of all timestamps and store the index of the ------ */
    /* -- corresponding trace in source ------------------------------------- */
    first = TRUE;
    for (i=0; i<numtrc; i++)
    {
      if ( trcdes[i].fd != -1 )
      {
        if ( first )
        {
          min_time = trcdes[i].lasttime;
          min_over = trcdes[i].overflows;
          source = i;
          first  = FALSE;
        }
        else if ( trcdes[i].overflows < min_over )
        {
          min_time = trcdes[i].lasttime;
          min_over = trcdes[i].overflows;
          source = i;
        }
        else if ( (trcdes[i].overflows == min_over) &&
                  (trcdes[i].lasttime < min_time) )
        {
          min_time = trcdes[i].lasttime;
          source = i;
        }
      }
    }

    if ( adjust )
    {
      if ( numrec == 0 ) first_time = trcdes[source].erec->ti;
      trcdes[source].erec->ti -= first_time;
    }
    /* -- correct event id to be global event id ---------------------------- */
#ifdef DEBUG 
    printf("Before conv event %ld ", trcdes[source].erec->ev);
#endif /* DEBUG */

    trcdes[source].erec->ev = GID(trcdes[source].nid, trcdes[source].erec->ev);

#ifdef DEBUG
    printf("Output: node %d event %d\n", source, trcdes[source].erec->ev);
#endif /* DEBUG */

    output (outfd, (char *) trcdes[source].erec, sizeof(PCXX_EV));
    numrec++;

# if defined(__ksr__) && defined(__PCXX__)
    erec = trcdes[source].erec;
    if ( erec->ev == PCXX_IN_BARRIER )
    {
      if ( ((last_pthread + 1) % num_pthreads) != erec->par )
        fprintf (stderr, "pcxx_Barrier sequence error at %ld (%d -> %d)\n",
                 erec->ti, last_pthread, erec->par);
      last_pthread = erec->par;
    }
# endif

    /* -- get next event record(s) from same trace -------------------------- */
    do
    {
      if ( (erec = get_next_rec (trcdes + source)) == NULL )
      {
        active--;
        break;
      }
      else
      {
        trcdes[source].numrec++;

        if ( erec->ev == PCXX_EV_CONT_EVENT )
        {
          /* -- continuation event: output immediately ---------------------- */
	  /* -- dynamic traces, correct event id to global event id --------- */
	  erec->ev = GID(trcdes[source].nid, erec->ev);
          if ( reassembly )
          {
            output (outfd, ((char *) erec) + sizeof(short unsigned int),
                    trcdes[source].contlen < CONTLEN ?
                    trcdes[source].contlen : CONTLEN);
            trcdes[source].contlen -= CONTLEN;
          }
          else
            output (outfd, (char *) erec, sizeof(PCXX_EV));
          numrec++;
        }
        else
        {
          /* -- correct nid event field (only the first time) --------------- */
          if ( trcdes[source].nid != -1 ) erec->nid = trcdes[source].nid;

          /* -- correct clock ----------------------------------------------- */
          erec->ti += trcdes[source].offset;

          /* -- check clock overflow ---------------------------------------- */
          if ( erec->ti < trcdes[source].lasttime ) trcdes[source].overflows++;
          trcdes[source].lasttime = erec->ti;

          /* -- remember continuation event length -------------------------- */
          trcdes[source].contlen = erec->par;

        }
      }
    }
    while ( erec->ev == PCXX_EV_CONT_EVENT );
  }
  while ( active > 0 );
  for (i=0; i<numtrc; i++)
  { 
    if (outfd != STDOUT) 
    {
      fprintf (stderr, "%s: %ld records read.\n",
             trcdes[i].name, trcdes[i].numrec);
    }
  }

  output_flush (outfd);
  close (outfd);
  exit (0);
}
