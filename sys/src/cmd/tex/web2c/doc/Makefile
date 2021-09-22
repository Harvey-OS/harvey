# Generated automatically from Makefile.in by configure.
# Makefile for Web2c documentation.

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
# texi.make -- making .dvi and .info from .texi.
MAKEINFO = makeinfo
MAKEINFO_FLAGS = --paragraph-indent=2 -I$(srcdir)

TEXI2DVI = texi2dvi
#M## To make sure that generation of the dvi files succeeds on the
#M## maintainer's system.
#M#TEXI2DVI = TEXMFCNF=$(web2cdir) texi2dvi

TEXI2HTML = texi2html
TEXI2HTML_FLAGS = -expandinfo -number -menu -split_chapter
# If you prefer one big .html file instead of several, remove
# -split-node or replace it by -split_chapter.

# For making normal text files out of Texinfo source.
one_info = --no-headers --no-split --no-validate

.SUFFIXES: .info .dvi .html .texi
.texi.info:
	$(MAKEINFO) $(MAKEINFO_FLAGS) $< -o $@
.texi.dvi:
	$(TEXI2DVI) $(TEXI2DVI_FLAGS) $<
.texi.html:
	$(TEXI2HTML) $(TEXI2HTML_FLAGS) $< 
# End of texi.make.

info: web2c.info
dvi: web2c.dvi

web2c.info: install.texi ref.txi
all: web2c.info web2c.dvi

#M#INSTALL: install.texi
#M#	$(MAKEINFO) $(one_info) $(MAKEINFO_FLAGS) $< -o $@
#M#	$(SHELL) $(top_srcdir)/../add-info-toc $@
#M#doc: info INSTALL
#M#
#M#ref.txi: ref.bbl
#M#	sed -e 's/~/ /g' \
#M#	    -e 's/\\[-`'\''"c^]//g' \
#M#	    -e 's/\\emdash */---/g' \
#M#	    -e 's/metapost/MetaPost/g' \
#M#	    -e 's/\\MF/Metafont/g' \
#M#	    -e 's/\\METAFONT/METAFONT/g' \
#M#	    -e 's/\\TUB/TUGboat/g' \
#M#	    -e 's/\\LaTeX/La@TeX/g' \
#M#	    -e 's/\\AMSTEX/AMS@TeX/g' \
#M#	    -e 's/\\AmSTeX/AMS@TeX/g' \
#M#	    -e 's/\\TeX/@TeX/g' \
#M#	    -e 's/\\noopsort{[^}]*}//g' \
#M#	    -e 's/[{}\]//g' \
#M#	    -e 's/@\[/{/g' \
#M#	    -e 's/@\]/}/g' \
#M#	    -e 's/@TeX/@TeX{}/g' \
#M#	$< >$@
#M#
#M#ref.bbl: ref.aux texinfo.bst
#M#	-bibtex ref
#M#
#M#ref.aux: ref.tex
#M#	rm -f ref.bbl
#M#	tex '\nonstopmode \input ref'

Makefile: Makefile.in ../config.status
	cd .. && $(SHELL) config.status

install: install-exec install-data
install-exec:
install-data: web2c.info
	$(SHELL) $(top_srcdir)/../mkinstalldirs $(infodir)
	test -r web2c.info || cd $(srcdir) && for i in web2c.i*; do \
	  $(INSTALL_DATA) $$i $(infodir)/$$i; done
	$(POSTINSTALL)
	if $(SHELL) -c 'install-info --version' >/dev/null 2>&1; then \
	  install-info --info-dir=$(infodir) $(infodir)/web2c.info; \
	else true; fi

check: all

#html: texi2html -expandinfo -number -menu -split_chapter

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
