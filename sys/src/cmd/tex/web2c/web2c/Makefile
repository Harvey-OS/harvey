# Makefile for TeX to C conversion programs.

SHELL=/bin/sh

CC=gcc
CCLD=$(CC)
cflags=-g -I../lib
CFLAGS=$(cflags)

YACC=bison -y
LEX=flex
LEXLIB=/usr/local/lib/libfl.a

libs=../lib/lib.a $(LIBS)

.SUFFIXES:
.SUFFIXES: .o .c
.c.o:
		$(CC) $(CFLAGS) -c $*.c

all: web2c fixwrites splitup regfix

web2c: web2cy.o web2cl.o web2c.o
	$(CCLD) -o web2c $(LDFLAGS) web2c.o web2cy.o web2cl.o $(LEXLIB) $(libs)

web2cl.o: web2cl.c web2c.h web2cy.h
web2cy.o: web2cy.c web2c.h
web2c.o: web2c.c web2cy.h

web2cl.c: web2c.lex
	$(LEX) web2c.lex
	rm -f web2cl.c
	mv lex.yy.c web2cl.c

web2cy.h: web2cy.c

web2cy.c: web2c.yacc
	@echo Expect 3 shift/reduce conflicts.
	$(YACC) -d -v web2c.yacc
	rm -f web2cy.c web2cy.h
	mv y.tab.c web2cy.c
	mv y.tab.h web2cy.h

fixwrites: fixwrites.c
	$(CCLD) -o fixwrites $(LDFLAGS) $(CFLAGS) fixwrites.c $(libs)

splitup: splitup.c
	$(CCLD) -o splitup $(LDFLAGS) $(CFLAGS) splitup.c $(libs)

regfix: regfix.c
	$(CCLD) -o regfix $(LDFLAGS) $(CFLAGS) regfix.c $(libs)

mostlyclean:
	rm -f *.o regfix splitup fixwrites web2c

clean: mostlyclean
	rm -f web2cy.c web2cl.c web2cy.h y.tab.h lex.yy.c y.output coerce.h

distclean: clean
	rm -f \#*\# *~ *.bak *.ckp core yacc.*

realclean: distclean
