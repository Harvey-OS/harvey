# This file makes the programs published in the Metafontware report: 
# GFtoDVI, GFtoPK, GFtype, and MFT.

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
all: triptrap gftodvi gftopk mft
triptrap: gftype


gftodvi: gftodvi.o $(commono)
	$(CCLD) -o gftodvi $(LDFLAGS) gftodvi.o $(commono) $(LIBS)
gftodvi.o: gftodvi.c $(commonh) gftodmem.h
gftodvi.c: gftodvi.p $(commondefines)
gftodvi.p: gftodvi.web gftodvi.ch


gftopk: gftopk.o $(commono)
	$(CCLD) -o gftopk $(LDFLAGS) gftopk.o $(commono) $(LIBS)
gftopk.o: gftopk.c $(commonh)
gftopk.c: gftopk.p $(commondefines)
gftopk.p: gftopk.web gftopk.ch


gftype: gftype.o $(commono)
	$(CCLD) -o gftype $(LDFLAGS) gftype.o $(commono) $(LIBS) 
gftype.o: gftype.c $(commonh)
gftype.c: gftype.p $(commondefines)
gftype.p: gftype.web gftype.ch


mft: mft.o $(commono)
	$(CCLD) -o mft $(LDFLAGS) mft.o $(commono) $(LIBS)
mft.o: mft.c $(commonh)
mft.c: mft.p $(commondefines)
mft.p: mft.web mft.ch


install: all
	$(INSTALL_PROGRAM) gftodvi $(bindir)/gftodvi
	$(INSTALL_PROGRAM) gftype $(bindir)/gftype
	$(INSTALL_PROGRAM) gftopk $(bindir)/gftopk
	$(INSTALL_PROGRAM) mft $(bindir)/mft

mostlyclean: 
	rm -f *.pool *.o gftodvi gftype gftopk mft

# Can't remove *.h, since that clobbers gftodmem.h.
clean: mostlyclean
	rm -f *.p *.c
	rm -f gftodvi.h gftype.h gftopk.h mft.h

distclean: clean
	rm -f \#*\# *~ *.bak *.ckp core

realclean: distclean
