# This file makes DVItype, PLtoTF, TFtoPL, and POOLtype from their
# respective sources.
# 
# Unfortunately, `texware.defines' is 15 characters long, so I omit the `s'.
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

all:		triptrap patgen pooltype
triptrap:	dvitype pltotf tftopl


# This is used by everything.
# 
texwarext.o:  texwarext.c ${SITEDIR}/site.h

# DVItype:
# 
dvitype:	dvitype.o dvitypext.o texwarext.o
		$(CC) $(LDFLAGS) -o dvitype dvitype.o texwarext.o dvitypext.o 
dvitypext.o:	dvitypext.c ${SITEDIR}/site.h
dvitype.o:	dvitype.c texware.h
dvitype.c:	dvitype.p texware.define
dvitype.p:	dvitype.web dvitype.ch

# PLtoTF:
# 
pltotf:		pltotf.o texwarext.o
		$(CC) $(LDFLAGS) -o pltotf pltotf.o texwarext.o
pltotf.o:	pltotf.c texware.h
		$(CC) $(CFLAGS) -c -Dindex=index_array -Dlink=link_array pltotf.c
pltotf.c:	pltotf.p texware.define
pltotf.p:	pltotf.web pltotf.ch

# TFtoPL:
# 
tftopl:		tftopl.o texwarext.o
		$(CC) $(LDFLAGS) -o tftopl tftopl.o texwarext.o
tftopl.o:	tftopl.c texware.h
		$(CC) $(CFLAGS) -c -Dindex=index_type tftopl.c
tftopl.c:	tftopl.p texware.define
tftopl.p:	tftopl.web tftopl.ch

# PatGen:
# 
patgen:	patgen.o texwarext.o
		$(CC) $(LDFLAGS) -o patgen patgen.o texwarext.o
patgen.o:	patgen.c texware.h
patgen.c:	patgen.p texware.define
patgen.p:	patgen.web patgen.ch

# POOLtype:
# 
pooltype:	pooltype.o texwarext.o
		$(CC) $(LDFLAGS) -o pooltype pooltype.o texwarext.o
pooltype.o:	pooltype.c texware.h
pooltype.c:	pooltype.p texware.define
pooltype.p:	pooltype.web pooltype.ch


clean:
		rm -f *.pool *.o dvitype patgen pltotf pooltype tftopl

veryclean:	clean
		rm -f *.p
		rm -f dvitype.c patgen.c pltotf.c pooltype.c tftopl.c
		rm -f dvitype.h patgen.h pltotf.h pooltype.h tftopl.h
		rm -f \#*\# *~ *.bak *.ckp core
