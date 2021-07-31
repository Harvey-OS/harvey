# This file makes DVIcopy.

SHELL=/bin/sh

CC=gcc -posix
CCLD=$(CC)
cflags=-g -I../lib
CFLAGS=$(cflags)

# Routines used everywhere.
commondefines=../lib/common.defines
commonh=../lib/common.h
commono=../lib/lib.a


.SUFFIXES:
.SUFFIXES: .o .c .p .ch
.p.c:
	$(SHELL) ../lib/convert $*.p $*.c
.ch.p:
	../web/tangle $*.web $*.ch
.c.o:
	$(CC) $(CFLAGS) -c $*.c


default: all
all: dvicopy


dvicopy: dvicopy.o $(commono)
	$(CCLD) -o dvicopy $(LDFLAGS) dvicopy.o $(commono) $(LIBS)
dvicopy.o: dvicopy.c $(commonh)
dvicopy.c: dvicopy.p $(commondefines)
dvicopy.p: dvicopy.web dvicopy.ch


install: all
	$(INSTALL_PROGRAM) dvicopy $(bindir)/dvicopy

mostlyclean:
	rm -f *.pool *.o dvicopy

clean: mostlyclean
	rm -f dvicopy.p dvicopy.c dvicopy.h

distclean: clean
	rm -f \#*\# *~ *.bak *.ckp core

realclean: distclean
