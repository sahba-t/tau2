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

CFLAGS          = $(TAU_INCLUDE) $(TAU_DEFS) -DSTD_COMPLEX

LIBS            = $(TAU_LIBS) 

LDFLAGS	      	= $(CFLAGS)

MAKEFILE      	= Makefile

PRINT	      	= pr

RM 	      	= /bin/rm -f

TARGET	      	= mandel

EXTRAOBJS     	= 

##############################################

all:		$(TARGET)	

install: 	$(TARGET)

$(TARGET):	$(TARGET).o mytimer.o ppmwrite.o
	$(CXX) $(LDFLAGS) $(TARGET).o mytimer.o ppmwrite.o -o $@ $(LIBS)

$(TARGET).o : $(TARGET).cpp
	$(CXX) $(CFLAGS) -c $(TARGET).cpp

mytimer.o : mytimer.c
	$(CC) -c $<

ppmwrite.o : ppmwrite.cpp
	$(CXX) $(CFLAGS) -c $<

clean: 	
	$(RM) $(TARGET).o $(TARGET) ppmwrite.o mytimer.o
##############################################
