#    Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
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

# $Id: dvx-gcc.mak,v 1.29 2004/12/10 23:48:48 giles Exp $
# makefile for DesqView/X/gcc/X11 configuration.

#include $(COMMONDIR)/gccdefs.mak
#include $(COMMONDIR)/dvxdefs.mak
#include $(COMMONDIR)/generic.mak
include $(GLSRCDIR)/version.mak

# ------------------------------- Options ------------------------------- #

####### The following are the only parts of the file you should need to edit.

# ------ Generic options ------ #

# Define the directory for the final executable, and the
# source, generated intermediate file, and object directories
# for the graphics library (GL) and the PostScript/PDF interpreter (PS).

BINDIR=bin
GLSRCDIR=src
GLGENDIR=obj
GLOBJDIR=obj
PSSRCDIR=src
PSLIBDIR=lib
PSGENDIR=obj
PSOBJDIR=obj

# Define the installation commands and target directories for
# executables and files.  The commands are only relevant to `make install';
# the directories also define the default search path for the
# initialization files (gs_*.ps) and the fonts.

INSTALL = $(GLSRCDIR)/instcopy -c
INSTALL_PROGRAM = $(INSTALL) -m 755
INSTALL_DATA = $(INSTALL) -m 644

prefix = c:/bin
bindir = c:/bin
gsdatadir = c:/gs
gsfontdir = c:/gsfonts

docdir=$(gsdatadir)/doc
exdir=$(gsdatadir)/examples
GS_DOCDIR=$(docdir)

# Define the default directory/ies for the runtime
# initialization, resource and font files.  Separate multiple directories with a ;.

GS_LIB_DEFAULT="$(gsdatadir)/lib;$(gsdatadir)/Resource;$(gsfontdir)"

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

# Choose whether to use a shared version of the PNG library (-lpng).
# See gs.mak and Make.htm for more information.

SHARE_LIBPNG=0

# Define the directory where the zlib sources are stored.
# See zlib.mak for more information.

ZSRCDIR=zlib

# Choose whether to use a shared version of the zlib library (-lgz).
# See gs.mak and Make.htm for more information.

SHARE_ZLIB=0
ZLIB_NAME=gz

# Choose shared or compiled in libjbig2dec and source location

SHARE_JBIG2=0
JBIG2SRCDIR=jbig2dec

# Define the directory where the icclib source are stored.
# See icclib.mak for more information

ICCSRCDIR=icclib

# IJS has not been ported to DesqView/X. If you do the port,
# you'll need to set these values. You'll also need to
# include the ijs.mak makefile (right after icclib.mak).
#
# Define the directory where the ijs source is stored,
# and the process forking method to use for the server.
# See ijs.mak for more information.

#IJSSRCDIR=ijs
#IJSEXECTYPE=unix



# ------ Platform-specific options ------ #

# Define the name of the C compiler.

CC=gcc

# Define the other compilation flags.
# Add -DBSD4_2 for 4.2bsd systems.
# Add -DSYSV for System V or DG/UX.
# Add -DSYSV -D__SVR3 for SCO ODT, ISC Unix 2.2 or before,
#   or any System III Unix, or System V release 3-or-older Unix.
# Add -DSVR4 (not -DSYSV) for System V release 4.
# XCFLAGS can be set from the command line.
# We don't include -ansi, because this gets in the way of the platform-
#   specific stuff that <math.h> typically needs; nevertheless, we expect
#   gcc to accept ANSI-style function prototypes and function definitions.
XCFLAGS=

# Under DJGPP, since we strip the executable by default, we may as
# well *not* use '-g'.

# CFLAGS=-g -O $(XCFLAGS)
CFLAGS=-O $(XCFLAGS)

# Define platform flags for ld.
# Ultrix wants -x.
# SunOS 4.n may need -Bstatic.
# XLDFLAGS can be set from the command line.
XLDFLAGS=

LDFLAGS=$(XLDFLAGS)

# Define any extra libraries to link into the executable.
# ISC Unix 2.2 wants -linet.
# SCO Unix needs -lsocket if you aren't including the X11 driver.
# (Libraries required by individual drivers are handled automatically.)

EXTRALIBS=-lsys -lc

# Define the standard libraries to search at the end of linking.
# All reasonable platforms require -lm, but Rhapsody and perhaps one or
# two others fold libm into libc and require STDLIBS to be empty.

STDLIBS=-lm

# Define the include switch(es) for the X11 header files.
# This can be null if handled in some other way (e.g., the files are
# in /usr/include, or the directory is supplied by an environment variable);
# in particular, SCO Xenix, Unix, and ODT just want
#XINCLUDE=
# Note that x_.h expects to find the header files in $(XINCLUDE)/X11,
# not in $(XINCLUDE).

XINCLUDE=

# Define the directory/ies and library names for the X11 library files.
# The former can be null if these files are in the default linker search path.
# Unfortunately, Quarterdeck's old libraries did not conform to the
# X11 conventions for naming, in that the main Xlib library was called
# libx.a, not libx11.a.  To make things worse, both are provided in
# the v2.00 library.  Creation dates indicate that 'libx.a' is left
# over from a previous build (or this could just be on my system, but
# others who have upgraded from the early version will have the same
# problem---SJT).  Thus I will make the default to look for
# 'libx11.a', since v1.0x does *not* have it and the linker will
# complain.  With the reverse default, the linker will find to the
# obsolete library on some systems.

# XLIBDIRS includes a prefix -L; XLIBDIR does not.
XLIBDIRS=
XLIBDIR=
# reverse the comments if you have QDDVX10x or the prerelease version
# of QDLIB200 (still available on some Simtel mirrors, unfortunately)
# XLIBS=Xt Xext X
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
# primitives for this platform.  Don't change this unless you really know
# what you're doing.

SYNC=posync

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.

FEATURE_DEVS=$(PSD)psl3.dev $(PSD)pdf.dev $(PSD)dpsnext.dev $(PSD)ttfont.dev $(PSD)epsf.dev $(GLD)pipe.dev $(PSD)fapi.dev

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

# Choose the device(s) to include.  See devs.mak for details,
# devs.mak and contrib.mak for the list of available devices.

DEVICE_DEVS=x11.dev
DEVICE_DEVS1=
DEVICE_DEVS2=
DEVICE_DEVS3=deskjet.dev djet500.dev laserjet.dev ljetplus.dev ljet2p.dev ljet3.dev ljet3d.dev ljet4.dev ljet4d.dev
DEVICE_DEVS4=cdeskjet.dev cdjcolor.dev cdjmono.dev cdj550.dev pj.dev pjxl.dev pjxl300.dev
DEVICE_DEVS5=paintjet.dev pjetxl.dev uniprint.dev
DEVICE_DEVS6=
DEVICE_DEVS7=
DEVICE_DEVS8=
DEVICE_DEVS9=pbm.dev pbmraw.dev pgm.dev pgmraw.dev pgnm.dev pgnmraw.dev pnm.dev pnmraw.dev ppm.dev ppmraw.dev
DEVICE_DEVS10=tiffcrle.dev tiffg3.dev tiffg32d.dev tiffg4.dev tifflzw.dev tiffpack.dev
DEVICE_DEVS11=tiff12nc.dev tiff24nc.dev tiffgray.dev tiff32nc.dev tiffsep.dev
DEVICE_DEVS12=psmono.dev psgray.dev bit.dev bitrgb.dev bitcmyk.dev
DEVICE_DEVS13=
DEVICE_DEVS14=
DEVICE_DEVS15=pdfwrite.dev
DEVICE_DEVS16=
DEVICE_DEVS17=
DEVICE_DEVS18=
DEVICE_DEVS19=
DEVICE_DEVS20=

# ---------------------------- End of options --------------------------- #

# Define the name of the partial makefile that specifies options --
# used in dependencies.

MAKEFILE=$(GLSRCDIR)/dvx-gcc.mak
TOP_MAKEFILES=$(MAKEFILE) $(GLSRCDIR)/dvx-head.mak

# Define the auxiliary programs dependency.

AK=

# Define the compilation rules and flags.

CCFLAGS=$(GENOPT) $(CFLAGS)
CC_=$(CC) $(CCFLAGS)
CC_NO_WARN=$(CC_)

# ---------------- End of platform-specific section ---------------- #

include $(GLSRCDIR)/dvx-head.mak
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
include $(GLSRCDIR)/dvx-tail.mak
include $(GLSRCDIR)/unix-end.mak
include $(GLSRCDIR)/unixinst.mak
