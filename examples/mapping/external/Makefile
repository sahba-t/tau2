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
 
include ../../../include/Makefile 
include ../../Makefile



CXX		= $(TAU_CXX)

CC		= $(TAU_CC)

CFLAGS        	= $(TAU_INCLUDE) $(TAU_DEFS)

LIBS 	      	= $(TAU_LIBS) $(LEXTRA1)

LDFLAGS	      	= $(USER_OPT)

MAKEFILE      	= Makefile

PRINT	      	= pr

RM 	      	= /bin/rm -f

TARGET	      	= external

EXTRAOBJS     	= 

##############################################

all:		$(TARGET)	

install: 	$(TARGET)

$(TARGET):	$(TARGET).o
	$(CXX) $(LDFLAGS) $(TARGET).o -o $@ $(LIBS)

$(TARGET).o : inst.cpp
	$(CXX) $(CFLAGS) -c inst.cpp -o $@

clean: 	
	$(RM) $(TARGET).o $(TARGET)
##############################################
