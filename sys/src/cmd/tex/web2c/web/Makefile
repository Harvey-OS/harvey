# This file makes Tangle and Weave from their WEB sources, or, in case
# of tangle, possibly from a bootstrap C file.

SHELL=/bin/sh

CC=gcc -posix
cflags=-g -I../lib
CFLAGS=$(cflags)
CCLD=$(CC)

# Routines used everywhere.
commondefines=../lib/common.defines
commonh=../lib/common.h
commono=../lib/lib.a

libs=$(commono) $(LIBS)

.SUFFIXES:
.SUFFIXES: .o .c .p .ch
.p.c:
	$(SHELL) ../lib/convert $*.p $*.c
.ch.p:
	./tangle $*.web $*.ch
.c.o:
	$(CC) $(CFLAGS) -c $*.c


default: triptrap
triptrap: tangle
all: tangle weave


tangle: tangle.o $(commono)
	$(CCLD) -o tangle $(LDFLAGS) tangle.o $(commono) $(LIBS)
tangle.o: tangle.c $(commonh)
tangle.c: tangle.p $(commondefines)

# Making tangle.p is a special case, since it is needed to compile
# itself.  We compile the (distributed) tangleboot.c to make a tangle
# which we can use to make the other programs.  (We don't bother to make
# tangle again using itself.)
tangle.p: tangleboot tangle.web tangle.ch
	./tangleboot tangle.web tangle.ch

tangleboot: tangleboot.c tangleboot.h
	$(CCLD) -o tangleboot $(CFLAGS) $(LDFLAGS) tangleboot.c $(libs)

# To make tangleboot.c, we have to assume the presence of a working
# tangle somewhere in PATH.
tangleboot.c tangleboot.h: tangleboot.p
	$(SHELL) ../lib/convert tangleboot.p temp.c
	sed s/tangle.h/tangleboot.h/ < temp.c > tangleboot.c
	rm -f temp.c

tangleboot.p: tangle.web tangle.ch
	tangle tangle.web tangle.ch
	mv tangle.p tangleboot.p

weave: weave.o $(commono)
	$(CCLD) -o weave $(LDFLAGS) weave.o $(libs)
weave.o: weave.c $(commonh)
weave.c: weave.p $(commondefines)
weave.p: weave.web weave.ch


install: all
	$(INSTALL_PROGRAM) tangle $(bindir)/tangle
	$(INSTALL_PROGRAM) weave $(bindir)/weave

mostlyclean: 
	rm -f *.o *.pool weave tangle tangleboot coerce.h

clean: mostlyclean
	rm -f tangle.p tangle.c tangle.h
	rm -f weave.p weave.c weave.h

distclean: clean
	rm -f \#*\# *~ *.bak *.ckp core

realclean: distclean
	rm -f tangleboot.c tangleboot.h tangleboot.p
