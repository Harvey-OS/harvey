# Generated automatically from Makefile.in by configure.
# Top-level Makefile for kpathsea-using programs.

# Package subdirectories, the library, and all subdirectories.
ESUBDIRS= dtl dvidvi dviljk dvipsk gsftopk lacheck makeindexk musixflx odvipsk oxdvik ps2pkm seetexk tetex tex4htk xdvik
DSUBDIRS=

programs = web2c $(ESUBDIRS)
kpathsea_dir = kpathsea
all_dirs = $(programs) $(kpathsea_dir) contrib

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
# makevars.make -- the directory names we pass.
# It's important that none of these values contain [ @%], for the sake
# of kpathsea/texmf.sed.
makevars = prefix=$(prefix) exec_prefix=$(exec_prefix) \
  bindir=$(bindir) scriptdir=$(scriptdir) libdir=$(libdir) \
  datadir=$(datadir) infodir=$(infodir) includedir=$(includedir) \
  manext=$(manext) mandir=$(mandir) \
  texmf=$(texmf) web2cdir=$(web2cdir) vartexfonts=$(vartexfonts)\
  texinputdir=$(texinputdir) mfinputdir=$(mfinputdir) mpinputdir=$(mpinputdir)\
  fontdir=$(fontdir) fmtdir=$(fmtdir) basedir=$(basedir) memdir=$(memdir) \
  texpooldir=$(texpooldir) mfpooldir=$(mfpooldir) mppooldir=$(mppooldir) \
  dvips_plain_macrodir=$(dvips_plain_macrodir) \
  dvilj_latex2e_macrodir=$(dvilj_latex2e_macrodir) \
  dvipsdir=$(dvipsdir) psheaderdir=$(psheaderdir) \
  default_texsizes='$(default_texsizes)'
# End of makevars.make.

# It's too bad we have to pass all these down, but I see no alternative,
# if we are to propagate changes at the top level.
# XMAKEARGS is for the user to override.
makeargs = $(MFLAGS) CC='$(CC)' CFLAGS='$(CFLAGS)' $(makevars) $(XMAKEARGS)
installargs = INSTALL_DATA='$(INSTALL_DATA)' \
  INSTALL_PROGRAM='$(INSTALL_PROGRAM)' $(makeargs)

# Not everything from common.make is relevant to this top-level
# Makefile, but most of it is, and it doesn't seem worth separating the
# compilation-specific stuff.
# common.make -- used by all Makefiles.
SHELL = /bin/sh

top_srcdir = .
srcdir = .

CC = c89
CFLAGS = -g $(XCFLAGS)
CPPFLAGS =  $(XCPPFLAGS)
DEFS = \#include\ \<stdio.h\> -DMAKE_TEX_MF_BY_DEFAULT=1 -DMAKE_TEX_PK_BY_DEFAULT=1 -DMAKE_TEX_TFM_BY_DEFAULT=1 -DMAKE_OMEGA_OCP_BY_DEFAULT=1 -DMAKE_OMEGA_OFM_BY_DEFAULT=1  $(XDEFS)

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
INSTALL = /usr/rsc/tex/web2c/./install.sh -c
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

# kpathsea is not a sibling, it's a child.
kpathsea_parent = .

# Make the library before the programs.
# Use if ... rather than || or && because Ultrix sh exits for
# no good reason with the latter.
default all: do-kpathsea
	for d in $(programs); do if test -d $$d; then \
	  (cd $$d && $(MAKE) $(makeargs) $@) || break; else true; fi; done

install install-data install-exec:
	for d in kpathsea $(programs); do if test -d $$d; then \
	  (cd $$d && $(MAKE) $(installargs) $@) || break; else true; fi; done

# Other standard targets for everything.
uninstall uninstall-exec uninstall-data \
mostlyclean clean realclean extraclean configclean \
info dvi check depend::
	for d in $(all_dirs); do if test -d $$d; then \
	  (cd $$d && $(MAKE) $(makeargs) $@) || break; else true; fi; done

distclean::
	for d in $(all_dirs) $(DSUBDIRS); do if test -d $$d; then \
	  (cd $$d && $(MAKE) $(makeargs) $@) || break; else true; fi; done

distclean realclean::
	rm -f Makefile config.status config.cache config.log klibtool.config

# Unconditionally remake the library, since we don't want to write out
# the dependencies here.
do-kpathsea:
	cd $(kpathsea_dir) && $(MAKE) $(makeargs)

# Targets that only apply to web2c.
triptrap trip trap mptrap \
formats fmts bases mems \
install-formats install-fmts install-bases install-mems \
install-links c-sources: do-kpathsea
	cd web2c && $(MAKE) $(makeargs) $@

#M#ac_dir = $(srcdir)/etc/autoconf
#M#autoconf = $(ac_dir)/acspecific.m4 $(ac_dir)/acgeneral.m4 $(ac_dir)/acsite.m4
#M#configure_in = $(srcdir)/configure.in
#M#$(srcdir)/configure: $(configure_in) $(srcdir)/withenable.ac $(autoconf)
#M#	cd $(srcdir) && autoconf -m $(ac_dir)
#M#
#M## And make sure that --enable-maintainer-mode is used if configure is
#M## called from the makefiles.  We do not try to do this through the
#M## cache, lest the option sneaks into a global cache file.
#M#enablemaintflag = --enable-maintainer-mode

config.status: $(srcdir)/configure
	$(SHELL) $(srcdir)/configure --no-create --verbose  $(enablemaintflag)

Makefile: $(srcdir)/Makefile.in config.status
	$(SHELL) config.status

TAGS:
	find $(srcdir) \( -name '*.[cCfFhilsy]' -o -name '*.el' \) | etags -
