# This file makes GFtoDVI, GFtoPK, GFtype, mft, PKtoGF, and PKtype from
# their respective sources.
#

SITEDIR=..
CC=/usr/ape/apebin/pcc
OPT=-g
CFLAGS=$(OPT) -Dlink=link_array -I$(SITEDIR)
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

all:		triptrap gftodvi gftopk mft
triptrap:	gftype


# This is used by everything.
# 
mfwarext.o:	mfwarext.c mfware.h
mfware.h:	${SITEDIR}/site.h

# GFtoDVI:
# 
gftodvi:	gftodvi.o mfwarext.o
		$(CC) $(LDFLAGS) -o gftodvi gftodvi.o mfwarext.o
gftodvi.o:	gftodvi.c mfware.h memory.h
gftodvi.c:	gftodvi.p mfware.defines
gftodvi.p:	gftodvi.web gftodvi.ch

# GFtoPK:
# 
gftopk:		gftopk.o mfwarext.o
		$(CC) $(LDFLAGS) -o gftopk gftopk.o mfwarext.o
gftopk.o:	gftopk.c mfware.h
gftopk.c:	gftopk.p mfware.defines
gftopk.p:	gftopk.web gftopk.ch

# GFtype:
# 
gftype:		gftype.o mfwarext.o
		$(CC) $(LDFLAGS) -o gftype gftype.o mfwarext.o
gftype.o:	gftype.c mfware.h
gftype.c:	gftype.p mfware.defines
gftype.p:	gftype.web gftype.ch

# MFT:
# 
mft:		mft.o mfwarext.o mftext.o
		$(CC) $(LDFLAGS) $(CFLAGS) -o mft mft.o mfwarext.o mftext.o
mft.o:		mft.c mfware.h
mft.c:		mft.p mfware.defines
mft.p:		mft.web mft.ch

clean:
		rm -f *.pool *.o gftodvi gftype gftopk mft

veryclean:	clean
		rm -f *.p
		rm -f gftodvi.c gftype.c gftopk.c mft.c
		rm -f gftodvi.h gftype.h gftopk.h mft.h
		rm -f \#*\# *~ *.bak *.ckp core
