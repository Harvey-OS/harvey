##################################
# Makefile for MetaPost in C	 #
##################################

SITEDIR	=..
CC=pcc
OPT=-g
CFLAGS=$(OPT) -I$(SITEDIR)
LDFLAGS=$(OPT)
SHELL=/bin/sh
# TANGLE=../web/tangle
TANGLE=/usr/lib/tex/bin/tangle
# TFTOPL=../texware/tftopl
TFTOPL=/usr/lib/tex/bin/tftopl

#########################################################
# Everything below here should seldom need changing	#
#########################################################

OBJS= mp0.o mp1.o mp2.o mp3.o mp4.o mp5.o mp6.o mp7.o mp8.o mp9.o
IOBJS=mpext.o inimp.o $(OBJS)
VOBJS=mpext.o imp.o $(OBJS)

makeargs=SITEDIR="$(SITEDIR)" CC="$(CC)" OPT="$(OPT)" \
         LDFLAGS="$(LDFLAGS)" SHELL="$(SHELL)"



.SUFFIXES:
.SUFFIXES: .o .c
.c.o:
		$(CC) $(CFLAGS) -c $*.c


default:	all

all:		inimp virmp plain.mem mfplain.mem
triptrap:	trapmp


inimp:		stamp-convert $(IOBJS)
		$(CC) $(LDFLAGS) -o inimp $(IOBJS)

virmp:		stamp-convert $(VOBJS)
		$(CC) $(LDFLAGS) -o virmp $(VOBJS)

run-triptrap:
		-ln trapdir/trap.mp .
		-ln trapdir/trap.mpx .
		-ln trapdir/trapf.tfm .
		touch trap.mpx
		-trapmp <trapdir/trap1.in >/dev/null
		-diff trapdir/trapin.log trap.log
		-trapmp <trapdir/trap2.in >trap.fot
		-diff trapdir/trap.log trap.log
		-diff trapdir/trap.fot trap.fot
		-diff trapdir/trap.5 trap.5
		-diff trapdir/trap.6 trap.6
		-diff trapdir/trap.148 trap.148
		-diff trapdir/trap.149 trap.149
		-diff trapdir/trap.150 trap.150
		-diff trapdir/trap.151 trap.151
		-diff trapdir/trap.197 trap.197
		-diff trapdir/trap.200 trap.200
		$(TFTOPL) trap.tfm trap.pl
		-diff trapdir/trap.pl trap.pl


# The `ini_to_trap' script changes mpd.h, instead of mp.ch, so
# retangling is not necessary.  After changing constants, making a
# trapmp means making an inimp.
# 
trapmp:		stamp-trap
		make $(makeargs) inimp
		mv inimp trapmp

stamp-trap:	stamp-convert
		rm -f *.o
		$(SHELL) ./ini_to_trap mpd.h
		touch stamp-trap

# tangle produces mf.p and mf.pool.
# 
mp.p:		mp.web mp.ch
		$(TANGLE) mp.web mp.ch

# The convert script produces mp[0-8].c, imp.c, coerce.h, and mpd.h.
# 
stamp-convert:	mp.p fixcoerce.h mp.defines fix
		$(SHELL) ./convert
		touch stamp-convert

# This filter is used in ./convert; it might not be necessary anymore
# but the web2c version of METAFONT used to use it.
#
fix:		fix.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o fix fix.c -ll

fix.c:		fix.lex
		lex fix.lex
		mv -f lex.yy.c fix.c

# The (generated) file imp.c has #ifdefs on INIMP, so we have to compile
# it with -DINIMP for inimp.
# 
inimp.o:	imp.c
		$(CC) $(CFLAGS) -DINIMP -c imp.c && mv imp.o inimp.o

$(OBJS):	stamp-convert mp.h   # included by mpd.h.
mpext.o:	mpd.h
mp0.o:		mp0.c
mp1.o:		mp1.c
mp2.o:		mp2.c
mp3.o:		mp3.c
mp4.o:		mp4.c
mp5.o:		mp5.c
mp6.o:		mp6.c
mp7.o:		mp7.c
mp8.o:		mp8.c
mp9.o:		mp9.c


# inimp requires mp.pool to be in one of the directories mentioned in the
# MPPOOL path from site.h.  This version should work if the path includes "."
#
plain.mem:	inimp
		./inimp ../mplib/plain dump

mfplain.mem:	inimp
		./inimp ../mplib/mfplain dump


clean:
		rm -f *mp*.o *mpext.o fix.o core
		rm -f trap.* trapf.tfm

veryclean:	clean
		rm -f mp?.c imp.c tmp.c *mpd.h coerce.h mp.pool mp.p \
			fix fix.c inimp virmp trapmp *.mem *.log stamp-*
		rm -f \#*\# *~ *.bak *.ckp core

