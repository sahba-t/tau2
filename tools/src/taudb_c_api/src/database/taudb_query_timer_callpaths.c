#include "taudb_internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern TAUDB_TIMER_CALLPATH* taudb_query_timer_callpaths_2005(TAUDB_CONNECTION* connection, TAUDB_TRIAL* trial, TAUDB_TIMER* timer);
extern TAUDB_TIMER_CALLPATH* taudb_process_callpath_timer(TAUDB_TRIAL* trial, TAUDB_TIMER_CALLPATH* timer_callpath);
extern void taudb_trim(char * s);

TAUDB_TIMER_CALLPATH* taudb_query_timer_callpaths(TAUDB_CONNECTION* connection, TAUDB_TRIAL* trial, TAUDB_TIMER* timer) {
#ifdef TAUDB_DEBUG_DEBUG
  printf("Calling taudb_private_query_timer_callpaths(%p,%p)\n", trial, timer);
#endif
  int nFields;
  int i, j;

  if (trial == NULL) {
    fprintf(stderr, "Error: trial parameter null. Please provide a valid trial.\n");
    return NULL;
  }
  
  // if the Trial already has the callpath data, return it.
  if (trial->timer_callpaths_by_id != NULL) {
    taudb_numItems = HASH_CNT(hh1,trial->timer_callpaths_by_id);
    return trial->timer_callpaths_by_id;
  }

  if (taudb_version == TAUDB_2005_SCHEMA) {
    return taudb_query_timer_callpaths_2005(connection, trial, timer);
  } 

  taudb_begin_transaction(connection);
  /*
   * Fetch rows from table_name, the system catalog of databases
   */
  char my_query[1024];
  //sprintf(my_query,"select * from timer where trial = %d", trial->id);
  sprintf(my_query,"select tc.* from timer_callpath tc inner join timer t on tc.timer = t.id");
  if (timer != NULL) {
    sprintf(my_query,"%s inner join timer t on tc.timer = t.id", my_query);
  }
  sprintf(my_query,"%s where t.trial = %d", my_query, trial->id);
  if (timer != NULL) {
    sprintf(my_query,"%s and t.id = %d", my_query, timer->id);
  }
  sprintf(my_query,"%s order by parent", my_query);
#ifdef TAUDB_DEBUG
  printf("%s\n", my_query);
#endif
  taudb_execute_query(connection, my_query);

  int nRows = taudb_get_num_rows(connection);
  taudb_numItems = nRows;

  nFields = taudb_get_num_columns(connection);

  /* the rows */
  for (i = 0; i < taudb_get_num_rows(connection); i++)
  {
    TAUDB_TIMER_CALLPATH* timer_callpath = taudb_create_timer_callpaths(1);
    /* the columns */
    for (j = 0; j < nFields; j++) {
      if (strcmp(taudb_get_column_name(connection, j), "id") == 0) {
        timer_callpath->id = atoi(taudb_get_value(connection, i, j));
      } else if (strcmp(taudb_get_column_name(connection, j), "timer") == 0) {
        timer_callpath->timer = taudb_get_timer_by_id(trial->timers_by_id, atoi(taudb_get_value(connection, i, j)));
      } else if (strcmp(taudb_get_column_name(connection, j), "parent") == 0) {
        TAUDB_TIMER_CALLPATH* parent = NULL;
        int parent_id = atoi(taudb_get_value(connection, i, j));
        parent = taudb_get_timer_callpath_by_id(trial->timer_callpaths_by_id, parent_id);
		timer_callpath->parent = parent;
      } else {
        printf("Error: unknown column '%s'\n", taudb_get_column_name(connection, j));
        taudb_exit_nicely(connection);
      }
    } 
    timer_callpath->name = taudb_strdup(taudb_get_callpath_string(timer_callpath));
    HASH_ADD(hh1, trial->timer_callpaths_by_id, id, sizeof(int), timer_callpath);
    HASH_ADD_KEYPTR(hh2, trial->timer_callpaths_by_name, timer_callpath->name, strlen(timer_callpath->name), timer_callpath);
  }

  taudb_clear_result(connection);
  taudb_close_transaction(connection);

  return (trial->timer_callpaths_by_id);
}

// convenience method
TAUDB_TIMER_CALLPATH* taudb_query_all_timer_callpaths(TAUDB_CONNECTION* connection, TAUDB_TRIAL* trial) {
  return taudb_query_timer_callpaths(connection, trial, NULL);
}

char* taudb_get_callpath_string(TAUDB_TIMER_CALLPATH* timer_callpath) {
  char* full_string = taudb_strdup(timer_callpath->timer->name);
  TAUDB_TIMER_CALLPATH* parent = timer_callpath->parent;
  while (parent != NULL) {
    // resize for "string -> string" with null terminator
    int new_length = strlen(full_string) + strlen(parent->timer->name) + 5;
    char* tmp_full_string = taudb_strdup(full_string);
    full_string = (char*)realloc(full_string, new_length);
	sprintf(full_string, "%s => %s", parent->timer->name, tmp_full_string);
	free(tmp_full_string);
    parent = parent->parent;
  }
  return full_string;
}


TAUDB_TIMER_CALLPATH* taudb_query_timer_callpaths_2005(TAUDB_CONNECTION* connection, TAUDB_TRIAL* trial, TAUDB_TIMER* timer) {
  int nFields;
  int i, j;

  taudb_numItems = 0;

  /* iterate over the timers, and create flat profile callpaths for the timers */
  TAUDB_TIMER *current, *tmp;
  HASH_ITER(hh1, trial->timers_by_id, current, tmp) {
    TAUDB_TIMER_CALLPATH* timer_callpath = taudb_create_timer_callpaths(1);
    timer_callpath->id = current->id;
    timer_callpath->timer = current;
    timer_callpath->name = taudb_strdup(current->name);
    taudb_trim(timer_callpath->name);
    timer_callpath->parent = NULL;
    HASH_ADD(hh1, trial->timer_callpaths_by_id, id, sizeof(int), timer_callpath);
    HASH_ADD_KEYPTR(hh2, trial->timer_callpaths_by_name, timer_callpath->name, strlen(timer_callpath->name), timer_callpath);
    taudb_numItems++;
  }

  taudb_begin_transaction(connection);
  /*
   * Fetch rows from table_name, the system catalog of databases
   */
  char my_query[1024];
  sprintf(my_query,"select name, id from interval_event where trial = %d and name like '%% => %%'", trial->id);
  if (timer != NULL) {
    sprintf(my_query,"%s and name like '%%%s%%'", my_query, timer->name);
  }
#ifdef TAUDB_DEBUG
  printf("%s\n", my_query);
#endif
  taudb_execute_query(connection, my_query);

  int nRows = taudb_get_num_rows(connection);
  taudb_numItems += nRows;

  nFields = taudb_get_num_columns(connection);

  /* the rows */
  for (i = 0; i < taudb_get_num_rows(connection); i++)
  {
    TAUDB_TIMER_CALLPATH* timer_callpath = taudb_create_timer_callpaths(1);
    /* the columns */
    for (j = 0; j < nFields; j++) {
      if (strcmp(taudb_get_column_name(connection, j), "id") == 0) {
        timer_callpath->id = atoi(taudb_get_value(connection, i, j));
      } else if (strcmp(taudb_get_column_name(connection, j), "name") == 0) {
        timer_callpath->name = taudb_strdup(taudb_get_value(connection,i,j));
        taudb_trim(timer_callpath->name);
#ifdef TAUDB_DEBUG_DEBUG
        printf("Got timer_callpath '%s'\n", timer_callpath->name);
#endif
      }
    } 
    taudb_process_callpath_timer(trial, timer_callpath);
    free(timer_callpath);
  }

  taudb_clear_result(connection);
  taudb_close_transaction(connection);

  return (trial->timer_callpaths_by_id);
}

TAUDB_TIMER_CALLPATH* taudb_process_callpath_timer(TAUDB_TRIAL* trial, TAUDB_TIMER_CALLPATH* timer_callpath) {
#ifdef TAUDB_DEBUG
    printf("taudb_process_callpath_timer: '%s'\n", timer_callpath->name);
#endif
  // tokenize the string
  char* callpath = timer_callpath->name;
  char* tmp_callpath = (char*)calloc((strlen(timer_callpath->name))+1, sizeof(char));
  char* end = strstr(callpath, " => ");
  TAUDB_TIMER* parent_timer = NULL;
  TAUDB_TIMER_CALLPATH* parent_callpath = NULL;
  TAUDB_TIMER_CALLPATH* last_parent = NULL;
  while (end != NULL) {
    // get the parent timer name
    char* token = (char*)calloc((end-callpath)+1, sizeof(char));
    strncpy(token, callpath, end-callpath);
    //taudb_trim(token);
    // find the parent timer in the hash
    //parent_timer = taudb_get_timer_by_name(trial->timers_by_name, token);
    parent_timer = taudb_get_timer_by_name(trial->timers_by_name, token);
    if (parent_timer != NULL) {
#ifdef TAUDB_DEBUG
      printf("Parent timer: '%s', id: %d\n", token, parent_timer->id);
#endif
    } else {
	  // this is not unusual - the events may be out of order
      //fprintf(stderr, "Timer not found : '%s'\n", token);
	  parent_timer = taudb_create_timers(1);
	  parent_timer->name = taudb_strdup(token);
      HASH_ADD_KEYPTR(hh2, trial->timers_by_name, parent_timer->name, strlen(parent_timer->name), parent_timer);
    }

    // get the parent callpath
    strncpy(tmp_callpath, callpath, end-(timer_callpath->name));
    // find the parent callpath in the hash
	parent_callpath = taudb_get_timer_callpath_by_name(trial->timer_callpaths_by_name, tmp_callpath);
    if (parent_callpath != NULL) {
#ifdef TAUDB_DEBUG
      printf("Parent callpath: '%s', id: %d\n", tmp_callpath, parent_callpath->id);
#endif
    } else {
      // make the timer, and add it to the name hash
      parent_callpath = taudb_create_timer_callpaths(1);
      parent_callpath->timer = parent_timer;
      parent_callpath->name = taudb_strdup(tmp_callpath);
      parent_callpath->parent = last_parent;
      // set the id for top level timer
      if (last_parent == NULL) {
        parent_callpath->id = parent_timer->id;
      }
      HASH_ADD_KEYPTR(hh2, trial->timer_callpaths_by_name, parent_callpath->name, strlen(parent_callpath->name), parent_callpath);
    }
    last_parent = parent_callpath;

    // increment the string pointer
    callpath = end+4;
    // increment the start index
    end = strstr(callpath, " => ");
  }
#ifdef TAUDB_DEBUG
  printf("Leaf timer: '%s'\n", callpath);
#endif

  // now, handle the leaf. The leaf timer may already exist - check for it
  TAUDB_TIMER* leaf_timer = NULL;
  // find the leaf timer in the hash
  leaf_timer = taudb_get_timer_by_name(trial->timers_by_name, callpath);
  if (leaf_timer != NULL) {
#ifdef TAUDB_DEBUG
    printf("Leaf timer: '%s', id: %d\n", callpath, leaf_timer->id);
#endif
  } else {
    //fprintf(stderr, "Timer not found : '%s'\n", callpath);
	leaf_timer = taudb_create_timers(1);
	leaf_timer->name = taudb_strdup(callpath);
    HASH_ADD_KEYPTR(hh2, trial->timers_by_name, leaf_timer->name, strlen(leaf_timer->name), leaf_timer);
  }

  TAUDB_TIMER_CALLPATH* leaf_callpath = NULL;
  // get the leaf callpath
  strcpy(tmp_callpath, timer_callpath->name);
  // find the leaf callpath in the hash
  leaf_callpath = taudb_get_timer_callpath_by_name(trial->timer_callpaths_by_name, tmp_callpath);
  if (leaf_callpath != NULL) {
#ifdef TAUDB_DEBUG
    printf("Leaf callpath: '%s', id: %d\n", tmp_callpath, leaf_callpath->id);
#endif
  } else {
    // make the timer, and add it to the name hash
    leaf_callpath = taudb_create_timer_callpaths(1);
    leaf_callpath->id = timer_callpath->id;
    leaf_callpath->timer = leaf_timer;
    leaf_callpath->parent = last_parent;
    leaf_callpath->name = taudb_strdup(timer_callpath->name);
    if (leaf_callpath->id > 0) {
      HASH_ADD(hh1, trial->timer_callpaths_by_id, id, sizeof(int), leaf_callpath);
    }
    HASH_ADD_KEYPTR(hh2, trial->timer_callpaths_by_name, leaf_callpath->name, strlen(leaf_callpath->name), leaf_callpath);
  }
  return leaf_callpath;
}

TAUDB_TIMER_CALLPATH* taudb_get_timer_callpath_by_id(TAUDB_TIMER_CALLPATH* timer_callpaths, const int id) {
#ifdef TAUDB_DEBUG_DEBUG
  printf("Calling taudb_get_timer_callpath_by_id(%p,%d)\n", timer_callpaths, id);
#endif
  if (timer_callpaths == NULL) {
    fprintf(stderr, "Error: timer_callpath parameter null. Please provide a valid set of timer_callpaths.\n");
    return NULL;
  }
  if (id == 0) {
    fprintf(stderr, "Error: id parameter null. Please provide a valid name.\n");
    return NULL;
  }

  TAUDB_TIMER_CALLPATH* timer_callpath = NULL;
  HASH_FIND(hh1, timer_callpaths, &id, sizeof(int), timer_callpath);
#ifdef ITERATE_ON_FAILURE
  // HASH_FIND is not working so well... now we iterate. Sigh.
  if (timer_callpath == NULL) {
    TAUDB_TIMER_CALLPATH *current, *tmp;
    HASH_ITER(hh1, timer_callpaths, current, tmp) {
#ifdef TAUDB_DEBUG_DEBUG
      printf ("TIMER_CALLPATH: '%s'\n", current->name);
#endif
      if (current->id == id) {
        return current;
      }
    }
  }
#endif
  return timer_callpath;
}

TAUDB_TIMER_CALLPATH* taudb_get_timer_callpath_by_name(TAUDB_TIMER_CALLPATH* timer_callpaths, const char* name) {
#ifdef TAUDB_DEBUG_DEBUG
  printf("Calling taudb_get_timer_callpath_by_name(%p,%s)\n", timer_callpaths, name);
#endif
  if (timer_callpaths == NULL) {
    fprintf(stderr, "Error: timer_callpath parameter null. Please provide a valid set of timer_callpaths.\n");
    return NULL;
  }
  if (name == NULL) {
    fprintf(stderr, "Error: name parameter null. Please provide a valid name.\n");
    return NULL;
  }

  TAUDB_TIMER_CALLPATH* timer_callpath = NULL;
  HASH_FIND(hh2, timer_callpaths, name, strlen(name), timer_callpath);
#ifdef ITERATE_ON_FAILURE
  // HASH_FIND is not working so well... now we iterate. Sigh.
  if (timer_callpath == NULL) {
    TAUDB_TIMER_CALLPATH *current, *tmp;
    HASH_ITER(hh2, timer_callpaths, current, tmp) {
#ifdef TAUDB_DEBUG_DEBUG
      printf ("TIMER_CALLPATH: '%s'\n", current->name);
#endif
      if (strcmp(current->name, name) == 0) {
        return current;
      }
    }
  }
#endif
  return timer_callpath;
}


extern void taudb_save_timer_callpaths(TAUDB_CONNECTION* connection, TAUDB_TRIAL* trial, boolean update) {
    if(update) {
    	printf("Updating timer callpaths not supported yet.");
    }
	
	const char* my_query = "insert into timer_callpath (timer, parent) values ($1, $2);";
    const char* statement_name = "TAUDB_INSERT_TIMER_CALLPATH";
    taudb_prepare_statement(connection, statement_name, my_query, 2);
    TAUDB_TIMER_CALLPATH *timer_callpath, *tmp;
    HASH_ITER(hh2, trial->timer_callpaths_by_name, timer_callpath, tmp) {
      const char* paramValues[2] = {0};
      char timer_id[32] = {0};
      sprintf(timer_id, "%d", timer_callpath->timer->id);
      paramValues[0] = timer_id;
	  char parent_id[32] = {0};
	  if(timer_callpath->parent != NULL) {
	  	 sprintf(parent_id, "%d", timer_callpath->parent->id);
		 paramValues[1] = parent_id;
	  } else {
		 paramValues[1] = NULL;
	  }
	  
      taudb_execute_statement(connection, statement_name, 2, paramValues);
      taudb_execute_query(connection, "select currval('timer_callpath_id_seq');");

      int nRows = taudb_get_num_rows(connection);
      if (nRows == 1) {
        timer_callpath->id = atoi(taudb_get_value(connection, 0, 0));
        //printf("New Timer Callpath: %d\n", timer_callpath->id);
      } else {
        printf("Failed.\n");
      }
  	  taudb_close_query(connection);
    }
    taudb_clear_result(connection);
  
}
