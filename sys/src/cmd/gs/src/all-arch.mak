#    Copyright (C) 1999, 2000 Aladdin Enterprises.  All rights reserved.
# 
# This software is provided AS-IS with no warranty, either express or
# implied.
# 
# This software is distributed under license and may not be copied,
# modified or distributed except as expressly authorized under the terms
# of the license contained in the file LICENSE in this distribution.
# 
# For more information about licensing, please refer to
# http://www.ghostscript.com/licensing/. For information on
# commercial licensing, go to http://www.artifex.com/licensing/ or
# contact Artifex Software, Inc., 101 Lucas Valley Road #110,
# San Rafael, CA  94903, U.S.A., +1(415)492-9861.

# $Id: all-arch.mak,v 1.16 2004/12/10 23:48:48 giles Exp $
#
# Author:
# 	Nelson H. F. Beebe
# 	Center for Scientific Computing
# 	University of Utah
# 	Department of Mathematics, 322 INSCC
# 	155 S 1400 E RM 233
# 	Salt Lake City, UT 84112-0090
# 	USA
# 	Email: beebe@math.utah.edu, beebe@acm.org, beebe@ieee.org (Internet)
# 	WWW URL: http://www.math.utah.edu/~beebe
# 	Telephone: +1 801 581 5254
# 	FAX: +1 801 585 1640, +1 801 581 4148
#
# /usr/local/src/ghostscript/gs6.0/Makefile, Sat Feb 12 09:46:16 2000
# Edit by Nelson H. F. Beebe <beebe@math.utah.edu>
# Update with settings of STDLIBS for several targets, because gs-6.0
# added a reference to the POSIX threads library, which is not
# universally available.
# /usr/local/src/ghostscript/gs5.94/Makefile, Sun Oct  3 08:07:02 1999
# Edit by Nelson H. F. Beebe <beebe@math.utah.edu>
# Major update with rearrangement of target names, and addition of
# -L/usr/local/lib everywhere.
#=======================================================================
# This Makefile is an interface to the UNMODIFIED unix*.mak files for
# building gs, so as to avoid the need for customizing Makefiles for
# multiple architectures with each new release of ghostscript.
#
# Usage:
#	make <arch-name> TARGETS='...'
#
# or, for convenience at Utah, 
#
#	make `hostname`
#
# HINT: for parallel GNU make runs, add -jnnn to TARGETS, e.g.
#       TARGETS=-j6.
# WARNING: this does not produce successful builds on at least SGI IRIX 6.
#
# Current target list:
#	all
#	clean
#	mostlyclean
#	clobber
#	distclean
#	maintainer-clean
#	init
#	install
#	install-no-X11
#	install-gnu-readline
#	install-binary
#	install-binary-gnu-readline
#	install-fontmap
#	install-pdfsec
#	apple-powermac-rhapsody5.5
#	apple-powerpc-rhapsody5.5
#	dec-alpha-osf
#	dec-alpha-osf-gnu-readline
#	dec-mips-ultrix
#	hp-parisc-hpux
#	hp-parisc-hpux-gnu-readline
#	ibm-rs6000-aix
#	ibm-rs6000-aix-c89
#	ibm-rs6000-aix-4-1-c89
#	ibm-rs6000-aix-3-2-5-gcc
#	ibm-rs6000-aix-gcc
#	ibm-rs6000-aix-4.2
#	ibm-rs6000-aix-4.2-gnu-readline
#	ibm-rs6000-aix-4.3
#	ibm-rs6000-aix-4.3-64bit
#	linux
#	linux-gnu-readline
#	next-m68K-mach
#	next-m68K-mach-gnu-readline
#	next-m68K-mach-cc
#	sgi-mips-irix5
#	sgi-mips-irix5-gnu-readline
#	sgi-mips-irix6.1
#	sgi-mips-irix6.3
#	sgi-mips-irix6.3-gnu-readline
#	sgi-mips-irix6.4
#	sgi-mips-irix6.4-gcc
#	sgi-mips-irix6.4-gnu-readline
#	sgi-mips-irix6.5
#	sgi-mips-irix6.5-gnu-readline
#	sgi-mips-irix6.5-64bit
#	sun-sparc-solaris
#	sun-sparc-solaris-64bit
#	sun-sparc-solaris-gnu-readline
#	sun-sparc-solaris-gcc
#	sun-sparc-solaris-opt-gnu-readline
#	sun-sparc-solaris-newsprint
#	sun-sparc-solaris-pg
#	sun-sparc-sunos-gcc
#	sun-sparc-sunos-gcc-gnu-readline
#
# Machine-specific targets (for "make `hostname`"):
## XXXXXXXX.YYYYYYYY.utah.edu
## XXXXXXXX.YYYYYYYY.utah.edu
## XXXXXXXX.YYYYYYYY.utah.edu
## XXXXXXXX.YYYYYYYY.utah.edu
## XXXXXXXX.YYYYYYYY.utah.edu
## XXXXXXXX.YYYYYYYY.utah.edu
## XXXXXXXX.YYYYYYYY.utah.edu
## XXXXXXXX.YYYYYYYY.utah.edu
## XXXXXXXX.YYYYYYYY.utah.edu
## XXXXXXXX.YYYYYYYY.utah.edu
## XXXXXXXX.YYYYYYYY.utah.edu
## XXXXXXXX.YYYYYYYY.utah.edu
#
# [29-Apr-1999] -- update for gs-5.82: Add XLIBDIRSALL list for
#		   install target, and add hostnames as convenience
#		   targets, duplicating information in the
#		   ../BUILD-GS.sh script, but allowing easier restarts
#		   after build failures.  Update install-fontmap target
#		   to reflect new location of installed Fontmap files.
# [20-Mar-1999] -- update for gs-5.73.  Alphabetize most definitions.
#		   Add more comments. Add common macros to eliminate
#		   duplication. Set the SHARE_* variables to use
#		   installed versions of support libraries, instead of
#		   always having to duplicate their source trees as we
#		   did with older releases.
# [19-Mar-1999] -- change SGI IRIX 6.x targets to use -n32 -mips3,
#		   since -n32 is the default when no memory model
#		   is selected
# [09-Feb-1999] -- add install-pdfsec target
# [03-Nov-1998] -- update for gs-5.60 and later
# [10-Aug-1998] -- add ibm-rs6000-aix-3-2-5-gcc and
#		   ibm-rs6000-aix-4-1-c89 targets
# [04-Aug-1998] -- add linux and ibm-rs6000-aix-c89 target, and
#		   dependencies on init target so that I do not need
#		   to remember to create the obj subdirectory manually
# [19-Mar-1998] -- add -32 -mips2 flag to SGI IRIX 6.x targets, so that
#		   the executables run on every SGI that can run that
#		   O/S.  Otherwise, when building on XXXXXXXX.YYYYYYYY.utah.edu,
#		   the compiler chooses a default of -mips4, which won't
#		   run on Indy and R4400 machines.
# [23-Feb-1998] -- add -jnnn hint above, and COMMON_DEVICES below
# [28-Nov-1997]
#=======================================================================

# Definitions needed from src/*.mak files:
DD			= $(GLD)
GLD			= $(GLGENDIR)/
GLGENDIR		= ./obj
GLOBJ			= ./obj/
PSD			= $(PSGENDIR)/
PSGENDIR		= ./obj

# Definition(s) for this file:
SRCDIR			= /usr/local/src

# Define local modifications of search paths:
TF			= /usr/local/lib/tex/fonts
GS_LIB_DEFAULT		= $$(gsdatadir)/lib:$$(gsdatadir)/fonts:$$(gsdatadir)/examples:$$(gsdir)/fonts:/usr/local/share/sys/fonts/postscript:$(TF)/lucida:$(TF)/mathtime:$(TF)/postscript/bakoma/pfb:$(TF)/vf

# Define local paths for install targets:
GS_SHARE_DIR		= /usr/local/share/ghostscript
GS_SRC_DIR		= $(SRCDIR)/ghostscript

# Arguments for make with cc (or other), and gcc:
ARGS			= -f src/unixansi.mak $(COMMON_ARGS)

ARGSGCC			= -f src/unix-gcc.mak $(COMMON_ARGS)

COMMON_ARGS		= DEVICE_DEVS_EXTRA='$(DEVICE_DEVS_EXTRA)' \
			  GS_LIB_DEFAULT='$(GS_LIB_DEFAULT)' \
			  JSRCDIR='$(JSRCDIR)' \
			  PNGSRCDIR='$(PNGSRCDIR)' \
			  PSRCDIR='$(PNGSRCDIR)' \
			  PVERSION=10208 \
			  SHARE_LIBPNG='$(SHARE_LIBPNG)' \
			  SHARE_ZLIB='$(SHARE_ZLIB)' \
			  XCFLAGS='$(XCFLAGS)' \
			  ZSRCDIR='$(ZSRCDIR)' \
			  $(TARGETS)

# Additional gcc-specific compilation flags
GCFLAGS			=

# Name of the installed binary executable (it will also be called gs-x.yy):
GS			= ngs
GS			= gs

# Here is a list of additional output devices that we need to support at
# the University of Utah Math, Physics, and INSCC installations;
# effective with gs-5.60, Each must have a $(DD) prefix:
DEVICE_DEVS_EXTRA	= $(DD)st800.dev $(DD)stcolor.dev

# [20-Mar-1999] Set FEATURE_DEVS_EXTRA to include gnrdline.dev, to
# support input line editing in gs when compiled with gcc.  Addition of
# this module also requires adding EXTRALIBS='-ltermcap' for each system
# below that uses gcc for the build.
FEATURE_DEVS_EXTRA	= $(PSD)gnrdline.dev

GNU_READLINE_ARGS	= EXTRALIBS='-ltermcap' \
			  FEATURE_DEVS_EXTRA='$(FEATURE_DEVS_EXTRA)' \
			  XCFLAGS='-I. -I$(JSRCDIR)'

# Additions to CFLAGS for all compilers
XCFLAGS			= -I/usr/local/include

# This variable contains a list of all X library locations, for
# use in the install target
XLIBDIRSALL=' \
		-L/usr/X11R6/lib \
		-L/usr/lib/X11 \
		-L/usr/lib/X11R5 \
		-L/usr/openwin/lib \
		-L/usr/lpp/X11/lib/R5 \
		-L/usr/lpp/X11/lib \
		-L/usr/local/$(SGIARCHLIB) \
		-L/usr/local/X11R5/lib \
		-L/usr/local/lib \
		-L/usr/local/lib32 \
'

# Compilation flags and load library for SGI IRIX 6.x builds:
SGIARCHFLAGS		= -n32 -mips3
SGIARCHLIB		= libn32

SGIARCH64FLAGS		= -64 -mips3
SGIARCH64LIB		= lib64

# [20-Mar-1999]: New from gs-5.73: use png and zlib libraries already
# installed on the system.

SHARE_LIBPNG		= 1
SHARE_ZLIB		= 1

# These are our standard paths to the library source trees
JSRCDIR			= $(SRCDIR)/jpeg
PNGSRCDIR		= $(SRCDIR)/libpng
ZSRCDIR			= $(SRCDIR)/zlib

# Use this to provide alternate targets to make, instead of the default
# all.  It can also be used to pass additional arguments to child makes,
# e.g., -j12 for 12 parallel jobs with GNU make.
TARGETS			=

#=======================================================================

BINDIR			= /usr/local/bin

CHMOD			= chmod

CP			= /bin/cp -p
CP			= rcp -p

MV			= /bin/mv

RM			= /bin/rm -f

SHELL			= /bin/sh

#=======================================================================

all:
	$(MAKE) $(ARGS)

# Convenience targets to make standard targets available
clean mostlyclean clobber distclean maintainer-clean:
	$(MAKE) $(ARGS) $@

init:
	-if test ! -d obj ; then mkdir obj ; fi

install:	install-binary install-fontmap install-pdfsec

install-no-X11:
	$(MAKE) install \
		FEATURE_DEVS_EXTRA= \
		DEVICE_DEVS= \
		SYNC=nosync \
		STDLIBS= \
		XLIBDIRS= \
		XLIBDIRSALL= \
		EXTRALIBS=

install-gnu-readline:	install-binary-gnu-readline install-fontmap install-pdfsec

# Remove the old gs binary first, so as to preserve the previous
# gs-x.yy version, if any.
install-binary:
	-$(RM) $(BINDIR)/$(GS)
	@$(MAKE) $(ARGS) install GS=$(GS) XLIBDIRS=$(XLIBDIRSALL) ; \
	d=`pwd` ; \
	d=`basename $$d` ; \
	d=`echo $$d | sed -e s/gs/gs-/` ; \
	$(RM) $(BINDIR)/$$d ; \
	ln $(BINDIR)/$(GS) $(BINDIR)/$$d ; \
	ls -l $(BINDIR)/$(GS) $(BINDIR)/$$d

install-binary-gnu-readline:
	-$(RM) $(BINDIR)/$(GS)
	@$(MAKE) $(ARGS) install GS=$(GS)  XLIBDIRS=$(XLIBDIRSALL) $(GNU_READLINE_ARGS) ; \
	d=`pwd` ; \
	d=`basename $$d` ; \
	d=`echo $$d | sed -e s/gs/gs-/` ; \
	$(RM) $(BINDIR)/$$d ; \
	ln $(BINDIR)/$(GS) $(BINDIR)/$$d ; \
	ls -l $(BINDIR)/$(GS) $(BINDIR)/$$d

install-fontmap:
	@d=`pwd` ; \
	d=`basename $$d` ; \
	d=`echo $$d | sed -e s/gs//` ; \
	if test -f $(GS_SRC_DIR)/Fontmap.new ; \
	then \
		if test -f $(GS_SHARE_DIR)/$$d/lib/Fontmap.org ; \
		then \
			true ; \
		else \
			mv $(GS_SHARE_DIR)/$$d/lib/Fontmap $(GS_SHARE_DIR)/$$d/lib/Fontmap.org ; \
		fi ; \
		$(CP) $(GS_SRC_DIR)/Fontmap.new $(GS_SHARE_DIR)/$$d/lib/Fontmap ; \
		ls -l $(GS_SHARE_DIR)/$$d/lib/Fontmap* ; \
	fi

install-pdfsec:
	@d=`pwd` ; \
	d=`basename $$d` ; \
	d=`echo $$d | sed -e s/gs//` ; \
	if test -f $(GS_SRC_DIR)/lib/pdf_sec.ps ; \
	then \
		$(MV) $(GS_SHARE_DIR)/$$d/lib/pdf_sec.ps $(GS_SHARE_DIR)/$$d/lib/pdf_sec.ps.org ; \
		$(CP) lib/pdf_sec.ps $(GS_SHARE_DIR)/$$d/lib/pdf_sec.ps ; \
		$(CHMOD) 664 $(GS_SHARE_DIR)/$$d/lib/pdf_sec.ps ; \
	fi

#=======================================================================
# Architecture-specific targets:
#
# NB: gcc 2.7.x produces bad code in zfont2.c:zregisterencoding(), and possibly
# elsewhere, so we must use native compilers for now.

# Apple Macintosh PowerPC running Rhapsody 5.5 (a NeXTStep 5 derivative,
# with no X Window System support):
apple-powermac-rhapsody5.5 apple-powerpc-rhapsody5.5:
	$(MAKE) $(ARGSGCC) \
		CC='gcc' \
		GCFLAGS=$(GCFLAGS) \
		FEATURE_DEVS_EXTRA= \
		DEVICE_DEVS= \
		SYNC=nosync \
		STDLIBS= \
		XLIBDIRS= \
		XLIBDIRSALL= \
		EXTRALIBS=
	@echo "#################################################################"
	@echo "# To install this program, in the top-level build directory, do #"
	@echo "#         make install-no-X11                                   #"
	@echo "#################################################################"

dec-alpha-osf:	init
	$(MAKE) $(ARGS) \
		CC='c89 -O4 -Olimit 1500' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/lib -L/usr/lib/X11'

dec-alpha-osf-gnu-readline:	init
	$(MAKE) $(ARGS) \
		CC='c89 -O4 -Olimit 1500' \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/lib -L/usr/lib/X11' \
		$(GNU_READLINE_ARGS)

# NB: Need -Dconst= for gcc 2.7.2 (unless gcc patch in make.doc is installed)
dec-mips-ultrix:	init
	$(MAKE) $(ARGSGCC) \
		CC='gcc -Dconst= -O3' \
		GCFLAGS=$(GCFLAGS) \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/lib -L/usr/lib/X11' \
		EXTRALIBS='-ltermcap'

hp-parisc-hpux:	init
	$(MAKE) $(ARGS) \
		CC='c89 -O -D_HPUX_SOURCE +Onolimit' \
		FEATURE_DEVS_EXTRA= \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/include/X11R5 \
		XLIBDIRS='-L/usr/lib/X11R5 -L/usr/local/lib' \
		$(GLOBJ)gdevupd.o $(GLOBJ)gxclread.o

	$(MAKE) $(ARGS) \
		CC='c89 -O -D_HPUX_SOURCE' \
		FEATURE_DEVS_EXTRA= \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/include/X11R5 \
		XLIBDIRS='-L/usr/lib/X11R5 -L/usr/local/lib'

hp-parisc-hpux-gnu-readline:	init
	$(MAKE) $(ARGS) \
		CC='c89 -O -D_HPUX_SOURCE +Onolimit' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/include/X11R5 \
		XLIBDIRS='-L/usr/local/lib -L/usr/lib/X11R5 -L/usr/local/lib' \
		$(GLOBJ)gdevupd.o $(GLOBJ)gxclread.o

	$(MAKE) $(ARGS) \
		CC='c89 -O -D_HPUX_SOURCE' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/include/X11R5 \
		XLIBDIRS='-L/usr/local/lib -L/usr/lib/X11R5 -L/usr/local/lib' \
		$(GNU_READLINE_ARGS)

# NB: gs3.68 executable core dumps with this compiler
ibm-rs6000-aix:	init
	$(MAKE) $(ARGS) \
		CC='cc -O -D_POSIX_SOURCE' \
		CP='cp -p' \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/lpp/X11/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/lpp/X11/lib/R5 -L/usr/lpp/X11/lib'

# This works on our local AIX 3.2.5 systems: additional header files
# and libraries are needed, because IBM does not supply the Athena
# widgets in /usr/lpp/X11.
ibm-rs6000-aix-c89:	init
	$(MAKE) $(ARGS) \
		CC='c89 -O -D_POSIX_SOURCE' \
		CP='cp -p' \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE='-I/usr/lpp/X11/include -I/usr/local/X11R5/include' \
		XLIBDIRS='-L/usr/local/lib -L/usr/lpp/X11/lib/R5 -L/usr/lpp/X11/lib -L/usr/local/X11R5/lib'

ibm-rs6000-aix-4-1-c89:	init
	$(MAKE) $(ARGS) \
		CC='c89 -O -D_POSIX_SOURCE' \
		CP='cp -p' \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE='-I/usr/lpp/X11/include' \
		XLIBDIRS='-L/usr/local/lib -L/usr/lpp/X11/lib/R5 -L/usr/lpp/X11/lib'

# NB: Need -Dconst= for gcc 2.7.1 (unless gcc patch in make.doc is installed)
ibm-rs6000-aix-3-2-5-gcc:	init
	$(MAKE) $(ARGSGCC) \
		CC='gcc -Dconst= -O -D_POSIX_SOURCE' \
		CP='cp -p' \
		GCFLAGS=$(GCFLAGS) \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE='-I/usr/lpp/X11/include -I/usr/local/X11R5/include'\
		XLIBDIRS='-L/usr/local/lib -L/usr/lpp/X11/lib/R5 -L/usr/lpp/X11/lib -L/usr/local/X11R5/lib' \
		EXTRALIBS='-ltermcap'

# NB: Need -Dconst= for gcc 2.7.1 (unless gcc patch in make.doc is installed)
ibm-rs6000-aix-gcc:	init
	$(MAKE) $(ARGSGCC) \
		CC='gcc -Dconst= -O -D_POSIX_SOURCE' \
		CP='cp -p' \
		GCFLAGS=$(GCFLAGS) \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/lpp/X11/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/lpp/X11/lib/R5 -L/usr/lpp/X11/lib' \
		EXTRALIBS='-ltermcap'

# gp_unix.o must be compiled outside POSIX environment to make
# struct timeval and struct timezone visible
ibm-rs6000-aix-4.2:	init
	$(MAKE) $(ARGS) \
		CC='cc -O -DMAXMEM=4096' \
		CP='cp -p' \
		FEATURE_DEVS_EXTRA= \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/lpp/X11/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/lpp/X11/lib/R5 -L/usr/lpp/X11/lib' \
		$(GLOBJ)gp_unix.o

	$(MAKE) $(ARGS) \
		CC='cc -O -D_POSIX_SOURCE -DMAXMEM=4096' \
		CP='cp -p' \
		FEATURE_DEVS_EXTRA= \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/lpp/X11/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/lpp/X11/lib/R5 -L/usr/lpp/X11/lib'

ibm-rs6000-aix-4.2-64bit:	init
	$(MAKE) $(ARGS) \
		CC='cc -q64 -O -DMAXMEM=4096' \
		CP='cp -p' \
		FEATURE_DEVS_EXTRA= \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/lpp/X11/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/lpp/X11/lib/R5 -L/usr/lpp/X11/lib' \
		$(GLOBJ)gp_unix.o

	$(MAKE) $(ARGS) \
		CC='cc -q64 -O -D_POSIX_SOURCE -DMAXMEM=4096' \
		CP='cp -p' \
		FEATURE_DEVS_EXTRA= \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/lpp/X11/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/lpp/X11/lib/R5 -L/usr/lpp/X11/lib'

ibm-rs6000-aix-4.2-gnu-readline:	init
	$(MAKE) $(ARGS) \
		CC='cc -O -DMAXMEM=4096' \
		CP='cp -p' \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/lpp/X11/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/local/lib -L/usr/lpp/X11/lib/R5 -L/usr/lpp/X11/lib' \
		$(GLOBJ)gp_unix.o

	$(MAKE) $(ARGS) \
		CC='cc -O -D_POSIX_SOURCE -DMAXMEM=4096' \
		CP='cp -p' \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/lpp/X11/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/local/lib -L/usr/lpp/X11/lib/R5 -L/usr/lpp/X11/lib' \
		$(GNU_READLINE_ARGS)

ibm-rs6000-aix-4.3:	init
	$(MAKE) $(ARGS) \
		CC='cc -O -D_ALL_SOURCE -DMAXMEM=4096 -Dconst=' \
		CP='cp -p' \
		FEATURE_DEVS_EXTRA= \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/lpp/X11/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/lpp/X11/lib/R6 -L/usr/lpp/X11/lib'

ibm-rs6000-aix-4.3-64bit:	init
	$(MAKE) $(ARGS) \
		CC='cc -q64 -O -D_ALL_SOURCE -DMAXMEM=4096 -Dconst=' \
		CP='cp -p' \
		FEATURE_DEVS_EXTRA= \
		INSTALL='/usr/ucb/install -c' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/lpp/X11/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/lpp/X11/lib/R6 -L/usr/lpp/X11/lib'

linux:	init
	$(MAKE) $(ARGSGCC) \
		CC='gcc' \
		GCFLAGS=$(GCFLAGS) \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/X11R6/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/X11R6/lib' \
		EXTRALIBS='-ltermcap'

linux-gnu-readline:	init
	$(MAKE) $(ARGSGCC) \
		CC='gcc' \
		GCFLAGS=$(GCFLAGS) \
		XINCLUDE=-I/usr/X11R6/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/X11R6/lib' \
		EXTRALIBS='-ltermcap' \
		$(GNU_READLINE_ARGS)

next-m68K-mach:	init
	$(MAKE) $(ARGS) \
		CC='gcc -Dconst= -O3 -D_POSIX_SOURCE' \
		FEATURE_DEVS_EXTRA= \
		GCFLAGS=$(GCFLAGS) \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/local/X11R5/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/local/X11R5/lib' \
		INCLUDE=/usr/include/bsd \
		EXTRALIBS='-ltermcap'

next-m68K-mach-gnu-readline:	init
	$(MAKE) $(ARGS) \
		CC='gcc -Dconst= -O3 -D_POSIX_SOURCE' \
		GCFLAGS=$(GCFLAGS) \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/local/X11R5/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/local/X11R5/lib' \
		INCLUDE=/usr/include/bsd \
		$(GNU_READLINE_ARGS)

next-m68K-mach-cc:	init
	$(MAKE) $(ARGS) \
		CC='cc -Dconst= -O3 -D_POSIX_SOURCE' \
		STDLIBS=-lm \
		XINCLUDE=-I/usr/local/X11R5/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/local/X11R5/lib' \
		INCLUDE=/usr/include/bsd

# NB: Need the -Dxxx settings to get certain system types defined for
# at least gp_unifs.c and zdevcal.c
sgi-mips-irix5:	init
	$(MAKE) $(ARGS) \
		CC='cc -D_POSIX_4SOURCE -woff 608' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gdevpdf.o \
		$(GLOBJ)gdevps.o \
		$(GLOBJ)gdevtifs.o \
		$(GLOBJ)gpmisc.o \
		$(GLOBJ)gp_unix.o \
		$(GLOBJ)zdevcal.o

	$(MAKE) $(ARGS) \
		CC='cc -ansi -D_POSIX_4SOURCE -woff 608 -Olimit 1100' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gxclread.o

	$(MAKE) $(ARGS) \
		CC='cc -ansi -D_POSIX_4SOURCE -woff 608' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/lib -L/usr/lib/X11'

sgi-mips-irix5-gnu-readline:	init
	$(MAKE) $(ARGS) \
		CC='cc -D_POSIX_4SOURCE -woff 608' \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gdevpdf.o \
		$(GLOBJ)gdevps.o \
		$(GLOBJ)gdevtifs.o \
		$(GLOBJ)gpmisc.o \
		$(GLOBJ)gp_unix.o \
		$(GLOBJ)zdevcal.o

	$(MAKE) $(ARGS) \
		CC='cc -ansi -D_POSIX_4SOURCE -woff 608 -Olimit 1100' \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gxclread.o

	$(MAKE) $(ARGS) \
		CC='cc -ansi -D_POSIX_4SOURCE -woff 608' \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/lib -L/usr/lib/X11' \
		$(GNU_READLINE_ARGS)

# NB: Need the -Dxxx settings to get certain system types defined for
# at least gp_unifs.c and zdevcal.c
sgi-mips-irix6.1:	init
	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -ansi -D_POSIX_4SOURCE -woff 1185,1429' \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/lib -L/usr/lib/X11'

# 
sgi-mips-irix6.3:	init
	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -D_POSIX_4SOURCE ' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gdevpdf.o \
		$(GLOBJ)gdevps.o \
		$(GLOBJ)gdevtifs.o \
		$(GLOBJ)gpmisc.o \
		$(GLOBJ)gp_unix.o \
		$(GLOBJ)zdevcal.o

	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -ansi -D_POSIX_4SOURCE -woff 1185,1429 -Olimit 1100' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gxclread.o

	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -ansi -D_POSIX_4SOURCE -woff 1185,1429' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/$(SGIARCHLIB) -L/usr/local/lib -L/usr/lib/X11'

sgi-mips-irix6.3-gnu-readline:	init
	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -D_POSIX_4SOURCE ' \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gdevpdf.o \
		$(GLOBJ)gdevps.o \
		$(GLOBJ)gdevtifs.o \
		$(GLOBJ)gpmisc.o \
		$(GLOBJ)gp_unix.o \
		$(GLOBJ)zdevcal.o

	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -ansi -D_POSIX_4SOURCE -woff 1185,1429 -Olimit 1100' \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gxclread.o

	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -ansi -D_POSIX_4SOURCE -woff 1185,1429' \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/$(SGIARCHLIB) -L/usr/local/lib -L/usr/lib/X11' \
		$(GNU_READLINE_ARGS)


# [06-Jan-2000] Problems have been reported with SGI MIPSpro compilers
# version 7.x (x <= 3) for at least idict.o and isave.o when those
# files are compiled with optimization.  We therefore add a step to
# compile them without optimization.
sgi-mips-irix6.4:	init
	$(MAKE) $(ARGS) \
		CFLAGS_STANDARD= \
		CC='cc $(SGIARCHFLAGS) -D_POSIX_4SOURCE' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)idict.o \
		$(GLOBJ)isave.o

	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -D_POSIX_4SOURCE' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gdevpdf.o \
		$(GLOBJ)gdevps.o \
		$(GLOBJ)gdevtifs.o \
		$(GLOBJ)gpmisc.o \
		$(GLOBJ)gp_unix.o \
		$(GLOBJ)zdevcal.o
#
	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -ansi -D_POSIX_4SOURCE -woff 1185,1429 -OPT:Olimit=2500' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gxclread.o

	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -ansi -D_POSIX_4SOURCE -woff 1185,1429' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/$(SGIARCHLIB) -L/usr/local/lib -L/usr/lib/X11'

sgi-mips-irix6.4-gcc:	init
	$(MAKE) $(ARGS) \
		CC='gcc -D_POSIX_4SOURCE' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/$(SGIARCHLIB) -L/usr/local/lib -L/usr/lib/X11'

# [06-Jan-2000] Problems have been reported with SGI MIPSpro compilers
# version 7.x (x <= 3) for at least idict.o and isave.o when those
# files are compiled with optimization.  We therefore add a step to
# compile them without optimization.
sgi-mips-irix6.4-gnu-readline:	init
	$(MAKE) $(ARGS) \
		CFLAGS_STANDARD= \
		CC='cc $(SGIARCHFLAGS) -D_POSIX_4SOURCE' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)idict.o \
		$(GLOBJ)isave.o

	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -D_POSIX_4SOURCE' \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gdevpdf.o \
		$(GLOBJ)gdevps.o \
		$(GLOBJ)gdevtifs.o \
		$(GLOBJ)gpmisc.o \
		$(GLOBJ)gp_unix.o \
		$(GLOBJ)zdevcal.o
#
	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -ansi -D_POSIX_4SOURCE -woff 1185,1429 -OPT:Olimit=2500' \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gxclread.o

	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCHFLAGS) -ansi -D_POSIX_4SOURCE -woff 1185,1429' \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/$(SGIARCHLIB) -L/usr/local/lib -L/usr/lib/X11' \
		$(GNU_READLINE_ARGS) \
		XCFLAGS='-I. -I$(JSRCDIR) -I/usr/local/include -L/usr/local/lib32 -L/usr/local/lib'


# IRIX 6.5 can be treated like 6.4 for ghostscript builds:
sgi-mips-irix6.5: sgi-mips-irix6.4

sgi-mips-irix6.5-gnu-readline: sgi-mips-irix6.4-gnu-readline

sgi-mips-irix6.5-64bit:	init
	$(MAKE) $(ARGS) \
		CFLAGS_STANDARD= \
		CC='cc $(SGIARCH64FLAGS) -D_POSIX_4SOURCE' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)idict.o \
		$(GLOBJ)isave.o

	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCH64FLAGS) -D_POSIX_4SOURCE' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gdevpdf.o \
		$(GLOBJ)gdevps.o \
		$(GLOBJ)gdevtifs.o \
		$(GLOBJ)gpmisc.o \
		$(GLOBJ)gp_unix.o \
		$(GLOBJ)zdevcal.o
#
	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCH64FLAGS) -ansi -D_POSIX_4SOURCE -woff 1185,1429 -OPT:Olimit=2500' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/lib/X11 -L/usr/local/lib' \
		$(GLOBJ)gxclread.o

	$(MAKE) $(ARGS) \
		CC='cc $(SGIARCH64FLAGS) -ansi -D_POSIX_4SOURCE -woff 1185,1429' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/include/X11 \
		XLIBDIRS='-L/usr/local/$(SGIARCH64LIB) -L/usr/local/lib -L/usr/lib/X11'

sun-sparc-solaris:	init
	$(MAKE) $(ARGS) \
		CC='cc -Xc' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/openwin/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/openwin/lib'

sun-sparc-solaris-64bit:	init
	$(MAKE) $(ARGS) \
		CC='cc -Xc  -xarch=v9a' \
		FEATURE_DEVS_EXTRA= \
		XINCLUDE=-I/usr/openwin/include \
		XLIBDIRS='-L/usr/openwin/lib/sparcv9 -L/usr/local/lib64'

sun-sparc-solaris-gnu-readline:	init
	$(MAKE) $(ARGS) \
		CC='cc -Xc' \
		XINCLUDE=-I/usr/openwin/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/openwin/lib' \
		$(GNU_READLINE_ARGS)

sun-sparc-solaris-gcc:	init
	$(MAKE) $(ARGSGCC) \
		CC='gcc' \
		GCFLAGS=$(GCFLAGS) \
		XINCLUDE=-I/usr/openwin/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/openwin/lib' \
		EXTRALIBS='-ltermcap'

# For ps2pdf FullBook.ps, these optimization options only reduced the time by 3%!
sun-sparc-solaris-opt-gnu-readline:	init
	$(MAKE) $(ARGS) \
		CC='cc -Xc -xO5 -dalign -xlibmil -xcg92 -xtarget=ultra1/2170' \
		XINCLUDE=-I/usr/openwin/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/openwin/lib' \
		$(GNU_READLINE_ARGS)

# [21-Dec-1998] Add missing $(DD) prefix to sparc.dev
sun-sparc-solaris-newsprint:	init
	$(MAKE) $(ARGS) \
		CC='cc -Xc' \
		XINCLUDE=-I/usr/openwin/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/openwin/lib' \
		DEVICE_DEVS2=$(DD)sparc.dev

sun-sparc-solaris-pg:	init
	$(MAKE) $(ARGS) \
		CC='cc -Xc -xO5 -dalign -xlibmil -fsimple=2 -fns -xsafe=mem -xtarget=ultra1/170 -xpg' \
		XINCLUDE=-I/usr/openwin/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/openwin/lib -ldl'

# [21-Dec-1998] Add missing $(DD) prefix to sparc.dev
# [28-Nov-1997] Extra device(s) to be compiled into gs to support
# local needs Neither unixansi.mak nor unix-gcc.mak currently sets
# DEVICE_DEVS2, so we are free to list only our extra ones here:

# NB: Need -Dconst= for gcc 2.7.1 (unless gcc patch in make.doc is installed)
sun-sparc-sunos-gcc:	init
	$(MAKE) $(ARGSGCC) \
		CC='gcc -Dconst=' \
		FEATURE_DEVS_EXTRA= \
		GCFLAGS=$(GCFLAGS) \
		XINCLUDE=-I/usr/openwin/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/openwin/lib' \
		DEVICE_DEVS2=$(DD)sparc.dev \
		EXTRALIBS='-ltermcap'

sun-sparc-sunos-gcc-gnu-readline:	init
	$(MAKE) $(ARGSGCC) \
		CC='gcc -Dconst=' \
		GCFLAGS=$(GCFLAGS) \
		XINCLUDE=-I/usr/openwin/include \
		XLIBDIRS='-L/usr/local/lib -L/usr/openwin/lib' \
		DEVICE_DEVS2=$(DD)sparc.dev \
		$(GNU_READLINE_ARGS)

# Convenience targets: build by hostname, using settings from
# /usr/local/src/ghostscript/BUILD-GS.sh

# [02-Oct-1999]: remove -gnu-readline from these: I still have not
# yet had time to debug the problems it creates interfacing
# to ps2pk et al
GNUREADLINE=-gnu-readline
GNUREADLINE=

## XXXXXXXX.YYYYYYYY.utah.edu:		dec-alpha-osf$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:	ibm-rs6000-aix-4.2$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:		sgi-mips-irix6.5$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:		hp-parisc-hpux$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:		sgi-mips-irix6.3$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:	next-m68K-mach$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:			apple-powerpc-rhapsody5.5$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:		apple-powerpc-rhapsody5.5$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:		dec-alpha-osf$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:		linux$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:		linux$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:		sun-sparc-sunos-gcc$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:		sun-sparc-solaris$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:		sun-sparc-solaris$(GNUREADLINE)
## XXXXXXXX.YYYYYYYY.utah.edu:		sgi-mips-irix5$(GNUREADLINE)
