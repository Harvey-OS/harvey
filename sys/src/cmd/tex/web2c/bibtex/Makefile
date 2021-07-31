# This file makes Bibtex.
#

SITEDIR=..
CC=/usr/ape/apebin/pcc
OPT=-g
CFLAGS=$(OPT) -I$(SITEDIR)
LDFLAGS=$(OPT)
SHELL=/bin/sh

.SUFFIXES:
.SUFFIXES: .o .c .p .ch
.p.c:
		$(SHELL) ./convert $*.p $*.c
.ch.p:
		../web/tangle $*.web $*.ch
.c.o:
		$(CC) $(CFLAGS) -c $*.c


default:	all

all:		bibtex


bibtex:		bibtex.o bibext.o
		$(CC) $(LDFLAGS) -o bibtex bibtex.o bibext.o
bibext.o:	bibext.c $(SITEDIR)/site.h
bibtex.o:	bibtex.c bib.h
bibtex.c:	bibtex.p bibtex.defines
bibtex.p:	bibtex.web bibtex.ch


clean:
		rm -f bibtex.pool *.o bibtex

veryclean:	clean
		rm -f bibtex.p bibtex.c bibtex.h coerce.h
		rm -f \#*\# *~ *.bak *.ckp core
