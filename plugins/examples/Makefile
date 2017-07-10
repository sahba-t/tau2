###############################################################################
## Top level Makefile for Plugins #
###############################################################################

include ../../include/Makefile
include ../../utils/include/Makefile

PLUGININSTALLDIR = $(TAU_PREFIX_INSTALL_DIR)/$(CONFIG_ARCH)/lib/shared$(TAU_CONFIG)

MAKE = make

CC = $(TAU_CC)

CXX = $(TAU_CXX)

CFLAGS = $(TAU_MPI_COMPILE_INCLUDE) $(TAU_INCLUDE) $(TAU_DEFS) $(USER_OPT) $(TAU_INTERNAL_FLAGS) -fPIC

LDFLAGS = $(TAU_MPI_LIB)

OBJS = libTAU-filter-plugin.so libTAU-mpit-recommend-sharp-usage-plugin.so libTAU-mpit-mvapich-free_unused_vbufs.so tau_plugin_example_disable_instrumentation_runtime.o tau_plugin_example_mpit_recommend_sharp_usage.o tau_plugin_example_free_unused_vbufs.o

SHAREDLIBS = libTAU-filter-plugin.so libTAU-mpit-recommend-sharp-usage-plugin.so libTAU-mpit-mvapich-free_unused_vbufs.so

all: install
	@echo

install: $(SHAREDLIBS)
	@mkdir -p $(PLUGININSTALLDIR)
	$(TAU_INSTALL) $^ $(PLUGININSTALLDIR)

libTAU-filter-plugin.so: tau_plugin_example_disable_instrumentation_runtime.o 
	$(CXX) $(LDFLAGS) -shared -o libTAU-filter-plugin.so tau_plugin_example_disable_instrumentation_runtime.o 

tau_plugin_example_disable_instrumentation_runtime.o: Tau_plugin_example_disable_instrumentation_runtime.cpp
	$(CXX) $(CFLAGS) -c Tau_plugin_example_disable_instrumentation_runtime.cpp -o tau_plugin_example_disable_instrumentation_runtime.o

libTAU-mpit-recommend-sharp-usage-plugin.so : tau_plugin_example_mpit_recommend_sharp_usage.o
	 $(CXX) $(LDFLAGS) -shared -o libTAU-mpit-recommend-sharp-usage-plugin.so  tau_plugin_example_mpit_recommend_sharp_usage.o

tau_plugin_example_mpit_recommend_sharp_usage.o: Tau_plugin_example_mpit_recommend_sharp_usage.cpp
	$(CXX) $(CFLAGS) -c Tau_plugin_example_mpit_recommend_sharp_usage.cpp -o tau_plugin_example_mpit_recommend_sharp_usage.o

libTAU-mpit-mvapich-free_unused_vbufs.so: tau_plugin_example_free_unused_vbufs.o
	$(CC) $(LDFLAGS) -shared -o libTAU-mpit-mvapich-free_unused_vbufs.so tau_plugin_example_free_unused_vbufs.o

tau_plugin_example_free_unused_vbufs.o: Tau_plugin_example_mpit_mvapich_free_unused_vbufs.c
	$(CC) $(CFLAGS) -c Tau_plugin_example_mpit_mvapich_free_unused_vbufs.c -o tau_plugin_example_free_unused_vbufs.o
clean:
	/bin/rm -rf *.o $(SHAREDLIBS)