# This file makes MetaPost support programs from the WEB and C sources.
#

SITEDIR	=..
CC=pcc
OPT=-g
CFLAGS=$(OPT) -I$(SITEDIR)
LDFLAGS=$(OPT)
SHELL=/bin/sh
# TANGLE=../web/tangle
TANGLE=/usr/lib/tex/bin/tangle

.SUFFIXES:
.SUFFIXES: .o .c .p .ch
.p.c:
		$(SHELL) ./convert $*.p $*.c
.ch.p:
		$(TANGLE) $*.web $*.ch
.c.o:
		$(CC) $(CFLAGS) -c $*.c


default:	all

all:		tex troff
tex:		dvitomp mptotex
troff:		dmp mptotr


# Don't test mptotr because its source file is identical to mptotex.c
# except for a few string constants.
# This will abort if either diff returns nonzero exit status
# otherwise the test is considered passed.
run-triptrap:	mptotex
		mptotex testex.mp | diff testex.tex -
		sed '15s/^%//' <testex.mp >testex1
		-mptotex testex1 >/dev/null 2>testex2
		sed '16s/^%//' <testex.mp >testex1
		-mptotex testex1 >/dev/null 2>>testex2
		sed '17s/^%//' <testex.mp >testex1
		-mptotex testex1 >/dev/null 2>>testex2
		sed '18s/^%//' <testex.mp >testex1
		-mptotex testex1 >/dev/null 2>>testex2
		sed '19s/^%//' <testex.mp >testex1
		-mptotex testex1 >/dev/null 2>>testex2
		diff testex.err testex2

# DVItoMP
dvitomp:	dvitomp.o dvitompext.o
		$(CC) $(CFLAGS) -o dvitomp dvitomp.o dvitompext.o
dvitompext.o:	dvitompext.c ${SITEDIR}/site.h
dvitomp.o:	dvitomp.c mpware.h
dvitomp.c:	dvitomp.p mpware.defines
dvitomp.p:	dvitomp.web dvitomp.ch


# MPtoTEX
mptotex:	mptotex.o
		$(CC) $(CFLAGS) -o mptotex mptotex.o

# MPtoTR
mptotr:		mptotr.o
		$(CC) $(CFLAGS) -o mptotr mptotr.o

# DMP
dmp:		dmp.o
		$(CC) $(CFLAGS) -o dmp dmp.o


clean:
	rm -f dvitomp.p dvitomp.pool *.o testex1 testex2
veryclean:	clean
	rm -f dvitomp.c dvitomp.h dvitomp mptotex mptotr dmp
	rm -f \#*\# *~ *.bak *.ckp core
