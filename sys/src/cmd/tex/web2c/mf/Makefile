# This file makes Metafont in its various incarnations.
#

SITEDIR=..
CC=/usr/ape/apebin/pcc
OPT=-g
CFLAGS=$(OPT) -I$(SITEDIR)
LDFLAGS=$(OPT)
SHELL=/bin/sh

wlibs=-lXt -lX11

windowlib=MFwindow/window.a
libs=$(windowlib) $(wlibs) 
objs=mf0.o mf1.o mf2.o mf3.o mf4.o mf5.o mf6.o mf7.o mf8.o mf9.o ini.o
iobjs=iextra.o inimf.o $(objs)
vobjs=extra.o imf.o $(objs)

makeargs=SITEDIR="$(SITEDIR)" CC="$(CC)" OPT="$(OPT)" \
         LDFLAGS="$(LDFLAGS)" wlibs="$(wlibs)"



.SUFFIXES:
.SUFFIXES: .o .c
.c.o:
		$(CC) $(CFLAGS) -c $*.c


default:	all

all:		inimf virmf
triptrap:	trapmf


inimf:		stamp-convert $(iobjs) $(windowlib)
		$(CC) $(LDFLAGS) -o inimf $(iobjs) $(libs)

virmf:		stamp-convert $(vobjs) $(windowlib)
		$(CC) $(LDFLAGS) -o virmf $(vobjs) $(libs)

run-triptrap:
		-ln MFtrap/trap.mf .
		-trapmf < MFtrap/trap1.in
		-diff MFtrap/trapin.log trap.log
		-trapmf < MFtrap/trap2.in > trap.fot
		-diff MFtrap/trap.log trap.log
		-diff MFtrap/trap.fot trap.fot
		../texware/tftopl trap.tfm trap.pl
		-diff MFtrap/trap.pl trap.pl
		../mfware/gftype -m -i trap.72270gf > trap.typ
		-diff MFtrap/trap.typ trap.typ

# The `ini_to_trap' script changes mfd.h, instead of cmf.ch, so
# retangling is not necessary.  After changing constants, making a
# trapmf means making an inimf.  We don't actually have to make the
# window library when making a trapmf, but it shouldn't hurt.
# 
trapmf:		stamp-trap
		make $(makeargs) inimf
		mv inimf trapmf

stamp-trap:	stamp-convert
		rm -f *.o
		$(SHELL) ./ini_to_trap mfd.h
		touch stamp-trap

# tangle produces mf.p and mf.pool.
# 
mf.p:		mf.web cmf.ch
		../web/tangle mf.web cmf.ch

# The convert script produces mf[0-9].c, imf.c, coerce.h, and mfd.h.
# The (hand-coded) memory.h is included in the mf?.c files; it isn't
# really necessary to `convert' again if it changes, only to recompile
# that file, but describing that relationship is ugly to do.
# (Furthermore, memory.h is stable.)
# 
stamp-convert:	mf.p memory.h fixcoerce.h mf.defines
		$(SHELL) ./convert
		touch stamp-convert

# The (hand-coded) file extra.c and the (generated) file imf.c have
# #ifdefs on INIMF, so we have to compile them with -DINIMF for inimf and
# trapmf.
# 
iextra.o:	extra.c mfd.h
		$(CC) $(CFLAGS) -DINIMF -c extra.c && mv extra.o iextra.o

inimf.o:	imf.c
		$(CC) $(CFLAGS) -DINIMF -c imf.c && mv imf.o inimf.o

$(objs):	stamp-convert mf.h   # included by mfd.h.
extra.o:	mfd.h
mf0.o:		mf0.c
mf1.o:		mf1.c
mf2.o:		mf2.c
mf3.o:		mf3.c
mf4.o:		mf4.c
mf5.o:		mf5.c
mf6.o:		mf6.c
mf7.o:		mf7.c
mf8.o:		mf8.c
mf9.o:		mf9.c

$(windowlib):
		cd MFwindow; make $(makeargs)
		
clean:
		rm -f stamp-* *.o trapmf inimf virmf
		rm -f mfd.h coerce.h *.log
		rm -f trap.*
		cd MFwindow; make clean

veryclean:	clean
		rm -f mf?.c imf.c mf.p mf.pool *.base mfput.*
		rm -f \#*\# *~ *.bak *.ckp core
		cd MFwindow; make veryclean
