# This file makes some font utilities that are not part of the
# Metafontware report but are nevertheless standard.

SHELL=/bin/sh

CC=gcc -posix
CCLD=$(CC)
cflags=-g -I../config -I../lib
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
all: pktogf pktype vftovp vptovf


pktogf: pktogf.o $(commono)
	$(CCLD) -o pktogf $(LDFLAGS) pktogf.o $(commono) $(LIBS)
pktogf.o: pktogf.c $(commonh)
pktogf.c: pktogf.p $(commondefines)
pktogf.p: pktogf.web pktogf.ch


pktype: pktype.o $(commono)
	$(CCLD) -o pktype $(LDFLAGS) pktype.o $(commono) $(LIBS)
pktype.o: pktype.c $(commonh)
pktype.c: pktype.p $(commondefines)
pktype.p: pktype.web pktype.ch


vftovp: vftovp.o $(commono)
	$(CCLD) -o vftovp $(LDFLAGS) vftovp.o $(commono) $(LIBS)
vftovp.o: vftovp.c $(commonh)
	$(CC) $(CFLAGS) -c -Dindex=index_var vftovp.c
vftovp.c: vftovp.p $(commondefines)
vftovp.p: vftovp.web vftovp.ch


vptovf: vptovf.o $(commono)
	$(CCLD) -o vptovf $(LDFLAGS) vptovf.o $(commono) $(LIBS)
vptovf.o: vptovf.c $(commonh)
	$(CC) $(CFLAGS) -c -Dindex=index_arr vptovf.c
vptovf.c: vptovf.p $(commondefines)
vptovf.p: vptovf.web vptovf.ch


install: all
	$(INSTALL_PROGRAM) pktogf $(bindir)/pktogf
	$(INSTALL_PROGRAM) pktype $(bindir)/pktype
	$(INSTALL_PROGRAM) vftovp $(bindir)/vftovp
	$(INSTALL_PROGRAM) vptovf $(bindir)/vptovf

mostlyclean:
	rm -f *.pool *.o pktogf pktype vftovp vptovf

clean: mostlyclean
	rm -f *.p *.c *.h

distclean: clean
	rm -f \#*\# *~ *.bak *.ckp core

realclean: distclean
