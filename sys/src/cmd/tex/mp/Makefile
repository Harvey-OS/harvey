# Directory that site.h is in. (`..' means the directory containing
# this Makefile.)
# 
SITEDIR=..

# Default C compiler.
CC=pcc

# Default flags to give the C compiler.  (Don't define CFLAGS directly.)
OPT=-g

# Default flags to give to the loader.
LDFLAGS=-g


default: all


##########################################################################
# If you have the web2c vesion of the TeX distribution, the next few lines
# can be merged with the top-level makefile web2c/Makefile
##########################################################################

# You only need to define these if you are going to do `make install',
# instead of installing the programs by hand.  They should match the
# paths you define in ./site.h.
# 
INSTALL=cp
bindir=/usr/bin
specbin=/usr/lib/tex/bin
makempxbin=/usr/lib/mp
mppooldir=/usr/lib/mp
memdir=/usr/lib/mp
mpinputs=/usr/lib/mp
trtables=/usr/lib/mp
trchars=/usr/lib/mp/charlib
texinputs=/usr/lib/tex/macros
texdocs=/usr/lib/tex/macros/doc

# It probably isn't worthwhile to make preloaded versions of MetaPost
# so the standard name "mp" should refer to virmp
#
VIRMP=mp


alldirs=mp mpware
triptrapdirs=mp

install:
	$(INSTALL) mp/inimp $(specbin)
	$(INSTALL) mp/virmp $(bindir)/$(VIRMP)
	if test -s mp/mp.pool; then $(INSTALL) mp/mp.pool $(mppooldir); fi
	-$(INSTALL) mp/*.mem $(memdir)
	$(INSTALL) mpware/dvitomp mpware/makempx mpware/mptotex $(makempxbin)
	$(INSTALL) mpware/dmp mpware/troffmpx mpware/mptotr $(makempxbin)
	$(INSTALL) mplib/trfonts.map mplib/trchars.adj $(trtables)
	$(INSTALL) mplib/charlib/* $(trchars)
	$(INSTALL) mplib/*.mp $(mpinputs)
	$(INSTALL) mplib/mproof.tex $(texinputs)
	$(INSTALL) mplib/mpintro.tex $(texdocs)

############################# end of lines to merge ##############################



makeargs=SITEDIR="$(SITEDIR)" CC="$(CC)" OPT="$(OPT)" \
SHELL="$(SHELL)" LDFLAGS="$(LDFLAGS)" wlibs="$(wlibs)"

# Make sure we use the sh.  If your sh is broken, you might try bash,
# the GNU version.
# 
SHELL=/bin/sh


triptrap:
	for name in $(triptrapdirs); \
        do \
          (cd $${name}; make $(makeargs) triptrap); \
        done  

run-triptrap:
	cd mp; make run-triptrap
	cd mpware; make run-triptrap

clean-triptrap:
	cd mp; make veryclean
	cd mpware; make clean


all:
	for name in $(alldirs); \
	do \
	  (cd $${name}; echo $${name}; make $(makeargs) all); \
	done

clean:
	for name in $(alldirs); \
	do \
	  (cd $${name}; make clean); \
	done

veryclean:
	for name in $(alldirs); \
	do \
	  (cd $${name}; make veryclean); \
	done
	rm -f \#*\# *~ *.bak *.ckp core
