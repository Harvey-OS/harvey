# This file makes the programs published in the TeXware technical
# report: DVItype, PatGen, PLtoTF, TFtoPL, and POOLtype.
# 
# Except that we compile and install PatGen 2.0, the revision of Liang's
# original program by Peter Breitenlohner for TeX 3.

SHELL=/bin/sh

CC=gcc -posix
CCLD=$(CC)
cflags=-g -I../lib
CFLAGS=$(cflags)

# Routines used everywhere.
commondefines=../lib/common.defines
commonh=../lib/common.h # Could add a lot more.
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
all: triptrap patgen pooltype
triptrap: dvitype pltotf tftopl


dvitype: dvitype.o $(commono)
	$(CCLD) -o dvitype $(LDFLAGS) dvitype.o $(commono) $(LIBS)
dvitype.o: dvitype.c $(commonh)
dvitype.c: dvitype.p $(commondefines)
dvitype.p: dvitype.web dvitype.ch


# (If you remove all the `2's here, you'll get the original patgen.)
patgen: patgen2.o $(commono)
	$(CCLD) -o patgen $(LDFLAGS) patgen2.o $(commono) $(LIBS)
patgen2.o: patgen2.c $(commonh)
patgen2.c: patgen2.p $(commondefines)
patgen2.p: patgen2.web patgen2.ch


pltotf: pltotf.o $(commono)
	$(CCLD) -o pltotf $(LDFLAGS) pltotf.o $(commono) $(LIBS)
pltotf.o: pltotf.c $(commonh)
	$(CC) $(CFLAGS) -c -Dindex=index_arr pltotf.c
pltotf.c: pltotf.p $(commondefines)
pltotf.p: pltotf.web pltotf.ch


pooltype: pooltype.o $(commono)
	$(CCLD) -o pooltype $(LDFLAGS) pooltype.o $(commono) $(LIBS)
pooltype.o: pooltype.c $(commonh)
pooltype.c: pooltype.p $(commondefines)
pooltype.p: pooltype.web pooltype.ch


tftopl: tftopl.o $(commono)
	$(CCLD) -o tftopl $(LDFLAGS) tftopl.o $(commono) $(LIBS)
tftopl.o: tftopl.c $(commonh)
	$(CC) $(CFLAGS) -c -Dindex=index_var tftopl.c
tftopl.c: tftopl.p $(commondefines)
tftopl.p: tftopl.web tftopl.ch


install: all
	$(INSTALL_PROGRAM) dvitype $(bindir)/dvitype
	$(INSTALL_PROGRAM) patgen $(bindir)/patgen
	$(INSTALL_PROGRAM) pltotf $(bindir)/pltotf
	$(INSTALL_PROGRAM) pooltype $(bindir)/pooltype
	$(INSTALL_PROGRAM) tftopl $(bindir)/tftopl

mostlyclean:
	rm -f *.pool *.o dvitype patgen pltotf pooltype tftopl

clean: mostlyclean
	rm -f *.p *.c *.h

distclean: clean
	rm -f \#*\# *~ *.bak *.ckp core

realclean: distclean
