# Generated automatically from Makefile.in by configure.
# Makefile for the Metafont window support library.

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
# library.make -- stuff only useful for libraries.
AR = ar
ARFLAGS = cq
RANLIB = :
# End of library.make.
# web2c/kpathsea.make -- In subdirectories of web2c, the build directory
# for kpathsea is one more level up.  c_auto_h_dir is used by make depend.
kpathsea_parent = ../..
c_auto_h_dir = ..

prog_cflags = -I.. -I$(srcdir)/..
# End of web2c/kpathsea.make.

x_cppflags = 
library = window

# Extra library-specific compiler options.
prog_cflags = -I.. -I$(srcdir)/.. $(x_cppflags)

objects = amiga.o epsf.o hp2627.o mftalk.o next.o regis.o suntools.o tek.o \
trap.o uniterm.o win32.o x11.o 

default all: $(library).a
$(library).a: $(objects)
	rm -f $@
	$(AR) $(ARFLAGS) $@ $(objects)
	$(RANLIB) $@

# It doesn't matter much what this default is, since if anything was
# enabled configure would hopefully have found it.
x11.c:
	rm -f $@
	$(LN) $(srcdir)/x11-Xt.c $@

Makefile: Makefile.in ../config.status
	cd .. && $(SHELL) config.status

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

distclean::
	rm -f x11.c

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
amiga.o: amiga.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h
epsf.o: epsf.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h
hp2627.o: hp2627.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h
mftalk.o: mftalk.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h
next.o: next.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h
regis.o: regis.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h
suntools.o: suntools.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h
tek.o: tek.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h
trap.o: trap.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h
uniterm.o: uniterm.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h
win32.o: win32.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h
x11-Xlib.o: x11-Xlib.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h 
x11-Xt.o: x11-Xt.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h 
x11.o: x11.c ../mfd.h ../texmfmp.h ../cpascal.h ../config.h \
 $(kpathsea_srcdir)/config.h ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h ../help.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tex-make.h ../texmfmem.h \
 ../mfcoerce.h 
