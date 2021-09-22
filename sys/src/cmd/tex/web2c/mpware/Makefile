# Generated automatically from Makefile.in by configure.
# Makefile for the MetaPost support programs.

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
# programs.make -- used by Makefiles for executables only.

# Don't include $(CFLAGS), since ld -g under Linux forces
# static libraries, e.g., libc.a and libX*.a.
LDFLAGS =  $(XLDFLAGS)

# proglib is for web2c; 
# XLOADLIBES is for the installer.
LIBS =  -ldir
LOADLIBES = $(proglib) $(kpathsea) $(LIBS) -lm $(XLOADLIBES)

# May as well separate linking from compiling, just in case.
CCLD = $(CC)
link_command = $(CCLD) -o $@ $(LDFLAGS) 

# When we link with Kpathsea, have to take account that it might be a
# shared library, etc.
kpathsea_link = $(LIBTOOL) link $(link_command)
# End of programs.make.
# web2c/kpathsea.make -- In subdirectories of web2c, the build directory
# for kpathsea is one more level up.  c_auto_h_dir is used by make depend.
kpathsea_parent = ../..
c_auto_h_dir = ..

prog_cflags = -I.. -I$(srcdir)/..
# End of web2c/kpathsea.make.

proglib = ../lib/lib.a
programs = dmp mpto newer makempx

default: all
all: $(programs) 

dmp: dmp.o $(kpathsea)
	$(kpathsea_link) dmp.o $(LOADLIBES)
mpto: mpto.o
	$(link_command) mpto.o $(proglib) $(LIBS) $(XLOADLIBES)
newer: newer.o
	$(link_command) newer.o $(proglib) $(LIBS) $(XLOADLIBES)
makempx: makempx.in Makefile.in
	sed -e 's,@BINDIR@,$(bindir),g' -e 's,@web2cver@,7.2,g' \
	    <$(srcdir)/makempx.in >$@
	-chmod a+x $@

newer.o dmp.o: ../c-auto.h

checkdir = $(srcdir)/../check
check: mpto makempx newer dmp
	makempx $(checkdir)/label.mp ../check/xlabel.mpx
	./mpto -tex $(checkdir)/testex.mp | diff $(checkdir)/testex.tex -
	sed '16s/^%//' <$(checkdir)/testex.mp >testex1
	-mpto -tex testex1 >/dev/null 2>testex2
	sed '17s/^%//' <$(checkdir)/testex.mp >testex1
	-mpto -tex testex1 >/dev/null 2>>testex2
	sed '18s/^%//' <$(checkdir)/testex.mp >testex1
	-mpto -tex testex1 >/dev/null 2>>testex2
	sed '19s/^%//' <$(checkdir)/testex.mp >testex1
	-mpto -tex testex1 >/dev/null 2>>testex2
	sed '20s/^%//' <$(checkdir)/testex.mp >testex1
	-mpto -tex testex1 >/dev/null 2>>testex2
	diff $(checkdir)/testex.err testex2

Makefile: Makefile.in ../config.status
	cd .. && $(SHELL) config.status

install: install-exec install-data
install-exec: $(programs)
	$(SHELL) $(top_srcdir)/../mkinstalldirs $(bindir) $(scriptdir)
	for p in mpto newer; do $(INSTALL_PROGRAM) $$p $(bindir)/$$p; done
	$(INSTALL_LIBTOOL_PROG) $(bindir) dmp
	$(INSTALL_SCRIPT) makempx $(scriptdir)/makempx
install-data:

# tkpathsea.make -- target for remaking kpathsea.
makeargs = $(MFLAGS) CC='$(CC)' CFLAGS='$(CFLAGS)' $(XMAKEARGS)

# This is wrong: the library doesn't depend on kpsewhich.c or
# acconfig.h.  But what to do?
$(kpathsea): $(kpathsea_srcdir)/*.c $(kpathsea_srcdir)/*.h \
	     $(kpathsea_srcdir)/texmf.in $(top_srcdir)/../make/paths.make
	cd $(kpathsea_dir) && $(MAKE) $(makeargs)
# End of tkpathsea.make.
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

mostlyclean::
	rm -f testex1 testex2

# rdepend.make -- rules for remaking the dependencies.
#M## 
#M## Have to use -M, not -MM, since we use <kpathsea/...> instead of
#M## "kpathsea/..." in the sources.  But that means we have to remove the
#M## directory prefixes and all the system include files.
#M## And <kpathsea/paths.h> is generated, not part of the distribution.
#M## 
#M## And, there's no need for any installer/user to ever run this, it can
#M## only cause trouble. So comment it out in the distribution.
#M## (It doesn't work when the source and build directories are different.)
#M#ifndef c_auto_h_dir
#M#c_auto_h_dir = .
#M#endif
#M#
#M#depend depend.make:: $(c_auto_h_dir)/c-auto.h \
#M#  $(top_srcdir)/../make/rdepend.make 
#M#	$(CC) -M $(ALL_CPPFLAGS) -I$(c_auto_h_dir) *.c \
#M#	  | sed -e 's,\(\.\./\)\+kpathsea/,$$(kpathsea_srcdir)/,g' \
#M#	        -e 's,$$(kpathsea_srcdir)/c-auto.h,$$(kpathsea_dir)/c-auto.h,g' \
#M#	        -e 's,$$(kpathsea_srcdir)/paths.h,$$(kpathsea_dir)/paths.h,g' \
#M#	        -e 's,/usr[^ ]* ,,g' \
#M#	        -e 's,/usr[^ ]*$$,,g' \
#M#	        -e 's,dvi2xx.o,dvilj.o dvilj2p.o dvilj4.o dvilj4l.o,' \
#M#	        -e 's,lex.yy,$$(LEX_OUTPUT_ROOT),g' \
#M#	  | $(top_srcdir)/../unbackslsh.awk \
#M#	  >depend.make
#M## If kpathsea, we're making .lo library objects instead of .o's.
#M#	pwd | grep -v kpathsea >/dev/null \
#M#	  || (sed -e 's/\.o:/.lo:/' \
#M#	          -e 's/kpsewhich.lo:/kpsewhich.o:/' \
#M#	          -e 's/kpsestat.lo:/kpsestat.o:/' \
#M#	          -e 's/access.lo:/access.o:/' \
#M#	          -e 's/readlink.lo:/readlink.o:/' \
#M#	      <depend.make >depend-tmp.make; mv depend-tmp.make depend.make)
#M#.PHONY: depend

# Let's stick a rule for TAGS here, just in case someone wants them.
# (We don't put them in the distributions, to keep them smaller.)
TAGS: *.c *.h
	pwd | grep kpathsea >/dev/null && append=../kpathsea/TAGS; \
	  etags $$append *.[ch]

# Prevent GNU make 3.[59,63) from overflowing arg limit on system V.
.NOEXPORT:

# End of rdepend.make.
dmp.o: dmp.c ../config.h $(kpathsea_srcdir)/config.h ../c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/tex-file.h \
 $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/c-pathch.h
mpto.o: mpto.c ../c-auto.h 
newer.o: newer.c ../c-auto.h 
