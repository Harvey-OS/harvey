# Makefile for manual pages.
# 
# You need to change the variables and the pathnames only if this
# Makefile is invoked on its own, instead of from `../Makefile'.

# Where to put the man pages.
srcdir=.
manext=l
mandir=/usr/local/man/man$(manext)

# These pathnames are used to customize the manual pages with
# local directory names.  They should match the system directories
# in the paths defined in `../config/site.h'.  The definitions aren't
# exactly the same as in ../Makefile.in, since the ones there will
# override the ones here, and the Makefile here should never be used
# directly.
# 
bindir=/usr/local/bin
texdatadir=/usr/local/lib/tex
texpooldir=$(texdatadir)
texinputdir=$(texdatadir)/macros
texfontdir=$(texdatadir)/fonts
formatdir=$(texdatadir)/formats

mfdatadir=/usr/local/lib/mf
mfpooldir=$(mfdatadir)
mfinputdir=$(mfdatadir)/macros
basedir=$(mfdatadir)/bases

cmsources=/usr/local/src/TeX+MF/typefaces/cm

localmodes=modes

# Delete the man pages for programs you aren't installing.  
# The programs are listed in alphabetical order.
# 
manfiles=		\
	bibtex.1	\
	dvitype.1	\
	gftodvi.1	\
	gftopk.1	\
	gftype.1	\
	latex.1		\
	mf.1		\
	mft.1		\
	patgen.1	\
	pktogf.1	\
	pktype.1	\
	pltotf.1	\
	pooltype.1	\
	slitex.1	\
	tangle.1	\
	tex.1 		\
	tftopl.1	\
	vftovp.1	\
	vptovf.1	\
	weave.1


# The edited file always has extension .1; we change it when we install.
# 
.SUFFIXES: .man .1
.man.1:
	sed -f sedscript $*.man > $*.1

all: $(manfiles)

$(manfiles): sedscript

# We do not make a dependency on the top-level Makefile since not all
# people keep the Makefile in the same directory as site.h; furthermore,
# the top-level Makefile usually changes for reasons other than those
# that affect the man pages, so we don't actually lose much.
# 
sedscript: ../$(srcdir)/lib/site.h Makefile
	sed -n -e '/^#define/s/#define[ 	][ 	]*\([A-Z_a-z][A-Z_a-z]*\)[ 	][ 	]*\(.*\)/s;@\1@;\2;/p' \
		../$(srcdir)/lib/site.h | \
	sed -e 's/"//g' -e 's/[ 	]*\/\*[^*]*\*\///g' > $@
	echo 's;@BINDIR@;$(bindir);' >> $@
	echo 's;@TEXLIBDIR@;$(texdatadir);' >> $@
	echo "s;@TEXPOOLDIR@;$(texpooldir);" >> $@
	echo 's;@FORMATDIR@;$(formatdir);' >> $@
	echo 's;@TEXINPUTDIR@;$(texinputdir);' >> $@
	echo 's;@TEXFONTDIR@;$(texfontdir);' >> $@
	echo 's;@MFLIBDIR@;$(mfdatadir);' >> $@
	echo 's;@MFPOOLDIR@;$(mfpooldir);' >> $@
	echo 's;@BASEDIR@;$(basedir);' >> $@
	echo 's;@MFINPUTDIR@;$(mfinputdir);' >> $@
	echo 's;@CMSOURCES@;$(cmsources);' >> $@
	echo 's;@LOCALMODES@;$(localmodes);' >> $@

install: all
	-if test -d $(mandir); then exit 0; else mkdir $(mandir); fi
	for nameone in $(manfiles);					\
        do								\
          name=`basename $${nameone} .1`;				\
          $(INSTALL_DATA) $${name}.1 $(mandir)/$${name}.$(manext);	\
        done

mostlyclean:
	rm -f sedscript *.1

distclean realclean clean: mostlyclean
