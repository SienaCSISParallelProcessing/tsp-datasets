# Makefile for C program to read and process a TMG file into a TSP input

PROGRAM=tmg2tsp
UTILCFILES=sll.c
ALGCFILES=
CFILES=$(UTILCFILES) $(ALGCFILES) tmggraph.c  $(PROGRAM).c
OFILES=$(CFILES:.c=.o)
CC=gcc
CFLAGS=-Wall -g

$(PROGRAM):	$(OFILES)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(OFILES) -lm

clean::
	/bin/rm $(PROGRAM) $(OFILES)
