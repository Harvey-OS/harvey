# This file makes TANGLE and WEAVE from their WEB sources.
#

SITEDIR=..
CC=/usr/ape/apebin/pcc
OPT=-g
CFLAGS=$(OPT) -Dlink=link_array -I$(SITEDIR)
LDFLAGS=
SHELL=/bin/sh

.SUFFIXES:
.SUFFIXES: .o .c .p .ch
.p.c:
		$(SHELL) ./convert $*.p $*.c
.ch.p:
		tangle $*.web $*.ch
.c.o:
		$(CC) $(CFLAGS) -c $*.c


default:	all

all:		tangle weave

# Tangle:
# 
tangle:		tangle.o tanglext.o
		$(CC) $(LDFLAGS) -o tangle tangle.o tanglext.o
tangle.o: 	tangle.c web.h
tangle.c: 	tangle.p web.defines
tangle.p:	tangle.web tangle.ch
		sh -c "until tangle tangle.web tangle.ch;		\
                  do							\
		    $(CC) $(CFLAGS) -o tangle tangleboot.c tanglext.c;	\
		  done"
tanglext.o: 	tanglext.c ${SITEDIR}/site.h


# Weave:
# 
weave: 		weave.o weavext.o
		$(CC) $(LDFLAGS) -o weave weave.o weavext.o
weave.o: 	weave.c web.h
weave.c:	weave.p web.defines
weave.p: 	weave.web weave.ch
weavext.o:	weavext.c ${SITEDIR}/site.h


clean:
		rm -f *.o *.pool weave tangle coerce.h

veryclean:	clean
		rm -f weave.c tangle.c weave.h tangle.p weave.p
		rm -f \#*\# *~ *.bak *.ckp core
