# This file makes some font utilities that are not part of MFware
# from their respective sources.
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

all:		pktogf pktype vftovp vptovf


# This is used by everything.
# 
extra.o:	extra.c extra.h
extra.h:	${SITEDIR}/site.h

# PKtoGF:
# 
pktogf:		pktogf.o extra.o
		$(CC) $(LDFLAGS) -o pktogf pktogf.o extra.o
pktogf.o:	pktogf.c extra.h
pktogf.c:	pktogf.p futil.defines
pktogf.p:	pktogf.web pktogf.ch

# PKtype:
# 
pktype:		pktype.o extra.o
		$(CC) $(LDFLAGS) -o pktype pktype.o extra.o
pktype.o:	pktype.c extra.h
pktype.c:	pktype.p futil.defines
pktype.p:	pktype.web pktype.ch

# VFtoVP:
# 
vftovp:	vftovp.o extra.o
		$(CC) $(LDFLAGS) -o vftovp vftovp.o extra.o
vftovp.o:	vftovp.c extra.h
vftovp.c:	vftovp.p futil.defines
vftovp.p:	vftovp.web vftovp.ch

# VPtoVF:
# 
vptovf:		vptovf.o extra.o
		$(CC) $(LDFLAGS) -o vptovf vptovf.o extra.o
vptovf.o:	vptovf.c extra.h
vptovf.c:	vptovf.p futil.defines
vptovf.p:	vptovf.web vptovf.ch

clean:
		rm -f *.pool *.o pktogf pktype vftovp vptovf

veryclean:	clean
		rm -f *.p
		rm -f pktogf.c pktype.c vftovp.c vptovf.c
		rm -f pktogf.h pktype.h vftovp.h vptovf.h
		rm -f \#*\# *~ *.bak *.ckp core
