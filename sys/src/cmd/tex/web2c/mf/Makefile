# This file makes Metafont in its various incarnations.

SHELL=/bin/sh

CC=gcc
cflags=-g -I../lib
CFLAGS=$(cflags)

CCLD=$(CC)
LIBS=

LN=ln
RANLIB=true

# The bases we know how to make.
bases=plain.base cmmf.base

# Name of the file that defines the local devices.
localmodes=modes.mf

# System libraries for window support.
wlibs=-lXt -lX11 #-lnsl_s -linet 

makeargs=SHELL='$(SHELL)' CC='$(CC)' cflags='$(cflags)' CCLD='$(CCLD)' \
LDFLAGS='$(LDFLAGS)' LIBS='$(LIBS)' RANLIB='$(RANLIB)' LN='$(LN)' \
srcdir=$(srcdir)

.SUFFIXES:
.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) -c $*.c


# Routines used everywhere.
commondefines=../lib/common.defines ../lib/texmf.defines
commonh=../lib/common.h ../lib/texmfmem.h
commono=../lib/lib.a

# Routines used in TeX and Metafont.
extrac=../lib/texmf.c
extrah=../lib/texmf.h
extrao=texmf.o

# Routines used in TeX, Metafont, and BibTeX.
fileioc=../lib/openinout.c
fileioo=openinout.o

windowlib=MFwindow/window.a
libs=$(xlibdir) $(windowlib) $(wlibs)

# We don't add `$(commono)' here, since it doesn't depend on anything in
# this directory.
# 
objs=$(fileioo) mf0.o mf1.o mf2.o mf3.o mf4.o mf5.o mf6.o mf7.o mf8.o mf9.o
iobjs=iextra.o inimf.o $(objs)
vobjs=mf-extra.o imf.o $(objs)


default: all
all: inimf virmf
triptrap: trapmf


# It is just a waste of disk space to have window support in inimf.
# 
inimf: $(iobjs) $(commono)
	$(CCLD) -o inimf $(LDFLAGS) $(iobjs) $(commono) $(LIBS)

virmf: $(vobjs) $(windowlib) $(commono)
	$(CCLD) -o virmf $(LDFLAGS) $(vobjs) $(libs) $(commono) $(LIBS)

run-trap: 
	rm -f trap.mf
	-$(LN) MFtrap/trap.mf .
	-$(SHELL) -c '. ./trapenv; ./trapmf < MFtrap/trap1.in > /dev/null 2>&1'
	-diff MFtrap/trapin.log trap.log
	-$(SHELL) -c '. ./trapenv; ./trapmf < MFtrap/trap2.in > trap.fot'
	-diff MFtrap/trap.log trap.log
	-diff MFtrap/trap.fot trap.fot
	../texware/tftopl ./trap.tfm trap.pl
	-diff MFtrap/trap.pl trap.pl
	$(SHELL) -c '. ./trapenv; \
	  ../mfware/gftype -m -i ./trap.72270gf > trap.typ'
	-diff MFtrap/trap.typ trap.typ


# Make base files automatically.
bases: $(bases)

mf.base: inimf
	echo $${MFINPUTS}
	./inimf 'plain; input $(localmodes); dump'
	mv plain.base mf.base
	mv plain.log mf.log

cmmf.base: mf.base
	./inimf \&./mf cmbase dump
	mv cmbase.base cmmf.base
	mv cmbase.log cmmf.log


# The `ini_to_trap' script changes mfd.h, instead of the change file, so
# retangling is not necessary.  After changing constants, making a
# trapmf means making an inimf.
# 
trapmf: stamp-trap
	$(MAKE) $(makeargs) inimf
	mv inimf trapmf

stamp-trap: mfd.h
	rm -f *.o
	$(SHELL) ./ini_to_trap mfd.h
	touch stamp-trap

# tangle produces mf.p and mf.pool.
# 
mf.p: mf.web cmf.ch
	../web/tangle mf.web cmf.ch

# The convert script produces mf[0-9].c, imf.c, coerce.h, and mfd.h.
mf0.c mf1.c mf2.c mf3.c mf4.c mf5.c mf6.c mf7.c mf8.c mf9.c \
imf.c coerce.h mfd.h: stamp-convert

stamp-convert: mf.p fixcoerce.h $(commondefines)
	$(SHELL) ./convert
	touch stamp-convert

# The (hand-coded) file $(extrac) and the (generated) file imf.c have
# #ifdefs for INIMF, so we have to compile them differently.
# 
iextra.o: $(extrac) mfd.h $(extrah) $(commonh)
	$(CC) $(CFLAGS) -I. -DINIMF -DINI -c $(extrac)
	mv $(extrao) iextra.o

inimf.o: imf.c mfd.h $(extrah) $(commonh)
	rm -f inimf.c
	$(LN) imf.c inimf.c
	$(CC) $(CFLAGS) -DINIMF -c inimf.c

$(objs): mfd.h

mf-extra.o: $(extrac) mfd.h
	rm -f mf-extra.c
	$(LN) $(extrac) mf-extra.c
	$(CC) $(CFLAGS) -I. -c mf-extra.c

$(fileioo): $(fileioc) $(commonh)
	$(CC) $(CFLAGS) -I. -c $(fileioc)

mf0.o: mf0.c $(commonh)
mf1.o: mf1.c $(commonh)
mf2.o: mf2.c $(commonh)
mf3.o: mf3.c $(commonh)
mf4.o: mf4.c $(commonh)
mf5.o: mf5.c $(commonh)
mf6.o: mf6.c $(commonh)
mf7.o: mf7.c $(commonh)
mf8.o: mf8.c $(commonh)
mf9.o: mf9.c $(commonh)

$(windowlib): mfd.h
	cd MFwindow; $(MAKE) $(makeargs) xincludedir=$(xincludedir)


install: all
	$(INSTALL_PROGRAM) virmf $(bindir)/virmf
	$(INSTALL_PROGRAM) inimf $(bindir)/inimf
	-if test -d $(mfpooldir); then exit 0; else mkdir $(mfpooldir); fi
	-if test -s mf.pool; then $(INSTALL_DATA) mf.pool $(mfpooldir); fi


mostlyclean: 
	rm -f stamp-* *.o trapmf inimf virmf
	rm -f mfd.h coerce.h *.log *.base
	rm -f trap.* mf-extra.c
	cd MFwindow; $(MAKE) clean

clean: mostlyclean
	rm -f mf?.c imf.c mf.p mf.pool
	cd MFwindow; $(MAKE) realclean

distclean: clean
	rm -f \#*\# *~ *.bak *.ckp core *gf *.tfm mfput.*
	rm -f cmf.ch

realclean: distclean
