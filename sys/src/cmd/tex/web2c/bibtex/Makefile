# This file makes BibTeX.

SHELL=/bin/sh

CC=gcc -posix
CCLD=$(CC)
cflags=-g -I../lib
CFLAGS=-DBibTeX $(cflags)

# Routines used everywhere.
commondefines=../lib/common.defines ../lib/texmf.defines
commonh=../lib/common.h
commono=../lib/lib.a

# Routines used in TeX, Metafont, and BibTeX.
fileioc=../lib/openinout.c
fileioo=openinout.o

libs=$(fileioo) $(commono) $(LIBS)


.SUFFIXES:
.SUFFIXES: .o .c .p .ch
.p.c:
	$(SHELL) ./convert $*.p $*.c
.ch.p:
	../web/tangle $*.web c$*.ch
.c.o:
	$(CC) $(CFLAGS) -c $*.c


default: all
all: bibtex


bibtex: bibtex.o $(commono) $(fileioo)
	$(CCLD) -o bibtex $(LDFLAGS) bibtex.o $(libs)
bibtex.o: bibtex.c $(commonh)
bibtex.c: bibtex.p $(commondefines) convert
bibtex.p: bibtex.web bibtex.ch

$(fileioo): $(fileioc)
	$(CC) $(CFLAGS) -c $(fileioc)


install: all
	$(INSTALL_PROGRAM) bibtex $(bindir)/bibtex

mostlyclean:
	rm -f *.pool *.o bibtex

clean: mostlyclean
	rm -f bibtex.p bibtex.c bibtex.h coerce.h

distclean: clean
	rm -f \#*\# *~ *.bak *.ckp core
	rm -f cbibtex.ch

realclean: distclean
