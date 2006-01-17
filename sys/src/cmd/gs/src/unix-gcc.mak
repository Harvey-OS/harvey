#    Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: unix-gcc.mak,v 1.50 2005/08/31 05:52:32 ray Exp $
# makefile for Unix/gcc/X11 configuration.

# ------------------------------- Options ------------------------------- #

####### The following are the only parts of the file you should need to edit.

# Define the directory for the final executable, and the
# source, generated intermediate file, and object directories
# for the graphics library (GL) and the PostScript/PDF interpreter (PS).

BINDIR=./bin
GLSRCDIR=./src
GLGENDIR=./obj
GLOBJDIR=./obj
PSSRCDIR=./src
PSLIBDIR=./lib
PSGENDIR=./obj
PSOBJDIR=./obj

# Do not edit the next group of lines.

#include $(COMMONDIR)/gccdefs.mak
#include $(COMMONDIR)/unixdefs.mak
#include $(COMMONDIR)/generic.mak
include $(GLSRCDIR)/version.mak
DD=$(GLGENDIR)/
GLD=$(GLGENDIR)/
PSD=$(PSGENDIR)/

# ------ Generic options ------ #

# Define the installation commands and target directories for
# executables and files.  The commands are only relevant to `make install';
# the directories also define the default search path for the
# initialization files (gs_*.ps) and the fonts.

INSTALL = $(GLSRCDIR)/instcopy -c
INSTALL_PROGRAM = $(INSTALL) -m 755
INSTALL_DATA = $(INSTALL) -m 644

prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
scriptdir = $(bindir)
libdir = $(exec_prefix)/lib
mandir = $(prefix)/man
man1ext = 1
datadir = $(prefix)/share
gsdir = $(datadir)/ghostscript
gsdatadir = $(gsdir)/$(GS_DOT_VERSION)

docdir=$(gsdatadir)/doc
exdir=$(gsdatadir)/examples
GS_DOCDIR=$(docdir)

# Define the default directory/ies for the runtime
# initialization, resource and font files.  Separate multiple directories with a :.

GS_LIB_DEFAULT=$(gsdatadir)/lib:$(gsdatadir)/Resource:$(gsdir)/fonts

# Define whether or not searching for initialization files should always
# look in the current directory first.  This leads to well-known security
# and confusion problems, but users insist on it.
# NOTE: this also affects searching for files named on the command line:
# see the "File searching" section of Use.htm for full details.
# Because of this, setting SEARCH_HERE_FIRST to 0 is not recommended.

SEARCH_HERE_FIRST=1

# Define the name of the interpreter initialization file.
# (There is no reason to change this.)

GS_INIT=gs_init.ps

# Choose generic configuration options.

# -DDEBUG
#	includes debugging features (-Z switch) in the code.
#	  Code runs substantially slower even if no debugging switches
#	  are set.
# -DNOPRIVATE
#	makes private (static) procedures and variables public,
#	  so they are visible to the debugger and profiler.
#	  No execution time or space penalty.

GENOPT=

# Choose capability options.

# -DHAVE_MKSTEMP
#	uses mkstemp instead of mktemp
#		This gets rid of several security warnings that look
#		ominous.  Enable this if you wish to get rid of them.

CAPOPT= -DHAVE_MKSTEMP

# Define the name of the executable file.

GS=gs

# Define the name of a pre-built executable that can be invoked at build
# time.  Currently, this is only needed for compiled fonts.  The usual
# alternatives are:
#   - the standard name of Ghostscript on your system (typically `gs'):
BUILD_TIME_GS=gs
#   - the name of the executable you are building now.  If you choose this
# option, then you must build the executable first without compiled fonts,
# and then again with compiled fonts.
#BUILD_TIME_GS=$(BINDIR)/$(GS) -I$(PSLIBDIR)

# Define the directories for debugging and profiling binaries, relative to
# the standard binaries.

DEBUGRELDIR=../debugobj
PGRELDIR=../pgobj

# Define the directory where the IJG JPEG library sources are stored,
# and the major version of the library that is stored there.
# You may need to change this if the IJG library version changes.
# See jpeg.mak for more information.

JSRCDIR=jpeg
JVERSION=6

# Note: if a shared library is used, it may not contain the
# D_MAX_BLOCKS_IN_MCU patch, and thus may not be able to read
# some older JPEG streams that violate the standard. If the JPEG
# library built from local sources, the patch will be applied.

SHARE_JPEG=0
JPEG_NAME=jpeg

# Define the directory where the PNG library sources are stored,
# and the version of the library that is stored there.
# You may need to change this if the libpng version changes.
# See libpng.mak for more information.

PSRCDIR=libpng
PVERSION=10208

# Choose whether to use a shared version of the PNG library, and if so,
# what its name is.
# See gs.mak and Make.htm for more information.

SHARE_LIBPNG=0
LIBPNG_NAME=png

# Define the directory where the zlib sources are stored.
# See zlib.mak for more information.

ZSRCDIR=zlib

# Choose whether to use a shared version of the zlib library, and if so,
# what its name is (usually libz, but sometimes libgz).
# See gs.mak and Make.htm for more information.

SHARE_ZLIB=0
#ZLIB_NAME=gz
ZLIB_NAME=z

# Choose shared or compiled in libjbig2dec and source location
SHARE_JBIG2=0
JBIG2SRCDIR=jbig2dec

# Define the directory where the icclib source are stored.
# See icclib.mak for more information

ICCSRCDIR=icclib

# Define the directory where the ijs source is stored,
# and the process forking method to use for the server.
# See ijs.mak for more information.

IJSSRCDIR=ijs
IJSEXECTYPE=unix

# Define how to build the library archives.  (These are not used in any
# standard configuration.)

AR=ar
ARFLAGS=qc
RANLIB=ranlib

# ------ Platform-specific options ------ #

# Define the name of the C compiler.

CC=gcc

# Define the name of the linker for the final link step.
# Normally this is the same as the C compiler.

CCLD=$(CC)

# Define the default gcc flags.
# Note that depending whether or not we are running a version of gcc with
# the 2.7.0-2.7.2 optimizer bug, either "-Dconst=" or
# "-Wcast-qual -Wwrite-strings" is automatically included.

GCFLAGS=-Wall -Wstrict-prototypes -Wmissing-declarations -Wmissing-prototypes -fno-builtin -fno-common

# Define the added flags for standard, debugging, profiling 
# and shared object builds.

CFLAGS_STANDARD=-O2
CFLAGS_DEBUG=-g -O0
CFLAGS_PROFILE=-pg -O2
CFLAGS_SO=-fPIC

# Define the other compilation flags.  Add at most one of the following:
#	-DBSD4_2 for 4.2bsd systems.
#	-DSYSV for System V or DG/UX.
# 	-DSYSV -D__SVR3 for SCO ODT, ISC Unix 2.2 or before,
#	   or any System III Unix, or System V release 3-or-older Unix.
#	-DSVR4 -DSVR4_0 (not -DSYSV) for System V release 4.0.
#	-DSVR4 (not -DSYSV) for System V release 4.2 (or later) and Solaris 2.
# XCFLAGS can be set from the command line.
# We don't include -ansi, because this gets in the way of the platform-
#   specific stuff that <math.h> typically needs; nevertheless, we expect
#   gcc to accept ANSI-style function prototypes and function definitions.
XCFLAGS=

CFLAGS=$(CFLAGS_STANDARD) $(GCFLAGS) $(XCFLAGS)

# Define platform flags for ld.
# SunOS 4.n may need -Bstatic.
# Solaris 2.6 (and possibly some other versions) with any of the SHARE_
# parameters set to 1 may need
#	-R /usr/local/xxx/lib:/usr/local/lib
# giving the full path names of the shared library directories.
# XLDFLAGS can be set from the command line.
XLDFLAGS=

LDFLAGS=$(XLDFLAGS)

# Define any extra libraries to link into the executable.
# ISC Unix 2.2 wants -linet.
# SCO Unix needs -lsocket if you aren't including the X11 driver.
# SVR4 may need -lnsl.
# Solaris may need -lnsl -lsocket -lposix4.
# (Libraries required by individual drivers are handled automatically.)

EXTRALIBS=

# Define the standard libraries to search at the end of linking.
# Most platforms require -lpthread for the POSIX threads library;
# on FreeBSD, change -lpthread to -lc_r; BSDI and perhaps some others
# include pthreads in libc and don't require any additional library.
# All reasonable platforms require -lm, but Rhapsody and perhaps one or
# two others fold libm into libc and don't require any additional library.

#STDLIBS=-lpthread -lm

# Since the default build is for nosync, don't include pthread lib
STDLIBS=-lm

# Define the include switch(es) for the X11 header files.
# This can be null if handled in some other way (e.g., the files are
# in /usr/include, or the directory is supplied by an environment variable);
# in particular, SCO Xenix, Unix, and ODT just want
#XINCLUDE=
# Note that x_.h expects to find the header files in $(XINCLUDE)/X11,
# not in $(XINCLUDE).

XINCLUDE=-I/usr/X11R6/include

# Define the directory/ies and library names for the X11 library files.
# XLIBDIRS is for ld and should include -L; XLIBDIR is for LD_RUN_PATH
# (dynamic libraries on SVR4) and should not include -L.
# Newer SVR4 systems can use -R in XLIBDIRS rather than setting XLIBDIR.
# Both can be null if these files are in the default linker search path;
# in particular, SCO Xenix, Unix, and ODT just want
#XLIBDIRS=
# Solaris and other SVR4 systems with dynamic linking probably want
#XLIBDIRS=-L/usr/openwin/lib -R/usr/openwin/lib
# X11R6 (on any platform) may need
#XLIBS=Xt SM ICE Xext X11

#XLIBDIRS=-L/usr/local/X/lib
XLIBDIRS=-L/usr/X11R6/lib
XLIBDIR=
XLIBS=Xt Xext X11

# Define whether this platform has floating point hardware:
#	FPU_TYPE=2 means floating point is faster than fixed point.
# (This is the case on some RISCs with multiple instruction dispatch.)
#	FPU_TYPE=1 means floating point is at worst only slightly slower
# than fixed point.
#	FPU_TYPE=0 means that floating point may be considerably slower.
#	FPU_TYPE=-1 means that floating point is always much slower than
# fixed point.

FPU_TYPE=1

# Define the .dev module that implements thread and synchronization
# primitives for this platform.

# If POSIX sync primitives are used, also change the STDLIBS to include
# the pthread library.
#SYNC=posync

# Default is No sync primitives since some platforms don't have it (HP-UX)
SYNC=nosync

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.

FEATURE_DEVS=$(PSD)psl3.dev $(PSD)pdf.dev $(PSD)dpsnext.dev $(PSD)ttfont.dev $(PSD)epsf.dev $(GLD)pipe.dev $(PSD)fapi.dev
#FEATURE_DEVS=$(PSD)psl3.dev $(PSD)pdf.dev
#FEATURE_DEVS=$(PSD)psl3.dev $(PSD)pdf.dev $(PSD)dpsnext.dev $(PSD)ttfont.dev $(PSD)rasterop.dev $(GLD)pipe.dev
# The following is strictly for testing.
FEATURE_DEVS_ALL=$(PSD)psl3.dev $(PSD)pdf.dev $(PSD)dpsnext.dev $(PSD)ttfont.dev $(PSD)rasterop.dev $(PSD)double.dev $(PSD)trapping.dev $(PSD)stocht.dev $(GLD)pipe.dev
#FEATURE_DEVS=$(FEATURE_DEVS_ALL)

# Choose whether to compile the .ps initialization files into the executable.
# See gs.mak for details.

COMPILE_INITS=0

# Choose whether to store band lists on files or in memory.
# The choices are 'file' or 'memory'.

BAND_LIST_STORAGE=file

# Choose which compression method to use when storing band lists in memory.
# The choices are 'lzw' or 'zlib'.

BAND_LIST_COMPRESSOR=zlib

# Choose the implementation of file I/O: 'stdio', 'fd', or 'both'.
# See gs.mak and sfxfd.c for more details.

FILE_IMPLEMENTATION=stdio

# Choose the implementation of stdio: '' for file I/O and 'c' for callouts
# See gs.mak and ziodevs.c/ziodevsc.c for more details.

STDIO_IMPLEMENTATION=c

# Override the default device.  This is set to 'display' by 
# unix-dll.mak when building a shared object.
DISPLAY_DEV=

# Define the name table capacity size of 2^(16+n).
# Setting this to a non-zero value will slow down the interpreter.

EXTEND_NAMES=0

# Choose the device(s) to include.  See devs.mak for details,
# devs.mak and contrib.mak for the list of available devices.

DEVICE_DEVS=$(DISPLAY_DEV) $(DD)x11.dev $(DD)x11alpha.dev $(DD)x11cmyk.dev $(DD)x11gray2.dev $(DD)x11gray4.dev $(DD)x11mono.dev

#DEVICE_DEVS1=
#DEVICE_DEVS2=
#DEVICE_DEVS3=
#DEVICE_DEVS4=
#DEVICE_DEVS5=
#DEVICE_DEVS6=
#DEVICE_DEVS7=
#DEVICE_DEVS8=
#DEVICE_DEVS9=
#DEVICE_DEVS10=
#DEVICE_DEVS11=
#DEVICE_DEVS12=
#DEVICE_DEVS13=
#DEVICE_DEVS14=
#DEVICE_DEVS15=
#DEVICE_DEVS16=
#DEVICE_DEVS17=
#DEVICE_DEVS18=
#DEVICE_DEVS19=
#DEVICE_DEVS20=

DEVICE_DEVS1=$(DD)bmpmono.dev $(DD)bmpgray.dev $(DD)bmpsep1.dev $(DD)bmpsep8.dev $(DD)bmp16.dev $(DD)bmp256.dev $(DD)bmp16m.dev $(DD)bmp32b.dev $(DD)stcolor.dev
DEVICE_DEVS2=$(DD)epson.dev $(DD)eps9high.dev $(DD)eps9mid.dev $(DD)epsonc.dev $(DD)ibmpro.dev
DEVICE_DEVS3=$(DD)deskjet.dev $(DD)djet500.dev $(DD)laserjet.dev $(DD)ljetplus.dev $(DD)ljet2p.dev $(DD)ljet3.dev $(DD)ljet3d.dev $(DD)ljet4.dev $(DD)ljet4d.dev $(DD)lj5mono.dev $(DD)lj5gray.dev
DEVICE_DEVS4=$(DD)cdeskjet.dev $(DD)cdjcolor.dev $(DD)cdjmono.dev $(DD)cdj550.dev $(DD)pj.dev $(DD)pjxl.dev $(DD)pjxl300.dev
DEVICE_DEVS5=$(DD)uniprint.dev $(DD)ijs.dev
DEVICE_DEVS6=$(DD)bj10e.dev $(DD)bj200.dev $(DD)bjc600.dev $(DD)bjc800.dev
DEVICE_DEVS7=$(DD)faxg3.dev $(DD)faxg32d.dev $(DD)faxg4.dev
DEVICE_DEVS8=$(DD)pcxmono.dev $(DD)pcxgray.dev $(DD)pcx16.dev $(DD)pcx256.dev $(DD)pcx24b.dev $(DD)pcxcmyk.dev
DEVICE_DEVS9=$(DD)pbm.dev $(DD)pbmraw.dev $(DD)pgm.dev $(DD)pgmraw.dev $(DD)pgnm.dev $(DD)pgnmraw.dev $(DD)pnm.dev $(DD)pnmraw.dev $(DD)ppm.dev $(DD)ppmraw.dev $(DD)pkm.dev $(DD)pkmraw.dev $(DD)pksm.dev $(DD)pksmraw.dev
DEVICE_DEVS10=$(DD)tiffcrle.dev $(DD)tiffg3.dev $(DD)tiffg32d.dev $(DD)tiffg4.dev $(DD)tifflzw.dev $(DD)tiffpack.dev
DEVICE_DEVS11=$(DD)tiff12nc.dev $(DD)tiff24nc.dev $(DD)tiffgray.dev $(DD)tiff32nc.dev $(DD)tiffsep.dev
DEVICE_DEVS12=$(DD)psmono.dev $(DD)psgray.dev $(DD)psrgb.dev $(DD)bit.dev $(DD)bitrgb.dev $(DD)bitcmyk.dev
DEVICE_DEVS13=$(DD)pngmono.dev $(DD)pnggray.dev $(DD)png16.dev $(DD)png256.dev $(DD)png16m.dev $(DD)pngalpha.dev
DEVICE_DEVS14=$(DD)jpeg.dev $(DD)jpeggray.dev $(DD)jpegcmyk.dev
DEVICE_DEVS15=$(DD)pdfwrite.dev $(DD)pswrite.dev $(DD)ps2write.dev $(DD)epswrite.dev $(DD)pxlmono.dev $(DD)pxlcolor.dev
DEVICE_DEVS16=$(DD)bbox.dev

DEVICE_DEVS17=
DEVICE_DEVS18=
DEVICE_DEVS19=
DEVICE_DEVS20=$(DD)cljet5.dev $(DD)cljet5c.dev
DEVICE_DEVS21=$(DD)spotcmyk.dev $(DD)devicen.dev $(DD)xcf.dev $(DD)bmpsep1.dev $(DD)bmpsep8.dev $(DD)bmp16m.dev $(DD)bmp32b.dev $(DD)psdcmyk.dev $(DD)psdrgb.dev

# ---------------------------- End of options --------------------------- #

# Define the name of the partial makefile that specifies options --
# used in dependencies.

MAKEFILE=$(GLSRCDIR)/unix-gcc.mak
TOP_MAKEFILES=$(MAKEFILE) $(GLSRCDIR)/unixhead.mak

# Define the auxiliary programs dependency. There aren't any, but we use
# this as a hook to detect whether we're running a version of gcc with
# the const optimization bug.

AK=$(GLGENDIR)/cc.tr

# Define the compilation rules and flags.

CCFLAGS=$(GENOPT) $(CAPOPT) $(CFLAGS) -DGX_COLOR_INDEX_TYPE='unsigned long long'
CC_=$(CC) `cat $(AK)` $(CCFLAGS)
CCAUX=$(CC) `cat $(AK)`
# These are the specific warnings we have to turn off to compile those
# specific few files that need this.  We may turn off others in the future.
CC_NO_WARN=$(CC_) -Wno-cast-qual -Wno-traditional

# ---------------- End of platform-specific section ---------------- #

include $(GLSRCDIR)/unixhead.mak
include $(GLSRCDIR)/gs.mak
include $(GLSRCDIR)/lib.mak
include $(PSSRCDIR)/int.mak
include $(PSSRCDIR)/cfonts.mak
include $(GLSRCDIR)/jpeg.mak
# zlib.mak must precede libpng.mak
include $(GLSRCDIR)/zlib.mak
include $(GLSRCDIR)/libpng.mak
include $(GLSRCDIR)/jbig2.mak
include $(GLSRCDIR)/icclib.mak
include $(GLSRCDIR)/ijs.mak
include $(GLSRCDIR)/devs.mak
include $(GLSRCDIR)/contrib.mak
include $(GLSRCDIR)/unix-aux.mak
include $(GLSRCDIR)/unixlink.mak
include $(GLSRCDIR)/unix-dll.mak
include $(GLSRCDIR)/unix-end.mak
include $(GLSRCDIR)/unixinst.mak

# This has to come last so it won't be taken as the default target.
$(AK):
	if ( $(CC) --version | egrep "^2\.7\.([01]|2(\.[^1-9]|$$))" >/dev/null ); then echo -Dconst= >$(AK); else echo -Wcast-qual -Wwrite-strings >$(AK); fi

# platform-specific clean-up
# this makefile is intended to be hand edited so we don't distribute
# the (presumedly modified) version in the top level directory
distclean : clean config-clean
	-$(RM) Makefile

maintainer-clean : distclean
	# nothing special to do


