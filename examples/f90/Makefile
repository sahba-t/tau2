#****************************************************************************
#*			TAU Portable Profiling Package			   **
#*			http://www.cs.uoregon.edu/research/tau	           **
#****************************************************************************
#*    Copyright 1997  						   	   **
#*    Department of Computer and Information Science, University of Oregon **
#*    Advanced Computing Laboratory, Los Alamos National Laboratory        **
#****************************************************************************
#######################################################################
##                  pC++/Sage++  Copyright (C) 1993,1995             ##
##  Indiana University  University of Oregon  University of Rennes   ##
#######################################################################
 
include ../../include/Makefile 
include ../Makefile



CXX		= $(TAU_CXX)

CC		= $(TAU_CC)

FFLAGS        	= $(TAU_INCLUDE) $(TAU_F90_FIXED)

LIBS            = $(TAU_LIBS) $(TAU_FORTRANLIBS)

LDFLAGS	      	= $(USER_OPT) $(TAU_DEFS)

MAKEFILE      	= Makefile

PRINT	      	= pr

RM 	      	= /bin/rm -f

F90		= $(TAU_F90)  

TARGET	      	= cubes

EXTRAOBJS     	= 

##############################################

all:		$(TARGET)	

install: 	$(TARGET)

$(TARGET):	$(TARGET).o  
	$(CXX) $(LDFLAGS) $(TARGET).o  -o $@ $(LIBS)

$(TARGET).o : $(TARGET).f
	$(F90) $(FFLAGS) -c $(TARGET).f

clean: 	
	$(RM) $(TARGET).o $(TARGET)
##############################################
