# This file makes TeX in its various incarnations.

SHELL=/bin/sh

CC=gcc -posix
CCLD=$(CC)
cflags=-g -I../lib
CFLAGS=-DTeX $(cflags)

LN=ln

# The formats we know how to make.
formats=amslatex.fmt amstex.fmt etex.fmt inrstex.fmt latex.fmt picplus.fmt \
slitex.fmt tex.fmt texinfo.fmt 

makeargs=SHELL='$(SHELL)' CC='$(CC)' cflags='$(cflags)' CCLD='$(CCLD)' \
LDFLAGS='$(LDFLAGS)' LIBS='$(LIBS)' LN='$(LN)'

.SUFFIXES: 
.SUFFIXES: .o .c
.c.o: 
	$(CC) $(CFLAGS) -c $*.c

# Routines used everywhere.
commondefines=../lib/common.defines ../lib/texmf.defines
commonh=../lib/common.h ../lib/texmfmem.h
commono=../lib/lib.a

# Routines used in both TeX and Metafont.
extrac=../lib/texmf.c
extrah=../lib/texmf.h
extrao=texmf.o

# Routines used in TeX, Metafont, and BibTeX.
fileioc=../lib/openinout.c
fileioo=openinout.o

# We don't add `$(commono)' here, since it doesn't depend on anything in
# this directory, and we write a dependency for `$(objs)' below.
# 
# If you have applied patches to make Michael Ferguson's multi-lingual
# TeX, or perhaps other patches as well, you will need to add `tex9.o'
# to this list.
objs=$(fileioo) tex0.o tex1.o tex2.o tex3.o tex4.o tex5.o tex6.o tex7.o tex8.o
iobjs=iextra.o initex.o $(objs)
vobjs=tex-extra.o itex.o $(objs)


default: all

all: initex virtex
triptrap: triptex

initex: $(iobjs) $(commono)
	$(CCLD) -o initex $(LDFLAGS) $(iobjs) $(commono) $(LIBS)

virtex: $(vobjs) $(commono)
	$(CCLD) -o virtex $(LDFLAGS) $(vobjs) $(commono) $(LIBS)

run-trip: 
	rm -f trip.tex
	-$(LN) TeXtrip/trip.tex .
	../texware/pltotf TeXtrip/trip.pl trip.tfm
	-$(SHELL) -c '. ./tripenv; \
          ./triptex < TeXtrip/trip1.in > /dev/null 2>&1'
	-diff TeXtrip/tripin.log trip.log
	-$(SHELL) -c '. ./tripenv; ./triptex < TeXtrip/trip2.in > trip.fot'
	-diff TeXtrip/trip.log trip.log
	-diff TeXtrip/trip.fot trip.fot
	$(SHELL) -c '. ./tripenv; \
	  ../texware/dvitype trip.dvi < TeXtrip/dvitype.in > trip.typ'
	-diff TeXtrip/trip.typ trip.typ


# Make format files automatically.
# 
formats: $(formats)

# AMSLaTeX requires that you
# 1) move Lamport's lfonts.tex out of TeX's way (I call it lfonts.latex),
# 2) say `lfonts.new' when lplain.tex can't find lfonts.tex,
# 3) choose a ``fontdef'' file (I use fontdef.ori),
# 4) a ``preload'' file, (I use preload.ori),
# 5) and a ``basefont'' file.  (I use basefont).  How automatic, huh?
amslatex.fmt: 
	./initex lplain \\dump
	mv lplain.fmt amslatex.fmt
	mv lplain.log amslatex.log

# As of AMSTeX 2.1, the initialization file is named `amstex.ini'.
# Because it explicitly reads plain.tex, we cannot use &./tex; that
# produces the `Too late for \patterns' error.
amstex.fmt: 
	./initex amstex.ini \\dump

etex.fmt: tex.fmt
	touch eplain.aux # Makes cross-reference warnings work right.
	./initex \&./tex eplain \\dump
	mv eplain.fmt etex.fmt
	mv eplain.log etex.log

inrstex.fmt: 
	./initex inrstex \\dump

latex.fmt: initex
	./initex lplain \\dump
	mv lplain.fmt latex.fmt
	mv lplain.log latex.log

picplus.fmt: tex.fmt
	./initex \&./tex picplus \\dump

slitex.fmt: initex
	./initex splain \\dump
	mv splain.fmt slitex.fmt
	mv splain.log slitex.log

tex.fmt: initex
	./initex plain \\dump
	mv plain.fmt tex.fmt
	mv plain.log tex.log

# Texinfo changes the escape character from `\' to `@'.
texinfo.fmt: tex.fmt
	./initex \&./tex texinfo @dump


# The `ini_to_trip' script changes texd.h, instead of the change file, so
# retangling is not necessary.  After changing constants, making a
# triptex means making an initex.
# 
triptex: stamp-trip
	$(MAKE) $(makeargs) initex
	mv initex triptex

stamp-trip: stamp-convert
	rm -f *.o
	$(SHELL) ./ini_to_trip texd.h
	touch stamp-trip

# tangle produces tex.p and tex.pool.
# 
tex.p: tex.web ctex.ch
	../web/tangle tex.web ctex.ch

# The convert script produces tex[0-8].c, itex.c, coerce.h, and texd.h.
tex0.c tex1.c tex2.c tex3.c tex4.c tex5.c tex6.c tex7.c tex8.c tex9.c \
itex.c coerce.h texd.h: stamp-convert

stamp-convert: tex.p fixcoerce.h $(commondefines)
	$(SHELL) ./convert
	touch stamp-convert

# The (hand-coded) file $(extrac) and the (generated) file itex.c have
# #ifdefs for INITEX, so we compile them differently.
# 
iextra.o: $(extrac) texd.h $(extrah) $(commonh)
	$(CC) $(CFLAGS) -I. -DINITEX -DINI -c $(extrac)
	mv $(extrao) iextra.o

initex.o: itex.c texd.h $(extrah) $(commonh)
	rm -f initex.c
	$(LN) itex.c initex.c
	$(CC) $(CFLAGS) -DINITEX -c initex.c

$(objs): texd.h
tex-extra.o: $(extrac) texd.h
	rm -f tex-extra.c
	$(LN) $(extrac) tex-extra.c
	$(CC) $(CFLAGS) -I. -c tex-extra.c

$(fileioo): $(fileioc) $(commonh)
	$(CC) $(CFLAGS) -I. -c $(fileioc)

tex0.o: tex0.c $(commonh)
tex1.o: tex1.c $(commonh)
tex2.o: tex2.c $(commonh)
tex3.o: tex3.c $(commonh)
tex4.o: tex4.c $(commonh)
tex5.o: tex5.c $(commonh)
tex6.o: tex6.c $(commonh)
tex7.o: tex7.c $(commonh)
tex8.o: tex8.c $(commonh)
tex9.o: tex8.c $(commonh)


install: all
	$(INSTALL_PROGRAM) virtex $(bindir)/virtex
	$(INSTALL_PROGRAM) initex $(bindir)/initex
	-if test -d $(texpooldir); then exit 0; else mkdir $(texpooldir); fi
	-if test -s tex.pool; then $(INSTALL_DATA) tex.pool $(texpooldir); fi


mostlyclean: 
	rm -f stamp-* *.o triptex initex virtex
	rm -f texd.h coerce.h *.log *.fmt *.aux
	rm -f 8terminal.tex trip.* tripos.tex dvitype.out
	rm -f initex.c tex-extra.c

clean: mostlyclean
	rm -f tex?.c itex.c tex.p tex.pool

distclean: clean
	rm -f \#*\# *~ *.bak *.ckp core *.dvi texput.*
	rm -f ctex.ch

realclean: distclean
