# Generated automatically from Makefile.in by configure.
# Makefile for TeX-project WEB sources to C conversion programs.

# These get expanded from the parent directory, not this one.
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

LEX = lex
LEXLIB = -ll
LEX_OUTPUT_ROOT = lex.yy
YACC = yacc

proglib = ../lib/lib.a
programs = web2c fixwrites splitup

web2c_objects = main.o y_tab.o $(LEX_OUTPUT_ROOT).o
all: $(programs)
web2c: $(web2c_objects)
	$(kpathsea_link) $(web2c_objects) $(LEXLIB) $(LOADLIBES) 

# We use y_tab.* for the sake of MS-DOS.
main.o: main.c y_tab.h
y_tab.c y_tab.h: web2c.y
	@echo Expect one shift/reduce conflict.
	$(YACC) -d -v $(srcdir)/web2c.y
	-test -f y.tab.c && mv -f y.tab.c y_tab.c
	-test -f y.tab.h && mv -f y.tab.h y_tab.h
$(LEX_OUTPUT_ROOT).o: y_tab.h
$(LEX_OUTPUT_ROOT).c: web2c.l
	$(LEX) $(srcdir)/web2c.l

fixwrites: fixwrites.o
	$(kpathsea_link) fixwrites.o $(LOADLIBES)
splitup: splitup.o
	$(kpathsea_link) splitup.o $(LOADLIBES)
regfix: regfix.o
	$(kpathsea_link) regfix.o $(LOADLIBES)

$(programs): $(proglib) $(kpathsea)

#M## It's really not worth srcdir etc. here.
#M#lib_sources = $(addprefix ../lib/,alloca.c basechsuffix.c chartostring.c \
#M#  eofeoln.c fprintreal.c input2int.c inputint.c main.c openclose.c \
#M#  uexit.c usage.c version.c zround.c)
#M#$(proglib): $(lib_sources)
#M#	cd ../lib && $(MAKE)
#M#always:

Makefile: Makefile.in ../config.status
	cd .. && $(SHELL) config.status

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

clean::
	rm -f y_tab.c y_tab.h y.out* yacc.* $(LEX_OUTPUT_ROOT).c

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
fixwrites.o: fixwrites.c ../config.h $(kpathsea_srcdir)/config.h \
 ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/c-pathmx.h
$(LEX_OUTPUT_ROOT).o: $(LEX_OUTPUT_ROOT).c \
 web2c.h ../config.h $(kpathsea_srcdir)/config.h \
 ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 y_tab.h
main.o: main.c web2c.h ../config.h $(kpathsea_srcdir)/config.h ../c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h \
 y_tab.h
regfix.o: regfix.c ../config.h $(kpathsea_srcdir)/config.h ../c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h 
splitup.o: splitup.c ../config.h $(kpathsea_srcdir)/config.h ../c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h 
y_tab.o: y_tab.c web2c.h ../config.h $(kpathsea_srcdir)/config.h \
 ../c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h $(kpathsea_srcdir)/debug.h \
 $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h $(kpathsea_srcdir)/progname.h \
 $(kpathsea_srcdir)/c-fopen.h 
