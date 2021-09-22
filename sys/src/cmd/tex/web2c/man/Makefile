# Generated automatically from Makefile.in by configure.
# Makefile for web2c manual pages.
# 
# You need to change the variables and the pathnames only if this
# Makefile is invoked on its own, instead of from `../Makefile'.

version = 7.2

SHELL = /bin/sh
top_srcdir = ..
srcdir = .

# common.make -- used by all Makefiles.
SHELL = /bin/sh

top_srcdir = ..
srcdir = .

CC = c89
CFLAGS = -g $(XCFLAGS)
CPPFLAGS =  $(XCPPFLAGS)
DEFS = -DHAVE_CONFIG_H $(XDEFS)

# Kpathsea needs this for compiling, programs need it for linking.
LIBTOOL = $(kpathsea_srcdir_parent)/klibtool

# You can change [X]CPPFLAGS, [X]CFLAGS, or [X]DEFS, but
# please don't change ALL_CPPFLAGS or ALL_CFLAGS.
# prog_cflags is set by subdirectories of web2c.
ALL_CPPFLAGS = $(DEFS) -I. -I$(srcdir) $(prog_cflags) \
  -I$(kpathsea_parent) -I$(kpathsea_srcdir_parent) $(CPPFLAGS)
ALL_CFLAGS = $(ALL_CPPFLAGS) $(CFLAGS) -c
compile = $(CC) $(ALL_CFLAGS)

.SUFFIXES: .c .o # in case the suffix list has been cleared, e.g., by web2c
.c.o:
	$(compile) $<

# Installation.
INSTALL = /usr/rsc/tex/web2c/web2c/./../install.sh -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_SCRIPT = $(INSTALL_PROGRAM)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_LIBTOOL_LIBS = INSTALL_DATA='$(INSTALL_DATA)' $(LIBTOOL) install-lib
INSTALL_LIBTOOL_PROG = INSTALL_PROGRAM='$(INSTALL_PROGRAM)' $(LIBTOOL) install-prog

# Creating (symbolic) links.
LN = ln

# We use these for many things.
kpathsea_parent = ..
kpathsea_dir = $(kpathsea_parent)/kpathsea
kpathsea_srcdir_parent = $(top_srcdir)/..
kpathsea_srcdir = $(kpathsea_srcdir_parent)/kpathsea
kpathsea = $(kpathsea_dir)/libkpathsea.la

#M#ifeq ($(CC), gcc)
#M#XDEFS = -D__USE_FIXED_PROTOTYPES__ -Wall -Wpointer-arith $(warn_more)
#M#CFLAGS = -pipe -g $(XCFLAGS)
#M#endif
# End of common.make.
# web2c/kpathsea.make -- In subdirectories of web2c, the build directory
# for kpathsea is one more level up.  c_auto_h_dir is used by make depend.
kpathsea_parent = ../..
c_auto_h_dir = ..

prog_cflags = -I.. -I$(srcdir)/..
# End of web2c/kpathsea.make.

INSTALL = /usr/rsc/tex/web2c/web2c/./../install.sh -c
INSTALL_DATA = ${INSTALL} -m 644

DITROFF = ditroff
DITROFF = groff

# These pathnames are used to customize the manual pages with local
# directory names.  They should match the system directories so the
# documentation and the programs will be consistent. They are overridden
# by make from web2c/Makefile.
# paths.make -- installation directories.
#
# The compile-time paths are defined in kpathsea/paths.h, which is built
# from kpathsea/texmf.in and these definitions.  See kpathsea/INSTALL
# for how the various path-related files are used and created.

# Do not change prefix and exec_prefix in Makefile.in!
# configure doesn't propagate the change to the other Makefiles.
# Instead, give the -prefix/-exec-prefix options to configure.
# (See kpathsea/INSTALL for more details.) This is arguably
# a bug, but it's not likely to change soon.
prefix = /usr/local
exec_prefix = ${prefix}

# Architecture-dependent executables.
bindir = ${exec_prefix}/bin

# Architecture-independent executables.
scriptdir = $(bindir)

# Architecture-dependent files, such as lib*.a files.
libdir = ${exec_prefix}/lib

# Architecture-independent files.
datadir = ${prefix}/share

# Header files.
includedir = ${prefix}/include

# GNU .info* files.
infodir = ${prefix}/info

# Unix man pages.
manext = 1
mandir = ${prefix}/man/man$(manext)

# TeX system-specific directories. Not all of the following are relevant
# for all programs, but it seems cleaner to collect everything in one place.

# The default paths are now in kpathsea/texmf.in. Passing all the
# paths to sub-makes can make the arg list too long on system V.
# Note that if you make changes below, you will have to make the
# corresponding changes to texmf.in or texmf.cnf yourself.

# The root of the main tree.
texmf = ${prefix}/share/texmf

# The directory used by varfonts.
vartexfonts = /var/tmp/texfonts

# Regular input files.
texinputdir = $(texmf)/tex
mfinputdir = $(texmf)/metafont
mpinputdir = $(texmf)/metapost
mftinputdir = $(texmf)/mft

# dvips's epsf.tex, rotate.tex, etc. get installed here;
# ditto for dvilj's fonts support.
dvips_plain_macrodir = $(texinputdir)/plain/dvips
dvilj_latex2e_macrodir = $(texinputdir)/latex/dvilj

# mktex.cnf, texmf.cnf, etc.
web2cdir = $(texmf)/web2c

# The top-level font directory.
fontdir = $(texmf)/fonts

# Memory dumps (.fmt/.base/.mem).
fmtdir = $(web2cdir)
basedir = $(fmtdir)
memdir = $(fmtdir)

# Pool files.
texpooldir = $(web2cdir)
mfpooldir = $(texpooldir)
mppooldir = $(texpooldir)

# Where the .map files from fontname are installed.
fontnamedir = $(texmf)/fontname

# For dvips configuration files, psfonts.map, etc.
dvipsdir = $(texmf)/dvips

# For dvips .pro files, gsftopk's render.ps, etc.
psheaderdir = $(dvipsdir)

# If a font can't be found close enough to its stated size, we look for
# each of these sizes in the order given.  This colon-separated list is
# overridden by the envvar TEXSIZES, and by a program-specific variable
# (e.g., XDVISIZES), and perhaps by a config file (e.g., in dvips).
# This list must be sorted in ascending order.
default_texsizes = 300:600

# End of paths.make.

localmodes = modes

# If you like, delete the man pages for programs you aren't installing.
# The programs are listed in alphabetical order.
# 
manfiles=		\
	amstex.1	\
	bibtex.1	\
	dmp.1		\
	dvicopy.1	\
	dvitype.1	\
	eplain.1	\
	etex.1		\
	gftodvi.1	\
	gftopk.1	\
	gftype.1	\
	latex.1		\
	makempx.1	\
	mf.1		\
	mft.1		\
	mpost.1		\
	mpto.1		\
	newer.1		\
	omega.1		\
	patgen.1	\
	pdftex.1	\
	pktogf.1	\
	pktype.1	\
	pltotf.1	\
	pooltype.1	\
	tangle.1	\
	tex.1 		\
	tftopl.1	\
	vftovp.1	\
	vptovf.1	\
	weave.1

default all: manfiles

# man.make: Makefile fragment for web2c manual pages.

#DITROFF = ditroff
DITROFF = groff

# The edited file always has extension .1; we change it when we install.
.SUFFIXES: .man .1 .txt .ps .dvi
.man.1:
	sed -f sedscript $< >$@
.1.dvi:
	$(DITROFF) -Tdvi -man $< >$@
.1.ps:
	$(DITROFF) -Tps -man $< >$@
.1.txt:
	$(DITROFF) -Tascii -man $< | col -b | expand >$@

all: $(manfiles)
.PHONY: dw

$(manfiles): sedscript

manfiles: $(manfiles)
dvi: $(manfiles:.1=.dvi)
ps: $(manfiles:.1=.ps)
txt: $(manfiles:.1=.txt)

# We do not depend on the top-level Makefile since the top-level
# Makefile can change for reasons that do not affect the man pages.
# At present, all but VERSION should be unused.
sedscript:
	cp /dev/null sedscript
	for f in $(kpathsea_dir)/paths.h; do \
	  sed -n -e '/^#define/s/#define[ 	][ 	]*\([A-Z_a-z][A-Z_a-z]*\)[ 	][ 	]*\(.*\)/s%@\1@%\2%/p' \
		$$f \
	  | sed -e 's/"//g' -e 's/[ 	]*\/\*[^*]*\*\///g' >>sedscript;\
	done
	echo 's%@VERSION@%$(version)%'		>>sedscript
	echo 's%@BINDIR@%$(bindir)%'		>>sedscript
	echo 's%@INFODIR@%$(infodir)%'		>>sedscript
	echo 's%@TEXINPUTDIR@%$(texinputdir)%'	>>sedscript
	echo 's%@MFINPUTDIR@%$(mfinputdir)%'	>>sedscript
	echo 's%@MPINPUTDIR@%$(mpinputdir)%'	>>sedscript
	echo 's%@FONTDIR@%$(fontdir)%'		>>sedscript
	echo 's%@FMTDIR@%$(fmtdir)%'		>>sedscript
	echo 's%@BASEDIR@%$(basedir)%'		>>sedscript
	echo 's%@MEMDIR@%$(memdir)%'		>>sedscript
	echo 's%@TEXPOOLDIR@%$(texpooldir)%'	>>sedscript
	echo 's%@MFPOOLDIR@%$(mfpooldir)%'	>>sedscript
	echo 's%@MPPOOLDIR@%$(mppooldir)%'	>>sedscript
	echo 's%@FONTMAPDIR@%$(dvipsdir)%'	>>sedscript
	echo 's%@LOCALMODES@%$(localmodes)%'	>>sedscript

install-man: manfiles
	$(top_srcdir)/../mkinstalldirs $(mandir)
	for nameone in $(manfiles); do					\
          name=`basename $${nameone} .1`;				\
          $(INSTALL_DATA) $${name}.1 $(mandir)/$${name}.$(manext);	\
        done

uninstall-man:
	for nameone in $(manfiles); do					\
	  name=`basename $${nameone} .1`;				\
	  rm -f $(mandir)/$${name}.$(manext);				\
	done

install-data: install-man
uninstall-data: uninstall-man

mostlyclean::
	rm -f *.1

clean::
	rm -f sedscript

# end of man.make

install install-data: install-man

Makefile: Makefile.in ../config.status
	(cd ..; $(SHELL) config.status)

# clean.make -- cleaning.
mostlyclean::
	rm -f *.o

clean:: mostlyclean
	rm -f $(program) $(programs) squeeze lib$(library).* $(library).a *.bad
	rm -f *.exe *.dvi *.lj

distclean:: extraclean clean
	rm -f Makefile
	rm -f config.status config.log config.cache c-auto.h
	rm -f stamp-auto stamp-tangle stamp-otangle

# Although we can remake configure and c-auto.in, we don't remove
# them, since many people may lack Autoconf.  Use configclean for that.
maintainer-clean:: distclean
	rm -f *.info*

extraclean::
	rm -f *.aux *.bak *.bbl *.blg *.dvi *.log *.pl *.tfm *.vf *.vpl
	rm -f *.*pk *.*gf *.mpx *.i *.s *~ *.orig  *.rej *\#*
	rm -f CONTENTS.tex a.out core mfput.* texput.* mpout.*

configclean:
	rm -f configure c-auto.in c-auto.h stamp-*
# End of clean.make.

# Prevent GNU make 3 from overflowing arg limit on system V.
.NOEXPORT:
