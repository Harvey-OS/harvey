# Overall Makefile for making TeX, Metafont, and friends, using WEB to C.
# 

# TeX 3.0 + Metafont 2.0 + web2c c
version=5.0c

# Directory that site.h is in. (`..' means the directory containing
# this Makefile.)
# 
SITEDIR=..

# Default C compiler.
CC=/usr/ape/apebin/pcc

# Default flags to give the C compiler.  (Don't define CFLAGS directly.)
OPT=

# Default flags to give to the loader.
LDFLAGS=

# Support libraries for Metafont.
x10lib=-lX
x11lib=-lXt -lX11
sunlib=-lsuntool -lsunwindow -lpixrect

# This line should select the libraries that match what you said in site.h.
wlibs=${x11lib}


default: all


# Stuff below here probably doesn't need to be changed.
# The order here doesn't really matter, but it's nice to do TeX first,
# since that's what most people care about.
# 
alldirs=web tex texware mf mfware fontutil bibtex
triptrapdirs=tex texware mf mfware

makeargs=SITEDIR="$(SITEDIR)" CC="$(CC)" OPT="$(OPT)" \
SHELL="$(SHELL)" LDFLAGS="$(LDFLAGS)" wlibs="$(wlibs)"

# Make sure we use the sh.  If your sh is broken, you might try bash,
# the GNU version.
# 
SHELL=/bin/sh

triptrap: stamp-web2c stamp-tangle
	for name in $(triptrapdirs); \
        do \
          (cd $${name}; make $(makeargs) triptrap); \
        done  

run-triptrap:
	cd tex; make run-triptrap
	cd mf; make run-triptrap

clean-triptrap:
	cd tex; make veryclean
	cd mf; make veryclean

all:	stamp-web2c
	for name in $(alldirs); \
        do \
          (cd $${name}; echo $${name}; make $(makeargs) all); \
        done

TeX:	stamp-web2c stamp-tangle
	cd tex; make $(makeargs) all

MF:	stamp-web2c stamp-tangle
	cd mf; make $(makeargs) all

stamp-web2c:
	cd web2c; make $(makeargs) all
	touch stamp-web2c

stamp-tangle:
	cd web; make $(makeargs) tangle
	touch stamp-tangle

clean:
	rm -f stamp-*
	for name in web2c $(alldirs); \
        do \
          (cd $${name}; make clean); \
        done

veryclean:
	rm -f stamp-*
	for name in web2c $(alldirs); \
        do \
          (cd $${name}; make veryclean); \
        done
	rm -f \#*\# *~ *.bak *.ckp core
