/****************************************************************************
 **                     TAU Portable Profiling Package                     **
 **                     http://www.cs.uoregon.edu/research/tau             **
 *****************************************************************************
 **    Copyright 1997-2009                                                 **
 **    Department of Computer and Information Science, University of Oregon **
 **    Advanced Computing Laboratory, Los Alamos National Laboratory        **
 ****************************************************************************/
/***************************************************************************
 **     File            : TauMpiT.c                                       **
 **     Description     : TAU Profiling Package                           **
 **     Contact         : tau-bugs@cs.uoregon.edu                         **
 **     Documentation   : See http://www.cs.uoregon.edu/research/tau      **
 ***************************************************************************/

//////////////////////////////////////////////////////////////////////
// Include Files 
//////////////////////////////////////////////////////////////////////
#include <mpi.h>
#include <Profile/Profiler.h> 
#include <Profile/TauEnv.h> 
#include <Profile/TauMpiTTypes.h>
#include <Profile/TauPlugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//////////////////////////////////////////////////////////////////////
// Global variables
//////////////////////////////////////////////////////////////////////
static MPI_T_pvar_handle  *tau_pvar_handles; 
static MPI_T_pvar_session tau_pvar_session;
static MPI_T_cvar_handle *tau_cvar_handles;
static unsigned long long int **pvar_value_buffer;
static void *read_value_buffer; // values are read into this buffer.
static MPI_Datatype *tau_mpi_datatype;
static int *tau_pvar_count;
static int *tau_cvar_num_vals;
static int tau_initial_pvar_count = 0;
int num_cvars = 0; //For now, we don't support case where number of CVARS changes dynamically at runtime

//////////////////////////////////////////////////////////////////////
// externs
//////////////////////////////////////////////////////////////////////
extern void Tau_track_pvar_event(int current_pvar_index, int current_pvar_subindex, const int *tau_pvar_count, int num_pvars, double data);extern void Tau_disable_tracking_mpi_t(void);
extern void Tau_allocate_pvar_event(int num_pvars, const int *tau_pvar_count);
extern void *Tau_MemMgr_malloc(int tid, size_t size);
extern void Tau_MemMgr_free(int tid, void *addr, size_t size);

//Plugin related functions
extern PluginManager* plugin_manager;
extern void Tau_util_apply_role_hook(PluginManager* plugin_manager, const char* role_name, int argc, void** argv);

#define dprintf TAU_VERBOSE

//////////////////////////////////////////////////////////////////////
int Tau_mpi_t_initialize(void) {
  int return_val, thread_provided, num_pvars, i, namelen, verb, varclass, bind, threadsup;
  int index;
  int readonly, continuous, atomic;
  char event_name[TAU_NAME_LENGTH + 1] = "";
  int desc_len;
  char description[TAU_NAME_LENGTH + 1] = "";
  MPI_Datatype datatype;
  MPI_T_enum enumtype;

  /* if TAU_TRACK_MPI_T_PVARS is not set to true, return with a success but do nothing 
   * to initialize MPI_T PVAR interface*/
  dprintf("TAU: Before MPI_T initialization\n");
  if (TauEnv_get_track_mpi_t_pvars() == 0) {
    return MPI_SUCCESS; 
  } 

  /* Initialize MPI_T */
  return_val = MPI_T_init_thread(MPI_THREAD_SINGLE, &thread_provided);

  if (return_val != MPI_SUCCESS) 
  {
    perror("MPI_T_init_thread ERROR:");
    return return_val;
  }
  else {
    /* track a performance pvar session */
    return_val = MPI_T_pvar_session_create(&tau_pvar_session);
    if (return_val != MPI_SUCCESS) {
      perror("MPI_T_pvar_session_create ERROR:");
      return return_val;
    }  
  }

  dprintf("MPI_T initialized successfully!\n");
  /* get the number of pvars exported by the implmentation */
  return_val = MPI_T_pvar_get_num(&num_pvars);
  if (return_val != MPI_SUCCESS) {
    perror("MPI_T_pvar_get_num ERROR:");
    return return_val;
  }
  dprintf("TAU MPI_T STARTED session: pvars exposed = %d\n", num_pvars);

  pvar_value_buffer = (unsigned long long int**)malloc(sizeof(unsigned long long int*) * (num_pvars + 1));
  tau_mpi_datatype = (MPI_Datatype *) malloc(sizeof(MPI_Datatype *) * (num_pvars+1));
  tau_pvar_handles = (MPI_T_pvar_handle*)malloc(sizeof(MPI_T_pvar_handle) * (num_pvars + 1));
  tau_pvar_count = (int*)malloc(sizeof(int) * (num_pvars + 1));
  memset(tau_pvar_count, 0, sizeof(int) * (num_pvars + 1));
  read_value_buffer = (void*)malloc(sizeof(unsigned long long int) * (TAU_NAME_LENGTH + 1));

  /* Initialize variables. Get the names of performance variables */
  for(i = 0; i < num_pvars; i++) {
    namelen = desc_len = TAU_NAME_LENGTH;
    return_val = MPI_T_pvar_get_info(i/*IN*/,
    event_name /*OUT*/,
    &namelen /*INOUT*/,
    &verb /*OUT*/,
    &varclass /*OUT*/,
    &datatype /*OUT*/,
    &enumtype /*OUT*/,
    description /*description: OUT*/,
    &desc_len /*desc_len: INOUT*/,
    &bind /*OUT*/,
    &readonly /*OUT*/,
    &continuous /*OUT*/,
    &atomic/*OUT*/);
    tau_mpi_datatype[i] = datatype;

  /* allocate a pvar handle that will be used later */
  return_val = MPI_T_pvar_handle_alloc(tau_pvar_session, i, NULL, &tau_pvar_handles[i], &tau_pvar_count[i]);
  if (return_val != MPI_SUCCESS) {
    perror("MPI_T_pvar_handle_alloc ERROR:");
    return return_val;
  }

  /*Non-continuous variables need to be started before being read. If this is not done
   *TODO:Currently, the MVAPICH and MPICH implementations error out if non-continuous PVARs are not started before being read.
   *Check if this is expected behaviour from an MPI implementation. No mention of the need to do this from a clients perspective in the 3.1 standard.*/
  if(continuous == 0) {
    return_val = MPI_T_pvar_start(tau_pvar_session, tau_pvar_handles[i]);
    if (return_val != MPI_SUCCESS) {
      perror("MPI_T_pvar_start ERROR:");
      return return_val;
    }
  }

  /* and a buffer to store the results in */
  pvar_value_buffer[i] = (unsigned long long int*)malloc(sizeof(unsigned long long int) * (tau_pvar_count[i] + 1));
  dprintf("Name: %s (%s), i = %d, varclass = %d, bind = %d, readonly = %d, continuous = %d, atomic = %d\n", event_name, description, i, varclass, bind, readonly, continuous, atomic);

  }
  
  Tau_allocate_pvar_event(num_pvars, tau_pvar_count);

  tau_initial_pvar_count = num_pvars;

  return return_val;
}

/*Cleanup by freeing handles and session*/
int Tau_mpi_t_cleanup(void) {
  int return_val, num_pvars, i;

  for(i=0;i < num_cvars;i++) {
    return_val = MPI_T_cvar_handle_free(&(tau_cvar_handles[i]));
    if (return_val != MPI_SUCCESS) {
      printf("TAU: Can't free cvar handle for index %d in this MPI implementation\n", i);
      return return_val;
    }
  }

  /*Don't free any handles and handles if nobody is tracking anything!*/
  if (TauEnv_get_track_mpi_t_pvars() == 0) {
    return MPI_SUCCESS;
  }

  /* get number of pvars from MPI_T */
  /*return_val = MPI_T_pvar_get_num(&num_pvars);
  if (return_val != MPI_SUCCESS) {
    perror("MPI_T_pvar_get_num ERROR:");
    return return_val;
  }*/
  
  for(i=0; i < tau_initial_pvar_count; i++) {
    return_val = MPI_T_pvar_handle_free(tau_pvar_session, &(tau_pvar_handles[i]));
    if (return_val != MPI_SUCCESS) {
      perror("MPI_T_pvar_handle_free ERROR:");
      return return_val;
    }
  }
  
  return_val = MPI_T_pvar_session_free(&tau_pvar_session);
  if (return_val != MPI_SUCCESS) {
    perror("MPI_T_pvar_session_free ERROR:");
    return return_val;
  }
  
  Tau_disable_tracking_mpi_t();
  return return_val;
}

/*Iterates through all the cvars provided by the implementation and prints the name and description of each cvar*/
int Tau_mpi_t_print_all_cvar_desc(int num_cvars) {
  int i, name_len, desc_len, verbosity, binding, scope, rank, return_val;
  char name[TAU_NAME_LENGTH]= "";
  char desc[TAU_NAME_LENGTH]= "";
  char tau_metric_name[TAU_NAME_LENGTH]= "";

  rank = Tau_get_node();
  MPI_Datatype datatype;
  MPI_T_enum enumtype;

  /*Iterate through the entire list of cvars exposed and print the name and description*/
  for (i=0; i < num_cvars; i++) {
    name_len = desc_len = TAU_NAME_LENGTH;
    return_val = MPI_T_cvar_get_info(i, name, &name_len, &verbosity, &datatype, &enumtype, desc, &desc_len, &binding, &scope);
    if (return_val != MPI_SUCCESS) {
      printf("TAU: Rank %d: Can't get cvar info i=%d, num_cvars=%d\n", rank, i, num_cvars);
      return return_val;
    }
    // This code gets called before MPI_Init has taken place. 
    // rank is -1 when TAU is linked in (as opposed to tau_exec). 
    //if (rank == 0) {
      dprintf("CVAR[%d] = %s \t \t desc = %s\n", i, name, desc);
      sprintf(tau_metric_name, "MPI_T CVAR: %s", name);
      TAU_METADATA(tau_metric_name, desc);
    //} 
  }

  return MPI_SUCCESS;
}

/*Parses cvar string using strtok, and separates the tokens using "," as the delimiter*/
void Tau_mpi_t_parse_cvar_string(int num_cvars, const char *cvar_string, char **cvar_array, int *number_of_elements) {

  /*A copy of the cvar string needs to be created because strtok doesn't accept const */
  char *cvar_copy = Tau_MemMgr_malloc(Tau_get_thread(), (strlen(cvar_string)+1)*sizeof(char));
  char *token;
  int current_index = 0;
  int iter = 0;
  char *save_ptr;

  strcpy(cvar_copy, cvar_string);
  token = strtok_r(cvar_copy, ",", &save_ptr);

  /*Initialize the string pointers to NULL*/
  for(iter=0; iter<num_cvars; iter++) {
    cvar_array[iter] = NULL;
  }

  /*Handling case where only one cvar metric or value is provided*/
  if(token == NULL) {
    cvar_array[current_index] = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(char)*TAU_NAME_LENGTH);
    strcpy(cvar_array[current_index], cvar_string);
    *number_of_elements = 1;
  }

  /*Handling case where there are more than one cvar metric or values are provided. We choose to ignore
   *trailing "," */
  while(token != NULL && token != "") {
    cvar_array[current_index] = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(char)*TAU_NAME_LENGTH);
    strcpy(cvar_array[current_index], token);
    current_index = current_index + 1;
    *number_of_elements = current_index;

    token = strtok_r(NULL, ",", &save_ptr);
  }

  //Tau_MemMgr_free(Tau_get_thread(), cvar_copy);
}

/*Iterates through the current list of vectors to see if the current vector element being added is new or already exists
 * If new, creates a new vector element. If vector element already exists, it appends a key value pair to the list of the corresponding vector element with the same name*/
void Tau_mpi_t_add_vector_element(VectorControlVariable *vectors, char *vector_name, char *key, int index_into_value_array, int *current_vector, char **cvar_values) {
  int iter, counter;
  ListStringPair *current_string_pair;

  for(iter = 0; iter < *current_vector; iter++) {
    //Found a vector with the same name that already exits
    if(strcmp(vectors[iter].name, vector_name) == 0) {
      counter = 0;
      current_string_pair = vectors[iter].list;
      while(counter < vectors[iter].number_of_elements - 1) {
        current_string_pair = current_string_pair->link;
        counter = counter + 1;
      }
      current_string_pair->link = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(ListStringPair));
      current_string_pair = current_string_pair->link;
      strcpy(current_string_pair->pair.first, key);
      strcpy(current_string_pair->pair.second, cvar_values[index_into_value_array]);
      vectors[iter].number_of_elements = vectors[iter].number_of_elements + 1;
      return;
    }
  }
  //Vector with this name doesn't exist. Add it!
  strcpy(vectors[*current_vector].name, vector_name);
  vectors[*current_vector].number_of_elements = 1;
  vectors[*current_vector].list = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(ListStringPair));
  strcpy(vectors[*current_vector].list->pair.first, key);
  strcpy(vectors[*current_vector].list->pair.second, cvar_values[index_into_value_array]);
  vectors[*current_vector].list->link = NULL;
  current_string_pair = NULL;
  *current_vector = *current_vector + 1;

}
 
/*Counts number of scalar and vector metrics*/
void Tau_mpi_t_count_scalar_and_vector_metrics(const char *cvars, char **cvar_metrics, int num_cvar_metrics, int *num_scalars, int *num_vectors) {

  int iter, current_scalar, current_vector;
  char *vector_metric_names;
  char *temporary_string;
  char *temporary_string2;
  int position_of_open_bracket, position_of_close_bracket;

  *num_scalars = 0;
  *num_vectors = 0;
  current_scalar = current_vector = 0;

  temporary_string = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(char)*TAU_NAME_LENGTH);
  temporary_string2 = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(char)*TAU_NAME_LENGTH);
  vector_metric_names = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(char)*strlen(cvars));

  /*Iterates through all cvar metrics to find out how many scalar and vector elements are present. Vector elements in the cvar metrics 
   * string are considered unique only if they have different names*/
  for(iter=0; iter < num_cvar_metrics; iter++) {
    if(strchr(cvar_metrics[iter],'[') != NULL) {
        position_of_open_bracket = strcspn(cvar_metrics[iter], "[");
        strncpy(temporary_string, cvar_metrics[iter], position_of_open_bracket);
        temporary_string[position_of_open_bracket] = ',';
        temporary_string[position_of_open_bracket+1] = '\0';
        if(strstr(vector_metric_names,temporary_string) == NULL) {
          *num_vectors = *num_vectors + 1;
        }
        strcat(vector_metric_names, temporary_string);
    } else {
      *num_scalars = *num_scalars + 1;
    }
  }

  ///Tau_MemMgr_free(Tau_get_thread(), temporary_string); Tau_MemMgr_free(Tau_get_thread(), temporary_string2); Tau_MemMgr_free(Tau_get_thread(), vector_metric_names);
}

/*Separates out scalar and vector elements into two different lists for ease of use*/
void Tau_mpi_t_map_cvar_metrics_to_values(char **cvar_metrics, char **cvar_values, int num_cvar_metrics, VectorControlVariable *vectors,ScalarControlVariable *scalars, int num_scalars, int num_vectors) {
  int iter, current_scalar, current_vector;
  char *vector_metric_names;
  char *temporary_string;
  char *temporary_string2;
  int position_of_open_bracket, position_of_close_bracket;

  current_scalar = current_vector = 0;

  temporary_string = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(char)*TAU_NAME_LENGTH);
  temporary_string2 = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(char)*TAU_NAME_LENGTH);

  /*Logic for separating the scalars and vectors into two separate lists*/
  for(iter=0; iter < num_cvar_metrics; iter++) {
    if(strchr(cvar_metrics[iter],'[') == NULL) {
      strcpy(scalars[current_scalar].name, cvar_metrics[iter]);
      strcpy(scalars[current_scalar].value, cvar_values[iter]);
      current_scalar = current_scalar + 1;
    } else {
        position_of_open_bracket = strcspn(cvar_metrics[iter], "[");
        position_of_close_bracket = strcspn(cvar_metrics[iter], "]");
        strncpy(temporary_string, cvar_metrics[iter], position_of_open_bracket); //Copy in the name of the vector
        strncpy(temporary_string2, &cvar_metrics[iter][position_of_open_bracket+1], position_of_close_bracket - position_of_open_bracket - 1); //Copy in the number contained with the [].
        temporary_string[position_of_open_bracket] = '\0';
        temporary_string2[position_of_close_bracket - position_of_open_bracket - 1] = '\0';
        Tau_mpi_t_add_vector_element(vectors, temporary_string, temporary_string2, iter, &current_vector, cvar_values);
    }
  }

  //Tau_MemMgr_free(Tau_get_thread(), temporary_string); Tau_MemMgr_free(Tau_get_thread(), temporary_string2);
}

/*Type the read, write and reset_value buffers depending on the MPI_Datatype of the cvar*/
void Tau_mpi_t_allocate_read_write_buffers(void **val, void **oldval, void **reset_value, MPI_Datatype datatype, int num_vals) {

  if(datatype == MPI_UNSIGNED || datatype == MPI_UNSIGNED_LONG || datatype == MPI_UNSIGNED_LONG_LONG || datatype == MPI_COUNT) {
    *val = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(unsigned long long));
    *oldval = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(unsigned long long));
    *reset_value = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(unsigned long long));
  }

  if(datatype == MPI_INT) {
    *val = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(signed int));
    *oldval = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(signed int));
    *reset_value = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(signed int));
  }

  if(datatype == MPI_CHAR) {
    *val = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(char));
    *oldval = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(char));
    *reset_value = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(char));
  }

  if(datatype == MPI_DOUBLE) {
    *val = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(double));
    *oldval = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(double));
    *reset_value = Tau_MemMgr_malloc(Tau_get_thread(), num_vals*sizeof(double));
  }
}

/*This function verifies that the scalar cvar metric has been written correctly by reading and checking against the reset_value. Logs metadata*/
void Tau_mpi_t_verify_write_and_log_scalar_metadata(int rank, void *val, void *reset_value, void *oldval, MPI_Datatype datatype, int num_vals, char *metric_name, char desc[TAU_NAME_LENGTH]) {
  char metastring[TAU_NAME_LENGTH] = "";
  char tau_metric_name[TAU_NAME_LENGTH] = "";

  if(datatype == MPI_UNSIGNED || datatype == MPI_UNSIGNED_LONG || datatype == MPI_UNSIGNED_LONG_LONG || datatype == MPI_COUNT) {
    if ((rank == 0) && (*(unsigned long long*)reset_value == *(unsigned long long*)val)) {
      dprintf("ResetValue %lld matches what we set for cvars=%s\n", *(unsigned long long*)reset_value, metric_name);
      sprintf(metastring,"%lld (old) -> %lld (new), %s", *(unsigned long long*)oldval, *(unsigned long long*)reset_value, desc);
    }
  } else if(datatype == MPI_INT) {
    if ((rank == 0) && (*(signed int*)reset_value == *(signed int*)val)) {
      dprintf("ResetValue %d matches what we set for cvars=%s\n", *(signed int*)reset_value, metric_name);
      sprintf(metastring,"%d (old) -> %d (new), %s", *(signed int*)oldval, *(signed int*)reset_value, desc);
    }
  } else if(datatype == MPI_DOUBLE) {
    if ((rank == 0) && (*(double*)reset_value == *(double*)val)) {
      dprintf("ResetValue %lf matches what we set for cvars=%s\n", *(double*)reset_value, metric_name);
      sprintf(metastring,"%lf (old) -> %lf (new), %s", *(double*)oldval, *(double*)reset_value, desc);
    }
  } else if(datatype == MPI_CHAR) {
    if ((rank == 0) && (strcmp((char*)reset_value, (char*)val) == 0)) {
      dprintf("ResetValue %s matches what we set for cvars=%s\n", (char *)reset_value, metric_name);
      sprintf(metastring,"%s (old) -> %s (new), %s", (char *)oldval, (char *)reset_value, desc);
    }
  }
 
  if (rank == 0) {
    sprintf(tau_metric_name, "MPI_T CVAR: %s", metric_name);
    TAU_METADATA(tau_metric_name, metastring);
  }
}

/*This function verifies that the vector cvar metric has been written correctly by reading and checking against the reset_value. Logs metadata*/
void Tau_mpi_t_verify_write_and_log_vector_metadata(int rank, void *val, void *reset_value, void *oldval, MPI_Datatype datatype, int num_vals, char *metric_name) {

  int is_reset_value_equal_to_written_value = 1;
  int iter;
  char old_value_string[TAU_NAME_LENGTH] = "";
  char reset_value_string[TAU_NAME_LENGTH] = "";
  char metastring[TAU_NAME_LENGTH] = "";
  char metric_name_with_array_index[TAU_NAME_LENGTH] = "";

/*First check the datatype of the values, and then accordingly log metadata*/
  if(datatype == MPI_UNSIGNED || datatype == MPI_UNSIGNED_LONG || datatype == MPI_UNSIGNED_LONG_LONG || datatype == MPI_COUNT) {
    for(iter=0; iter < num_vals; iter++) {
      if(((unsigned long long*)reset_value)[iter] != ((unsigned long long*)val)[iter]) {
        is_reset_value_equal_to_written_value = 0;
        break;
      }
    }
    if((rank == 0) && is_reset_value_equal_to_written_value) {
      dprintf("ResetValue matches what we set for cvars=%s\n", metric_name);
      for(iter=0; iter < num_vals; iter++) {
        if(((unsigned long long*)oldval)[iter] != ((unsigned long long*)reset_value)[iter]) {
          sprintf(old_value_string,"%lld,",((unsigned long long*)oldval)[iter]);
          sprintf(reset_value_string,"%lld,",((unsigned long long*)reset_value)[iter]);
          sprintf(metastring,"%s (old) -> %s (new)", old_value_string, reset_value_string);
          sprintf(metric_name_with_array_index,"MPI_T CVAR: %s[%d]", metric_name, iter);
          TAU_METADATA(metric_name_with_array_index, metastring);
          strcpy(old_value_string, ""); strcpy(reset_value_string, ""); strcpy(metastring, ""); strcpy(metric_name_with_array_index, "");
        }
      }
    }
  } else if(datatype == MPI_INT) {
    for(iter=0; iter < num_vals; iter++) {
      if(((signed int*)reset_value)[iter] != ((signed int*)val)[iter]) {
        is_reset_value_equal_to_written_value = 0;
        break;
      }
    }
    if((rank == 0) && is_reset_value_equal_to_written_value) {
      dprintf("ResetValue matches what we set for cvars=%s\n", metric_name);
      for(iter=0; iter < num_vals; iter++) {
        if(((signed int*)oldval)[iter] != ((signed int*)reset_value)[iter]) {
          sprintf(old_value_string,"%lld,",((signed int*)oldval)[iter]);
          sprintf(reset_value_string,"%lld,",((signed int*)reset_value)[iter]);
          sprintf(metastring,"%s (old) -> %s (new)", old_value_string, reset_value_string);
          sprintf(metric_name_with_array_index,"MPI_T CVAR: %s[%d]", metric_name, iter);
          TAU_METADATA(metric_name_with_array_index, metastring);
          strcpy(old_value_string, ""); strcpy(reset_value_string, ""); strcpy(metastring, ""); strcpy(metric_name_with_array_index, "");
        }
      }
    }
  } else if(datatype == MPI_DOUBLE) {
    for(iter=0; iter < num_vals; iter++) {
      if(((double*)reset_value)[iter] != ((double*)val)[iter]) {
        is_reset_value_equal_to_written_value = 0;
        break;
      }
    }
    if((rank == 0) && is_reset_value_equal_to_written_value) {
      dprintf("ResetValue matches what we set for cvars=%s\n", metric_name);
      for(iter=0; iter < num_vals; iter++) {
        if(((double*)oldval)[iter] != ((double*)reset_value)[iter]) {
          sprintf(old_value_string,"%lld,",((double*)oldval)[iter]);
          sprintf(reset_value_string,"%lld,",((double*)reset_value)[iter]);
          sprintf(metastring,"%s (old) -> %s (new)", old_value_string, reset_value_string);
          sprintf(metric_name_with_array_index,"MPI_T CVAR: %s[%d]", metric_name, iter);
          TAU_METADATA(metric_name_with_array_index, metastring);
          strcpy(old_value_string, ""); strcpy(reset_value_string, ""); strcpy(metastring, ""); strcpy(metric_name_with_array_index, "");
        }
      }
    }
  }
}
/*Convert a string to a typed value, depending on the MPI_Datatype.*/
void Tau_mpi_t_convert_string_to_typed_value(char *string_value, MPI_Datatype datatype, void *val, int index) {
  if(datatype == MPI_UNSIGNED || datatype == MPI_UNSIGNED_LONG || datatype == MPI_UNSIGNED_LONG_LONG || datatype == MPI_COUNT) {
    sscanf(string_value, "%lld", &((unsigned long long*)val)[index]);
  } else if(datatype == MPI_INT) {
    sscanf(string_value, "%d", &((signed int*)val)[index]);
  } else if(datatype == MPI_CHAR) {
    sscanf(string_value, "%s", &((char*)val)[index]);
  } else if(datatype == MPI_DOUBLE) {
    sscanf(string_value, "%lf", &((double*)val)[index]);
  }
}

int Tau_mpi_t_parse_and_write_cvars(const char *cvars, const char *values) {

      int cindex, num_cvar_metrics, num_cvar_values, num_scalars, num_vectors, array_index, thread_provided;
      int return_val, iter, rank;
       
      char name[TAU_NAME_LENGTH]= ""; 
      char desc[TAU_NAME_LENGTH]= ""; 
      int verbosity, binding, scope, i, j, k;
      int name_len;
      int desc_len;
      MPI_Datatype datatype; 
      MPI_T_enum enumtype; 
      char **cvar_metrics_array, **cvar_values_array;
      VectorControlVariable *vectors;
      ScalarControlVariable *scalars;
      void *val, *oldval, *reset_value;
      ListStringPair *current_string_pair;

      rank = Tau_get_node(); /* MPI_Init may not been invoked yet */

      /*Initialize the MPI_T interface in case we have not done so already. No harm in re-initializing the interface.*/
      return_val = MPI_T_init_thread(MPI_THREAD_SINGLE, &thread_provided);

      if (return_val != MPI_SUCCESS) {
        perror("MPI_T_init_thread ERROR:");
        return return_val;
      }

      cvar_metrics_array = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(char*)*num_cvars);
      cvar_values_array = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(char*)*num_cvars);

      /*Parse the metrics and values using strtok. At this stage, we aren't concerned with differenting between scalar and vector metrics.*/
      Tau_mpi_t_parse_cvar_string(num_cvars, cvars, cvar_metrics_array, &num_cvar_metrics);
      Tau_mpi_t_parse_cvar_string(num_cvars, values, cvar_values_array, &num_cvar_values);
   

      /*Checking if number of cvar metrics and cvar values provided are equal*/
      if(num_cvar_metrics != num_cvar_values) { 
        printf("TAU: Rank %d: Number of CVAR metrics don't match number of CVAR values provided. Please check environment variables TAU_MPI_T_CVAR_METRICS and TAU_MPI_T_CVAR_VALUES", rank);
	return return_val;
      }
 
      /*Counts the number of scalar and vector metrics*/ 
      Tau_mpi_t_count_scalar_and_vector_metrics(cvars, cvar_metrics_array, num_cvar_metrics, &num_scalars, &num_vectors);


      scalars = Tau_MemMgr_malloc(Tau_get_thread(), (num_scalars)*sizeof(ScalarControlVariable));
      vectors = Tau_MemMgr_malloc(Tau_get_thread(), (num_vectors)*sizeof(VectorControlVariable));

      /*Maps cvar metrics to values, handling both scalars and vectors appropriately*/
      Tau_mpi_t_map_cvar_metrics_to_values(cvar_metrics_array, cvar_values_array, num_cvar_metrics, vectors, scalars, num_scalars, num_vectors);
      
      for (i=0; i < num_cvars; i++) {
        name_len = desc_len = TAU_NAME_LENGTH;
        return_val = MPI_T_cvar_get_info(i, name, &name_len, &verbosity, &datatype, &enumtype, desc, &desc_len, &binding, &scope);
        if (return_val != MPI_SUCCESS) {
	  printf("TAU: Rank %d: Can't get cvar info i=%d, num_cvars=%d\n", rank, i, num_cvars);
	  return return_val; 
        }
        /*Iterate through all the scalar metrics*/
        for(iter=0;iter < num_scalars; iter++) {
          if (strcmp(name, scalars[iter].name)==0) {
            if (rank == 0) {
              dprintf("Rank: %d FOUND CVAR match: %s, cvars = %s, desc = %s\n", rank, name, scalars[iter].name, desc);
            }
            cindex = i;
            /*This check is in place to handle the case where user inputs scalar values for a cvar that is a vector, except for a variable of type MPI_CHAR*/
            if(tau_cvar_num_vals[cindex] != 1 && datatype != MPI_CHAR) { 
              printf("TAU: Rank %d: CVAR %s assumed scalar by user, but is actually a vector\n", rank, scalars[iter].name);
              return return_val;
            }
 
            Tau_mpi_t_allocate_read_write_buffers(&val, &oldval, &reset_value, datatype, tau_cvar_num_vals[cindex]);

            return_val = MPI_T_cvar_read(tau_cvar_handles[cindex], oldval);
            if (return_val != MPI_SUCCESS) {
               printf("TAU: Rank %d: Can't read cvar %s in this MPI implementation\n", rank, scalars[iter].name);
               return return_val;
            } else {
              if (rank == 0) {
                //TODO: Print old values based on type dprintf("Oldval = %d, newval=%lld, cvars=%s\n", oldval, val, cvar_metrics_array[iter]);
              }
            }
           
            array_index = 0; //for scalars, array index into the val array is 0 as there is only one element
            Tau_mpi_t_convert_string_to_typed_value(scalars[iter].value, datatype, val, array_index);

            return_val = MPI_T_cvar_write(tau_cvar_handles[cindex], val);
            if (return_val != MPI_SUCCESS) {
               printf("TAU: Rank %d: Can't write cvar %s in this MPI implementation\n", rank, scalars[iter].name);
               return return_val;
            }
            return_val = MPI_T_cvar_read(tau_cvar_handles[cindex], reset_value);
            if (return_val != MPI_SUCCESS) {
              printf("TAU: Rank %d: Can't read cvar %s in this MPI implementation\n", rank, scalars[iter].name);
              return return_val;
            } else {
              Tau_mpi_t_verify_write_and_log_scalar_metadata(rank, val, reset_value, oldval, datatype, tau_cvar_num_vals[cindex], scalars[iter].name, desc);
            }
            //Tau_MemMgr_free(Tau_get_thread(), val); Tau_MemMgr_free(Tau_get_thread(), oldval); Tau_MemMgr_free(Tau_get_thread(), reset_value);
         
          }
        }

        /*Iterate through vector metrics*/
        for(iter=0;iter < num_vectors; iter++) {
          if (strcmp(name, vectors[iter].name)==0) {
            if (rank == 0) {
              dprintf("Rank: %d FOUND CVAR match: %s, cvars = %s, desc = %s\n", rank, name, vectors[iter].name, desc);
            }
            cindex = i;

            Tau_mpi_t_allocate_read_write_buffers(&val, &oldval, &reset_value, datatype, tau_cvar_num_vals[cindex]);

            return_val = MPI_T_cvar_read(tau_cvar_handles[cindex], oldval);
            
            return_val = MPI_T_cvar_read(tau_cvar_handles[cindex], val);

            if (return_val != MPI_SUCCESS) {
               printf("TAU: Rank %d: Can't read cvar %s in this MPI implementation\n", rank, vectors[iter].name);
               return return_val;
            } else {
               //TODO: Print old values based on type
            }
           
            current_string_pair = vectors[iter].list;
            k = 0;
            while(k < vectors[iter].number_of_elements) {
              sscanf(current_string_pair->pair.first, "%d", &array_index);
              Tau_mpi_t_convert_string_to_typed_value(current_string_pair->pair.second, datatype, val, array_index);
              current_string_pair = current_string_pair->link;
              k = k + 1;
            }

            return_val = MPI_T_cvar_write(tau_cvar_handles[cindex], val);
            if (return_val != MPI_SUCCESS) {
               printf("TAU: Rank %d: Can't write cvar %s in this MPI implementation\n", rank, vectors[iter].name);
               return return_val;
            }
            return_val = MPI_T_cvar_read(tau_cvar_handles[cindex], reset_value);

            if (return_val != MPI_SUCCESS) {
              printf("TAU: Rank %d: Can't read cvar %s in this MPI implementation\n", rank, vectors[iter].name);
              return return_val;
            } else {
              Tau_mpi_t_verify_write_and_log_vector_metadata(rank, val, reset_value, oldval, datatype, tau_cvar_num_vals[cindex], vectors[iter].name);
            }
            //Tau_MemMgr_free(Tau_get_thread(), val); Tau_MemMgr_free(Tau_get_thread(), oldval); Tau_MemMgr_free(Tau_get_thread(), reset_value);

          }
        }

      /* NOT implemented: return_val = MPI_T_cvar_get_index(cvars, &cindex);
      if (return_val != MPI_SUCCESS) { 
        printf("TAU: Rank %d: Can't access MPI_T variable %s in this MPI implementation\n", Tau_get_node(), cvars);
        return return_val;
      }
      */
      }  

      //Tau_MemMgr_free(Tau_get_thread(), cvar_metrics_array); Tau_MemMgr_free(Tau_get_thread(), cvar_values_array); Tau_MemMgr_free(Tau_get_thread(), vectors); Tau_MemMgr_free(Tau_get_thread(), scalars);

      /*Finalize the MPI_T since we called MPI_T_init_thread above. This ensure the tools interface is back to the state
       *state it was in before this function call.*/
      /*return_val = MPI_T_finalize();
      if (return_val != MPI_SUCCESS) {
        printf("TAU: Rank %d: Call to MPI_T_finalize failed\n", rank);
        return return_val;
      }*/

      return return_val;
}

/*Read CVARs to be set and their values from the environment and write them to the MPI_T interface */
int Tau_mpi_t_cvar_initialize(void) {
  int return_val, iter, thread_provided, rank, i;
  const char *cvars = TauEnv_get_cvar_metrics();
  const char *values = TauEnv_get_cvar_values();
  char metastring[TAU_NAME_LENGTH] = "";

  /* MPI_Comm_rank(MPI_COMM_WORLD, &rank); */
  rank = Tau_get_node(); /* MPI_Init has not been invoked yet */

  /*Initialize the MPI_T interface in case we have not done so already. This can happen when performance variables are not being tracked.*/
  return_val = MPI_T_init_thread(MPI_THREAD_SINGLE, &thread_provided);

  if (return_val != MPI_SUCCESS) {
    perror("MPI_T_init_thread ERROR:");
    return return_val;
  }

  /* get number of cvars from MPI_T */
  return_val = MPI_T_cvar_get_num(&num_cvars);
  if (return_val != MPI_SUCCESS) {
    perror("MPI_T_cvar_get_num ERROR:");
    return return_val;
  }

  tau_cvar_handles = Tau_MemMgr_malloc(Tau_get_thread(), num_cvars*sizeof(MPI_T_cvar_handle));
  tau_cvar_num_vals = Tau_MemMgr_malloc(Tau_get_thread(), num_cvars*sizeof(int));

  for(i=0;i < num_cvars;i++) {
    return_val = MPI_T_cvar_handle_alloc(i, NULL, &(tau_cvar_handles[i]), &(tau_cvar_num_vals[i]));
    if (return_val != MPI_SUCCESS) {
      printf("TAU: Can't allocate cvar handle for index %d in this MPI implementation\n", i);
      return return_val;
    }
  }

  return_val = Tau_mpi_t_print_all_cvar_desc(num_cvars);
  if(return_val != MPI_SUCCESS) {
    printf("TAU: Rank %d: Can't read the MPI_T control variables in this MPI implementation\n", rank);
    return return_val;
  }

  if (strcmp(cvars, "") == 0) {
    dprintf("TAU: No CVARS specified using TAU_MPI_T_CVAR_METRICS and TAU_MPI_T_CVAR_VALUES\n");

  } else {
    dprintf("CVAR_METRICS=%s\n", cvars);
    if (strcmp(values, "") == 0) {
      printf("TAU: WARNING: Environment variable TAU_MPI_T_CVAR_METRICS is not specified for TAU_MPI_T_CVAR_METRICS=%s\n", 
	cvars);
    } else { // both cvars and values are specified*/
      /*Parse the cvars and values and write to the MPI_T interface */
      return_val = Tau_mpi_t_parse_and_write_cvars(cvars, values);

      TAU_METADATA("TAU_MPI_T_CVAR_METRICS", cvars);
      TAU_METADATA("TAU_MPI_T_CVAR_VALUES", values);
      sprintf(metastring, "%d", TauEnv_get_track_mpi_t_pvars());
      TAU_METADATA("TAU_TRACK_MPI_T_PVARS", metastring);
    }
  }

  return return_val; 
}

/*Implement user based CVAR tuning policy based on a policy file (?)
 * TODO: This tuning logic should be in a separate module/file. Currently implementing hard-coded policies for MVAPICH meant only for experimentation purposes*/
void Tau_enable_user_cvar_tuning_policy(const int num_pvars, int *tau_pvar_count, unsigned long long int **pvar_value_buffer) {
  int return_val, i, namelen, verb, varclass, bind, threadsup;
  int index;
  int readonly, continuous, atomic;
  char event_name[TAU_NAME_LENGTH + 1] = "";
  char metric_string[TAU_NAME_LENGTH], value_string[TAU_NAME_LENGTH];
  int desc_len;
  char description[TAU_NAME_LENGTH + 1] = "";
  MPI_Datatype datatype;
  MPI_T_enum enumtype;
  static int firsttime = 1;
  static unsigned long long int *reduced_value_array = NULL;
  static char *reduced_value_cvar_string = NULL;
  static char *reduced_value_cvar_value_string = NULL;

  /*MVAPICH specific thresholds and names*/
  char PVAR_MAX_VBUF_USAGE[TAU_NAME_LENGTH] = "mv2_vbuf_max_use_array";
  char PVAR_VBUF_ALLOCATED[TAU_NAME_LENGTH] = "mv2_vbuf_allocated_array";
  int PVAR_VBUF_WASTED_THRESHOLD = 10; //This is the threshold above which we will be free from the pool

  char CVAR_ENABLING_POOL_CONTROL[TAU_NAME_LENGTH] = "MPIR_CVAR_VBUF_POOL_CONTROL";
  char CVAR_SPECIFYING_REDUCED_POOL_SIZE[TAU_NAME_LENGTH] = "MPIR_CVAR_VBUF_POOL_REDUCED_VALUE";

  int pvar_max_vbuf_usage_index, pvar_vbuf_allocated_index, has_threshold_been_breached_in_any_pool;
  pvar_max_vbuf_usage_index = -1;
  pvar_vbuf_allocated_index = -1;
  has_threshold_been_breached_in_any_pool = 0;

 if(firsttime) {
  firsttime = 0;
  for(i = 0; i < num_pvars; i++){
      namelen = desc_len = TAU_NAME_LENGTH;
      return_val = MPI_T_pvar_get_info(i/*IN*/,
      event_name /*OUT*/,
      &namelen /*INOUT*/,
      &verb /*OUT*/,
      &varclass /*OUT*/,
      &datatype /*OUT*/,
      &enumtype /*OUT*/,
      description /*description: OUT*/,
      &desc_len /*desc_len: INOUT*/,
      &bind /*OUT*/,
      &readonly /*OUT*/,
      &continuous /*OUT*/,
      &atomic/*OUT*/);

      if(strcmp(event_name, PVAR_MAX_VBUF_USAGE) == 0) {
        pvar_max_vbuf_usage_index = i;
      } else if (strcmp(event_name, PVAR_VBUF_ALLOCATED) == 0) {
        pvar_vbuf_allocated_index = i;
      }
      reduced_value_array = (unsigned long long int *)calloc(sizeof(unsigned long long int), tau_pvar_count[pvar_max_vbuf_usage_index]);
      reduced_value_cvar_string = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(char)*TAU_NAME_LENGTH);
      strcpy(reduced_value_cvar_string, "");
      reduced_value_cvar_value_string = Tau_MemMgr_malloc(Tau_get_thread(), sizeof(char)*TAU_NAME_LENGTH);
      strcpy(reduced_value_cvar_value_string, "");
  }

  if((pvar_max_vbuf_usage_index == -1) || (pvar_vbuf_allocated_index == -1)) {
    printf("Unable to find the indexes of PVARs required for tuning\n");
    return;
  } else {
    dprintf("Index of %s is %d and index of %s is %d\n", PVAR_MAX_VBUF_USAGE, pvar_max_vbuf_usage_index, PVAR_VBUF_ALLOCATED, pvar_vbuf_allocated_index);
  }
 }
  /*Tuning logic: If the difference between allocated vbufs and max use vbufs in a given
  * vbuf pool is higher than a set threshhold, then we will free from that pool.*/
  for(i = 0 ; i < tau_pvar_count[pvar_max_vbuf_usage_index]; i++) {
    if(pvar_value_buffer[pvar_max_vbuf_usage_index][i] > 1000) pvar_value_buffer[pvar_max_vbuf_usage_index][i] = 0; /*HACK - we are getting garbage values for pool2. Doesn't seem to be an issue in TAU*/

    if((pvar_value_buffer[pvar_vbuf_allocated_index][i] - pvar_value_buffer[pvar_max_vbuf_usage_index][i]) > PVAR_VBUF_WASTED_THRESHOLD) {
      has_threshold_been_breached_in_any_pool = 1;
      reduced_value_array[i] = pvar_value_buffer[pvar_max_vbuf_usage_index][i];
      dprintf("Threshold breached: Max usage for %d pool is %llu but vbufs allocated are %llu\n", i, pvar_value_buffer[pvar_max_vbuf_usage_index][i], pvar_value_buffer[pvar_vbuf_allocated_index][i]);
    } else {
      reduced_value_array[i] = pvar_value_buffer[pvar_vbuf_allocated_index][i] + 10; //Some value higher than current allocated
    }

    if(i == (tau_pvar_count[pvar_max_vbuf_usage_index])) {
      sprintf(metric_string,"%s[%d]", CVAR_SPECIFYING_REDUCED_POOL_SIZE, i);
      sprintf(value_string,"%llu", reduced_value_array[i]);
    } else {
      sprintf(metric_string,"%s[%d],", CVAR_SPECIFYING_REDUCED_POOL_SIZE, i);
      sprintf(value_string,"%llu,", reduced_value_array[i]);
    }
    
    strcat(reduced_value_cvar_string, metric_string);
    strcat(reduced_value_cvar_value_string, value_string);

  }

  if(has_threshold_been_breached_in_any_pool) {
    sprintf(metric_string,"%s,%s", CVAR_ENABLING_POOL_CONTROL, reduced_value_cvar_string);
    sprintf(value_string,"%d,%s", 1, reduced_value_cvar_value_string);
    dprintf("Metric string is %s and value string is %s\n", metric_string, value_string);
    Tau_mpi_t_parse_and_write_cvars(metric_string, value_string);
  } else {
    sprintf(metric_string,"%s", CVAR_ENABLING_POOL_CONTROL);
    sprintf(value_string,"%d", 0);
    dprintf("Metric string is %s and value string is %s\n", metric_string, value_string);
    Tau_mpi_t_parse_and_write_cvars(metric_string, value_string);
  }
}

//////////////////////////////////////////////////////////////////////
int Tau_track_mpi_t_here(void) {
  int return_val, num_pvars, i;
  int index;
  MPI_Datatype datatype;
  MPI_T_enum enumtype;
  int mpi_t_enable_user_tuning_policy = 0;
  
  /* if TAU_TRACK_MPI_T_PVARS is not set to true, return with a success but do nothing 
   * to process MPI_T events */
  if (TauEnv_get_track_mpi_t_pvars() == 0) {
    return MPI_SUCCESS; 
  } 

  if(TauEnv_get_mpi_t_enable_user_tuning_policy() == 1) {
    mpi_t_enable_user_tuning_policy = 1;
  }

  /* get number of pvars from MPI_T */
  return_val = MPI_T_pvar_get_num(&num_pvars);
  if (return_val != MPI_SUCCESS) {
    perror("MPI_T_pvar_get_num ERROR:");
    return return_val;
  }

  /* The MPI library can dynamically increase the number of PVARs exposed during runtime
   * We don't currently support this feature because we need to re-allocate PVAR handles 
   * and other data-structures inside a signal-handler, and calling malloc inside a 
   * signal handler is unsafe. 
   * Until we find an alternate mechanism, we will just read the number of pvars that
   * were exposed during  Tau_mpi_t_initialize() */
  if (num_pvars > tau_initial_pvar_count) {
    dprintf("TAU: Number of PVARS increased from %d to %d. Only reading %d PVARS\n", tau_initial_pvar_count, num_pvars, tau_initial_pvar_count);

  }

  int rank = Tau_get_node(); 
  int size, j; 

  for(i = 0; i < tau_initial_pvar_count; i++) {
    // get data from event
    //printf("Reading data for PVAR %d\n",i);
    MPI_T_pvar_read(tau_pvar_session, tau_pvar_handles[i], read_value_buffer);
    MPI_Type_size(tau_mpi_datatype[i], &size); 

    for(j = 0; j < tau_pvar_count[i]; j++) {
      pvar_value_buffer[i][j] = 0;
    }
    //memcpy(pvar_value_buffer[i], read_value_buffer, size*tau_pvar_count[i]);
    memcpy(pvar_value_buffer[i], read_value_buffer, sizeof(unsigned long long int)*tau_pvar_count[i]);

    for(j = 0; j < tau_pvar_count[i]; j++){
      /* unsigned long long int to double conversion can result in an error. 
       * We first convert it to a long long int. */
      long long int mydata = (long long int) pvar_value_buffer[i][j]; 
      int is_double = 0; 
      if (tau_mpi_datatype[i] == MPI_DOUBLE) is_double=1; 
      if (is_double) {
        // long long int mydata_d = (long long int) (pvar_value_buffer[i][j]); 
        // First convert it from unsigned long long to long long and then to a double
        //double double_data = ((double)(pvar_value_buffer[i][j]));
        double double_data = ((double)(mydata));

        dprintf("RANK:%d: pvar_value_buffer[%d][%d]=%lld, double_data=%g, size = %d, is_double=%d\n",rank,i,j,mydata, double_data, size, is_double);
	if (double_data > 1e-14 )  {
      
        // Double values are really large for timers. Please check 1E18?? 
          //Tau_track_pvar_event(i, num_pvars, double_data);
          dprintf("Not tracking anything for now \n");
        } 
      } else {
        dprintf("RANK:%d: pvar_value_buffer[%d][%d]=%lld, size = %d, is_double=%d\n",rank,i,j,mydata, size, is_double);
        /* Trigger the TAU event if it is non-zero */
	if (mydata > 0L) {
          Tau_track_pvar_event(i, j, tau_pvar_count, tau_initial_pvar_count, mydata);
        
        }
      }
    }

  }

  /*Implement user based CVAR tuning policy if TAU_MPI_T_ENABLE_USER_TUNING_POLICY is set*/
  if(mpi_t_enable_user_tuning_policy) {
    dprintf("RANK:%d: User based tuning policy enabled \n", rank);

#if TAU_PLUGIN_ENABLED 
    char *plugin_name;
    const char *plugin_path;
    int argc = 3;
    void **argv = Tau_MemMgr_malloc(Tau_get_thread(), argc*sizeof(void*));
    int* num_pvars_heap = Tau_MemMgr_malloc(Tau_get_thread(), argc*sizeof(int));
    (*num_pvars_heap) = tau_initial_pvar_count;
    argv[0] = (void *)num_pvars_heap;
    argv[1] = (void *)tau_pvar_count;
    argv[2] = (void **)pvar_value_buffer;
    Tau_util_apply_role_hook(plugin_manager, "MPIT", argc, argv);

#else
    Tau_enable_user_cvar_tuning_policy(num_pvars, tau_pvar_count, pvar_value_buffer);
#endif /* TAU_PLUGIN_ENABLED */
  }
  
  dprintf("Finished!!\n");
}

//////////////////////////////////////////////////////////////////////
// EOF : TauMpiT.c
//////////////////////////////////////////////////////////////////////


