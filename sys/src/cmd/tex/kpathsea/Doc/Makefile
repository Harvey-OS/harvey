# Generated automatically from Makefile.in by configure.
# Makefile for kpathsea --kb@mail.tug.org.  Public domain.
version = 3.2

# Add -DNO_DEBUG to disable debugging, for vanishingly better performance.

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
prefix = /sys/lib/
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
texmf = /sys/lib/texmf

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
# common.make -- used by all Makefiles.
SHELL = /bin/sh

top_srcdir = .
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
INSTALL = /usr/rsc/tex/web2c/kpathsea/./../install.sh -c
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

#M#warn_more = -Wmissing-prototypes -Wwrite-strings -Wshadow
#M#MAKEINFO_FLAGS := $(MAKEINFO_FLAGS) --no-split

# Install these header files (except acconfig.h).
install_headers = *.h $(srcdir)/*.h

# Put tex-file.o first, because it's what depends on the paths, and may
# reduce frustration if the paths are wrong by doing it first.

objects = tex-file.lo absolute.lo atou.lo cnf.lo concat.lo concat3.lo \
concatn.lo db.lo debug.lo dir.lo elt-dirs.lo expand.lo extend-fname.lo \
file-p.lo find-suffix.lo fn.lo fontmap.lo getopt.lo getopt1.lo hash.lo \
kdefault.lo line.lo magstep.lo make-suffix.lo path-elt.lo \
pathsearch.lo proginit.lo progname.lo readable.lo rm-suffix.lo \
str-list.lo str-llist.lo tex-glyph.lo tex-hush.lo tex-make.lo \
tilde.lo truncate.lo uppercasify.lo variable.lo version.lo xcalloc.lo \
xfopen.lo xfseek.lo xftell.lo xgetcwd.lo xmalloc.lo xopendir.lo xputenv.lo \
xrealloc.lo xstat.lo xstrdup.lo $(liblobjs) $(malloc)
liblobjs =  basename.lo strcasecmp.lo

library = kpathsea

# We want to compile almost everything with libtool ...
KPATHSEA_CC = $(LIBTOOL) compile "$(CC)"
.SUFFIXES: .lo
.c.lo:
	$(KPATHSEA_CC) $< $(ALL_CFLAGS)

default all: kpsewhich kpsestat access readlink $(library).info manfiles

kpsewhich: $(kpathsea) kpsewhich.o
	$(kpathsea_link) kpsewhich.o $(LOADLIBES)

kpsestat: kpsestat.o
	$(link_command) kpsestat.o

access: access.o
	$(link_command) access.o

readlink: readlink.o
	$(link_command) readlink.o

$(kpathsea): $(objects) klibtool.version
	$(LIBTOOL) archive $(AR) $(ARFLAGS) $@ $(objects)

# All the objects depend on the klibtool configuration file.
$(objects): ../klibtool.config

# Create klibtool.version in the build tree
klibtool.version:
	$(LN) $(srcdir)/kpathsea.version $@

# Make variable substitutions for paths.h.
texmf.cnf: texmf.in texmf.sed
	sed -f texmf.sed $(srcdir)/texmf.in >$@

# The idea is to turn each var=value into s%@var@%value%g. Seems simpler
# to put the substitutions in a file than to play shell quoting games.
texmf.sed: Makefile
	rm -f texmf.sed
	echo $(makevars) \
	| tr ' ' '\012' \
	| sed -e 's/^/s%@/' -e 's/=/@%/' -e 's/$$/%/' -e 's/$$/g/' \
	>$@
# Insert $TEXMF in as many of the hardwired paths as possible.  We could
# use the slightly different rule 's%$(texmf)/%\$$TEXMF/%g' and avoid
# the need for the corrections below.  But if texmf.in is edited
# the heuristic may no longer work.
	echo 's%$(texmf)%\$$TEXMF%g' >>$@
# Now we repair the damage this may have caused.  Don't replace the rhs of
# the TEXMFMAIN assignment itself, then we'd end up with TEXMFMAIN = $TEXMF
# and TEXMF = $TEXMFMAIN.  The (commented) assignment of TEXMFLOCAL is
# likely to have been mangled as well.
	echo '/^ *TEXMFMAIN[ =]/s%\$$TEXMF%$(texmf)%' >>$@
	echo '/^[% ]*TEXMFLOCAL[ =]/s%\$$TEXMF%$(texmf)%' >>$@
# And fill in the last clause of TEXMFCNF with an absolute path.
	echo '/^ *TEXMFCNF[ =]/s%@web2c@%$(web2cdir)%' >>$@

# First null out comments and leading/trailing whitespace, then remove
# lines that define invalid C identifiers, then remove blank lines and
# lines that define lowercase values (those are never path values). Each
# line remaining looks like
# <name> = <value>
# (but = and surrounding spaces are optional, hence can't remove all spaces).
# Change this to #ifndef DEFAULT_name@#define DEFAULT_name "value"@#endif,
# then change the @'s to newlines (sed isn't good at multiline replacements).
# 
# No backslash-newline escapes in the long sed replacement because that
# will turn into a space in the output.
# 
# Without the $TEXMF/prefix/etc. substitutions, if the cnf file was
# not found, the compile-time paths would be of little use, since TEXMF
# (etc.) wouldn't be defined. Alternatively, we could have a way to
# specify compile-time default values for variables in general, but I
# think it's better to keep the last-resort paths as simple as possible.
# 
# The definition of DEFAULT_TEXMF (and other variables)
# that winds up in the final paths.h will not be used.
#
# We don't want to rewrite paths.h when we have only changed comments
# in texmf.in that have no effect on paths.h, since that would cause
# almost everything to be rebuilt.
$(kpathsea_dir)/paths.h: stamp-paths
stamp-paths: texmf.cnf
	echo "/* paths.h: Generated from texmf.cnf. */" >paths.tmp
	sed -e 's/%.*//' -e 's/^[ 	]*//' -e 's/[ 	]*$$//' texmf.cnf \
	| grep '^[ 	]*[A-Z0-9_]*[ 	=]' \
	| sed '/^$$/d' \
	| sed 's/^\([^ 	=]*\)[ 	]*=*[ 	]*\(.*\)/#ifndef DEFAULT_\1@#define DEFAULT_\1 "\2"@#endif/' \
	| tr '@' '\012' \
	| sed -e 's%\$$TEXMFMAIN%$(texmf)%g' \
	      -e 's%\$$TEXMF%$(texmf)%g' \
	      -e 's%\$$VARTEXFONTS%$(vartexfonts)%g' \
	      -e 's%\$$web2cdir%$(web2cdir)%g' \
	      -e 's%\$$prefix%$(prefix)%g' \
	>>paths.tmp
	@if cmp -s paths.h paths.tmp 2>/dev/null; then \
	  echo "paths.h is unchanged"; \
	else \
	  echo "cp paths.tmp paths.h"; \
	  cp paths.tmp paths.h; \
	fi
	rm -f paths.tmp
	date >stamp-paths

# Need an extra definition for this. Dependencies included below.
tex-file.o:
	$(KPATHSEA_CC) -DDEFAULT_FONT_SIZES='\"$(default_texsizes)\"' $(srcdir)/tex-file.c

check: kpsewhich
	./kpsewhich -expand-var '$$TEXMF'

install: install-exec install-data
uninstall: uninstall-exec uninstall-data

install-exec: kpsewhich
	$(SHELL) $(top_srcdir)/../mkinstalldirs $(bindir) $(scriptdir) \
	  $(libdir) $(web2cdir)
	if grep 'original mktexpk --' $(scriptdir)/mktexpk >/dev/null 2>&1\
	   || test ! -r $(scriptdir)/mktexpk; then \
	  $(INSTALL_SCRIPT) $(srcdir)/mktexpk $(scriptdir)/mktexpk; \
	else true; fi
	if grep 'original mktexmf --' $(scriptdir)/mktexmf >/dev/null 2>&1\
	   || test ! -r $(scriptdir)/mktexmf; then \
	  $(INSTALL_SCRIPT) $(srcdir)/mktexmf $(scriptdir)/mktexmf; \
	else true; fi
	if grep 'original mktextfm --' $(scriptdir)/mktextfm >/dev/null 2>&1\
	   || test ! -r $(scriptdir)/mktextfm; then \
	  $(INSTALL_SCRIPT) $(srcdir)/mktextfm $(scriptdir)/mktextfm; \
	else true; fi
	if grep 'original mktexlsr --' $(scriptdir)/mktexlsr >/dev/null 2>&1\
	   || test ! -r $(scriptdir)/mktexlsr; then \
	  $(INSTALL_SCRIPT) $(srcdir)/mktexlsr $(scriptdir)/mktexlsr; \
	else true; fi
	$(INSTALL_SCRIPT) $(srcdir)/mktex.opt $(web2cdir)/mktex.opt
	$(INSTALL_SCRIPT) $(srcdir)/mktexdir $(web2cdir)/mktexdir
	$(INSTALL_SCRIPT) $(srcdir)/mktexdir.opt $(web2cdir)/mktexdir.opt
	$(INSTALL_SCRIPT) $(srcdir)/mktexnam $(web2cdir)/mktexnam
	$(INSTALL_SCRIPT) $(srcdir)/mktexnam.opt $(web2cdir)/mktexnam.opt
	$(INSTALL_SCRIPT) $(srcdir)/mktexupd $(web2cdir)/mktexupd
	$(INSTALL_LIBTOOL_LIBS) $(libdir) lib$(library).la
	$(INSTALL_LIBTOOL_PROG) $(bindir) kpsewhich
	$(INSTALL_PROGRAM) kpsestat $(bindir)
	$(INSTALL_PROGRAM) access $(bindir)
	$(INSTALL_PROGRAM) readlink $(bindir)
uninstall-exec:
	rm -f $(bindir)/kpsewhich

install-data: texmf.cnf kpathsea.info install-man
	$(SHELL) $(top_srcdir)/../mkinstalldirs $(texmf) $(infodir) $(web2cdir)
	if grep 'original texmf.cnf --' $(web2cdir)/texmf.cnf >/dev/null 2>&1 \
	   || test ! -r $(web2cdir)/texmf.cnf; then \
	  $(INSTALL_DATA) texmf.cnf $(web2cdir)/texmf.cnf; \
	else true; fi
	test -r kpathsea.info || cd $(srcdir) && for i in kpathsea.i*; do \
	  $(INSTALL_DATA) $$i $(infodir)/$$i; done
# Should we install the headers?  They are not just system-dependent,
# which is bad enough, but even compiler-dependent.
	$(SHELL) $(top_srcdir)/../mkinstalldirs $(includedir)/kpathsea
	for f in $(install_headers); do \
	  $(INSTALL_DATA) $$f $(includedir)/kpathsea/; done
	rm -f $(includedir)/kpathsea/acconfig.h
	$(POSTINSTALL)
	test -r $(infodir)/dir || $(INSTALL_DATA) $(top_srcdir)/../dir $(infodir)
	if $(SHELL) -c 'install-info --version' >/dev/null 2>&1; then \
	  install-info --info-dir=$(infodir) $(infodir)/kpathsea.info; \
	else true; fi

uninstall-data:
	rm -f $(infodir)/kpathsea.i*
#	for f in $(install_headers); do rm -f $(includedir)/`basename $$f`; done

# distdir is used by other distributions; they define $(distdir).
distdir: doc $(library).dvi $(library).h
	rm -rf $(distdir)
	mkdir -p $(distdir)
	ln $(ln_files) kpathsea.version $(distdir)
	touch *.info*
	ln mktex* *.info* *.texi $(library).aux $(library).cps $(distdir)
	rm -f $(distdir)/*.1 $(distdir)/*.man
	ln *.man $(distdir)
	ln BUGS README.CONFIGURE HIER PROJECTS *.ac $(distdir)
	touch kpathsea.h # to avoid .cps.h rule on Solaris
	cd $(distdir) && rm -f paths.h

# I don't use this, but other programmers want it.  acconfig.h is an
# autoheader input file, not an includable C header. Bad name.
$(library).h: always
	rm -f $@
	echo '#include <kpathsea/config.h>' >$@
	ls -1 *.h \
	  | grep -v '\(acconfig\|config\|kpathsea\|win32lib\|c-auto\)\.h' \
	  | sed -e 's,^,#include <kpathsea/,' -e s',$$,>,' >>$@
	touch -r `ls -1t *.h | tail +2 | head -1` $@ 
always:
.PHONY: always

# config.make -- autoconf rules to remake the Makefile, c-auto.h, etc.
#M#ac_dir = $(top_srcdir)/../etc/autoconf
#M#autoconf = $(ac_dir)/acspecific.m4 $(ac_dir)/acgeneral.m4 $(ac_dir)/acsite.m4
#M#autoheader = $(ac_dir)/acconfig.h $(ac_dir)/autoheader.m4
#M#
#M## I define $(autoconf) to acgeneral.m4 and the other Autoconf files, so
#M## configure automatically gets remade in the sources with a new Autoconf
#M## release.  But it would be bad for installers with Autoconf to remake
#M## configure (not to mention require Autoconf), so I take out the variable
#M## $(autoconf) definition before release.
#M## 
#M## BTW, xt.ac isn't really required for dvipsk or dviljk, but it doesn't
#M## seem worth the trouble.
#M## 
#M#configure_in = $(srcdir)/configure.in $(kpathsea_srcdir)/common.ac \
#M#  $(kpathsea_srcdir)/withenable.ac $(kpathsea_srcdir)/xt.ac \
#M#  $(kpathsea_srcdir_parent)/acklibtool.m4
#M#$(srcdir)/configure: $(configure_in) $(autoconf)
#M#	cd $(srcdir) && autoconf -m $(ac_dir)
#M#
#M## And make sure that --enable-maintainer-mode is used if configure is
#M## called from the makefiles.  We do not try to do this through the
#M## cache, lest the option sneaks into a global cache file.
#M#enablemaintflag = --enable-maintainer-mode

config.status: $(srcdir)/configure
	$(SHELL) $(srcdir)/configure --no-create $(enablemaintflag)

Makefile: $(srcdir)/Makefile.in config.status $(top_srcdir)/../make/*.make
	$(SHELL) config.status

# This rule isn't used for the top-level Makefile, but it doesn't hurt.
# We don't depend on config.status because configure always rewrites
# config.status, even when it doesn't change. Thus it might be newer
# than c-auto.h when we don't need to remake the latter.
c-auto.h: stamp-auto
stamp-auto: $(srcdir)/c-auto.in
	$(SHELL) config.status
	date >stamp-auto

#M## autoheader reads acconfig.h (and c-auto.h.top) automatically.
#M#$(srcdir)/c-auto.in: $(srcdir)/stamp-auto.in
#M#$(srcdir)/stamp-auto.in: $(configure_in) $(autoheader) \
#M#  $(kpathsea_srcdir)/acconfig.h
#M#	cd $(srcdir) && autoheader -m $(ac_dir) -l $(kpathsea_srcdir)
#M#	date >$(srcdir)/stamp-auto.in

# End of config.make.

info: $(library).info
dvi: $(library).dvi

$(library).info: bugs.texi hier.texi install.texi unixtex.texi

#M#INSTALL: install.texi
#M#	$(MAKEINFO) $(one_info) $(MAKEINFO_FLAGS) $< -o $@
#M#	$(top_srcdir)/../add-info-toc $@
#M#BUGS: bugs.texi
#M#	$(MAKEINFO) $(one_info) $(MAKEINFO_FLAGS) $< -o $@
#M#	$(top_srcdir)/../add-info-toc $@
#M#HIER: hier.texi
#M#	$(MAKEINFO) $(one_info) $(MAKEINFO_FLAGS) $< -o $@
#M#unixtex.ftp: unixtex.texi
#M#	$(MAKEINFO) $(one_info) $(MAKEINFO_FLAGS) $< -o $@
#M#doc: info HIER BUGS INSTALL unixtex.ftp

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
	rm -f kpsewhich kpsestat access readlink *.lo klibtool.version
	rm -rf PROF PROF_SHARED SHARED STATIC
distclean::
	rm -f paths.h texmf.cnf texmf.sed stamp-paths

# The manual pages
manfiles = access.1 kpsestat.1 kpsewhich.1 readlink.1 \
           mktexlsr.1 mktexmf.1 mktextfm.1 mktexpk.1

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
absolute.lo: absolute.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/absolute.h $(kpathsea_srcdir)/c-pathch.h \
 $(kpathsea_srcdir)/c-ctype.h 
access.o: access.c \
 $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
atou.lo: atou.c config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
basename.lo: basename.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
cnf.lo: cnf.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/cnf.h $(kpathsea_srcdir)/db.h $(kpathsea_srcdir)/str-list.h \
 $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/line.h $(kpathsea_dir)/paths.h \
 $(kpathsea_srcdir)/pathsearch.h $(kpathsea_srcdir)/str-llist.h \
 $(kpathsea_srcdir)/tex-file.h $(kpathsea_srcdir)/variable.h
concat.lo: concat.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
concat3.lo: concat3.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
concatn.lo: concatn.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/concatn.h $(kpathsea_srcdir)/c-vararg.h
db.lo: db.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/absolute.h $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/c-pathch.h \
 $(kpathsea_srcdir)/c-ctype.h $(kpathsea_srcdir)/db.h \
 $(kpathsea_srcdir)/str-list.h $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/line.h \
 $(kpathsea_srcdir)/pathsearch.h $(kpathsea_srcdir)/str-llist.h \
 $(kpathsea_srcdir)/readable.h $(kpathsea_srcdir)/tex-file.h $(kpathsea_srcdir)/variable.h
debug.lo: debug.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
dir.lo: dir.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-dir.h \
 $(kpathsea_srcdir)/c-stat.h \
 $(kpathsea_srcdir)/hash.h
elt-dirs.lo: elt-dirs.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/expand.h $(kpathsea_srcdir)/fn.h \
 $(kpathsea_srcdir)/pathsearch.h $(kpathsea_srcdir)/str-llist.h \
 $(kpathsea_srcdir)/xopendir.h $(kpathsea_srcdir)/c-dir.h 
expand.lo: expand.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/expand.h $(kpathsea_srcdir)/pathsearch.h \
 $(kpathsea_srcdir)/str-llist.h $(kpathsea_srcdir)/tilde.h $(kpathsea_srcdir)/variable.h \
 $(kpathsea_srcdir)/concatn.h $(kpathsea_srcdir)/c-vararg.h $(kpathsea_srcdir)/absolute.h
extend-fname.lo: extend-fname.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
file-p.lo: file-p.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/xstat.h $(kpathsea_srcdir)/c-stat.h 
find-suffix.lo: find-suffix.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h 
fn.lo: fn.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/fn.h
fontmap.lo: fontmap.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/fontmap.h $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/line.h \
 $(kpathsea_srcdir)/pathsearch.h $(kpathsea_srcdir)/str-llist.h \
 $(kpathsea_srcdir)/str-list.h $(kpathsea_srcdir)/tex-file.h
getopt.lo: getopt.c config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h 
getopt1.lo: getopt1.c config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h getopt.h 
hash.lo: hash.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/str-list.h
kdefault.lo: kdefault.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/default.h
kpsestat.o: kpsestat.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-stat.h 
kpsewhich.o: kpsewhich.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/expand.h $(kpathsea_srcdir)/getopt.h \
 $(kpathsea_srcdir)/line.h $(kpathsea_srcdir)/pathsearch.h $(kpathsea_srcdir)/str-llist.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h $(kpathsea_srcdir)/tex-glyph.h \
 $(kpathsea_srcdir)/variable.h
line.lo: line.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/line.h
magstep.lo: magstep.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/magstep.h
make-suffix.lo: make-suffix.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h 
path-elt.lo: path-elt.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/pathsearch.h $(kpathsea_srcdir)/str-llist.h
pathsearch.lo: pathsearch.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/absolute.h $(kpathsea_srcdir)/expand.h \
 $(kpathsea_srcdir)/db.h $(kpathsea_srcdir)/str-list.h $(kpathsea_srcdir)/pathsearch.h \
 $(kpathsea_srcdir)/str-llist.h $(kpathsea_srcdir)/readable.h $(kpathsea_srcdir)/variable.h 
proginit.lo: proginit.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/proginit.h $(kpathsea_srcdir)/tex-file.h
progname.lo: progname.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/absolute.h $(kpathsea_srcdir)/c-pathch.h \
 $(kpathsea_srcdir)/c-ctype.h $(kpathsea_srcdir)/c-stat.h \
 $(kpathsea_srcdir)/pathsearch.h $(kpathsea_srcdir)/str-llist.h \
 $(kpathsea_srcdir)/tex-file.h
putenv.lo: putenv.c
readable.lo: readable.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-stat.h \
 $(kpathsea_srcdir)/readable.h $(kpathsea_srcdir)/tex-hush.h \
 $(kpathsea_srcdir)/truncate.h
readlink.o: readlink.c \
 $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-pathmx.h $(kpathsea_srcdir)/c-stat.h 
rm-suffix.lo: rm-suffix.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
str-list.lo: str-list.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/str-list.h
str-llist.lo: str-llist.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/str-llist.h
strcasecmp.lo: strcasecmp.c config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h 
strstr.lo: strstr.c
strtol.lo: strtol.c config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h 
tex-file.lo: tex-file.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/c-vararg.h $(kpathsea_srcdir)/cnf.h \
 $(kpathsea_srcdir)/concatn.h $(kpathsea_srcdir)/default.h $(kpathsea_srcdir)/expand.h \
 $(kpathsea_srcdir)/fontmap.h $(kpathsea_srcdir)/hash.h $(kpathsea_dir)/paths.h \
 $(kpathsea_srcdir)/pathsearch.h $(kpathsea_srcdir)/str-llist.h \
 $(kpathsea_srcdir)/tex-file.h $(kpathsea_srcdir)/tex-make.h $(kpathsea_srcdir)/variable.h
tex-glyph.lo: tex-glyph.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/absolute.h $(kpathsea_srcdir)/expand.h \
 $(kpathsea_srcdir)/fontmap.h $(kpathsea_srcdir)/hash.h $(kpathsea_srcdir)/pathsearch.h \
 $(kpathsea_srcdir)/str-llist.h $(kpathsea_srcdir)/tex-glyph.h \
 $(kpathsea_srcdir)/tex-file.h $(kpathsea_srcdir)/tex-make.h $(kpathsea_srcdir)/variable.h
tex-hush.lo: tex-hush.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/pathsearch.h \
 $(kpathsea_srcdir)/str-llist.h $(kpathsea_srcdir)/tex-hush.h $(kpathsea_srcdir)/variable.h
tex-make.lo: tex-make.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-fopen.h \
 $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/concatn.h $(kpathsea_srcdir)/c-vararg.h \
 $(kpathsea_srcdir)/db.h $(kpathsea_srcdir)/str-list.h $(kpathsea_srcdir)/fn.h \
 $(kpathsea_srcdir)/magstep.h $(kpathsea_srcdir)/readable.h $(kpathsea_srcdir)/tex-make.h \
 $(kpathsea_srcdir)/tex-file.h $(kpathsea_srcdir)/variable.h
tilde.lo: tilde.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-pathch.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/tilde.h 
truncate.lo: truncate.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-namemx.h $(kpathsea_srcdir)/c-pathch.h \
 $(kpathsea_srcdir)/c-ctype.h $(kpathsea_srcdir)/c-pathmx.h \
 $(kpathsea_srcdir)/truncate.h
uppercasify.lo: uppercasify.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-ctype.h 
variable.lo: variable.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-ctype.h \
 $(kpathsea_srcdir)/cnf.h $(kpathsea_srcdir)/fn.h $(kpathsea_srcdir)/variable.h
version.lo: version.c c-auto.h
win32lib.lo: win32lib.c
xcalloc.lo: xcalloc.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
xfopen.lo: xfopen.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
xfseek.lo: xfseek.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
xftell.lo: xftell.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
xgetcwd.lo: xgetcwd.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/c-pathmx.h
xmalloc.lo: xmalloc.c \
 $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
xopendir.lo: xopendir.c $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/xopendir.h $(kpathsea_srcdir)/c-dir.h 
xputenv.lo: xputenv.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
xrealloc.lo: xrealloc.c \
 $(kpathsea_srcdir)/config.h c-auto.h \
 $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h \
 $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
xstat.lo: xstat.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/xstat.h $(kpathsea_srcdir)/c-stat.h 
xstrdup.lo: xstrdup.c $(kpathsea_srcdir)/config.h c-auto.h $(kpathsea_srcdir)/c-std.h \
 $(kpathsea_srcdir)/c-unistd.h \
 $(kpathsea_srcdir)/systypes.h \
 $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
 $(kpathsea_srcdir)/c-minmax.h \
 $(kpathsea_srcdir)/c-limits.h \
 $(kpathsea_srcdir)/c-proto.h \
 $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
 $(kpathsea_srcdir)/progname.h
