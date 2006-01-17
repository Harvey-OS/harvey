#    Copyright (C) 1995, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: ugcclib.mak,v 1.29 2004/12/10 23:48:48 giles Exp $
# makefile for Unix / gcc library testing.

BINDIR=./libobj
GLSRCDIR=./src
GLGENDIR=./libobj
GLOBJDIR=./libobj
DD=$(GLGENDIR)/
GLD=$(GLGENDIR)/

#include $(COMMONDIR)/gccdefs.mak
#include $(COMMONDIR)/unixdefs.mak
#include $(COMMONDIR)/generic.mak
include $(GLSRCDIR)/version.mak

gsdir = /usr/local/share/ghostscript
gsdatadir = $(gsdir)/$(GS_DOT_VERSION)
GS_DOCDIR=$(gsdatadir)/doc
GS_LIB_DEFAULT=$(gsdatadir)/lib:$(gsdatadir)/Resource:$(gsdir)/fonts
SEARCH_HERE_FIRST=1
GS_INIT=gs_init.ps

#GENOPT=-DDEBUG
GENOPT=
GS=gslib

# We don't expect to build debug or profiling configurations....
DEBUGRELDIR=.
PGRELDIR=.

JSRCDIR=jpeg
JVERSION=6

SHARE_JPEG=0
JPEG_NAME=jpeg

PSRCDIR=libpng
PVERSION=10208
SHARE_LIBPNG=1
LIBPNG_NAME=png

ZSRCDIR=zlib
SHARE_ZLIB=1
ZLIB_NAME=z

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

CC=gcc
CCLD=$(CC)

GCFLAGS_NO_WARN=-fno-builtin -fno-common
GCFLAGS_WARNINGS=-Wall -Wcast-qual -Wpointer-arith -Wstrict-prototypes -Wwrite-strings
GCFLAGS=$(GCFLAGS_NO_WARN) $(GCFLAGS_WARNINGS)
XCFLAGS=
CFLAGS_STANDARD=-O2
CFLAGS_DEBUG=-g -O
CFLAGS_PROFILE=-pg -O2
CFLAGS=$(CFLAGS_DEBUG) $(GCFLAGS) $(XCFLAGS)
LDFLAGS=$(XLDFLAGS)
STDLIBS=-lm
EXTRALIBS=
XINCLUDE=-I/usr/X11R6/include
XLIBDIRS=-L/usr/X11R6/lib
XLIBDIR=
XLIBS=Xt Xext X11

FPU_TYPE=1
SYNC=posync

FEATURE_DEVS=$(GLD)dps2lib.dev $(GLD)psl2cs.dev $(GLD)cielib.dev\
 $(GLD)psl3lib.dev $(GLD)path1lib.dev $(GLD)patlib.dev $(GLD)htxlib.dev \
 $(GLD)roplib.dev $(GLD)devcmap.dev
COMPILE_INITS=0
BAND_LIST_STORAGE=file
BAND_LIST_COMPRESSOR=zlib
FILE_IMPLEMENTATION=stdio
STDIO_IMPLEMENTATION=
DEVICE_DEVS=$(DD)x11cmyk.dev $(DD)x11mono.dev $(DD)x11.dev $(DD)x11alpha.dev\
 $(DD)djet500.dev $(DD)pbmraw.dev $(DD)pgmraw.dev $(DD)ppmraw.dev\
 $(DD)bitcmyk.dev $(GLD)bbox.dev
DEVICE_DEVS1=
DEVICE_DEVS2=
DEVICE_DEVS3=
DEVICE_DEVS4=
DEVICE_DEVS5=
DEVICE_DEVS6=
DEVICE_DEVS7=
DEVICE_DEVS8=
DEVICE_DEVS9=
DEVICE_DEVS10=
DEVICE_DEVS11=
DEVICE_DEVS12=
DEVICE_DEVS13=
DEVICE_DEVS14=
DEVICE_DEVS15=
DEVICE_DEVS16=
DEVICE_DEVS17=
DEVICE_DEVS18=
DEVICE_DEVS19=
DEVICE_DEVS20=

MAKEFILE=$(GLSRCDIR)/ugcclib.mak
TOP_MAKEFILES=$(MAKEFILE)

AK=
CCFLAGS=$(GENOPT) $(CFLAGS)
CC_=$(CC) $(CCFLAGS)
CCAUX=$(CC)
CC_NO_WARN=$(CC_) -Wno-cast-qual -Wno-traditional

include $(GLSRCDIR)/unixhead.mak
include $(GLSRCDIR)/gs.mak
include $(GLSRCDIR)/lib.mak
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

# The following replaces unixlink.mak

LIB_ONLY=$(GLOBJ)gsnogc.$(OBJ) $(GLOBJ)gconfig.$(OBJ) $(GLOBJ)gscdefs.$(OBJ)
ldt_tr=$(GLOBJ)ldt.tr
$(GS_XE): $(ld_tr) $(ECHOGS_XE) $(LIB_ALL) $(DEVS_ALL) $(GLOBJ)gslib.$(OBJ) $(LIB_ONLY)
	$(ECHOGS_XE) -w $(ldt_tr) -n - $(CCLD) $(LDFLAGS) -o $(GS_XE)
	$(ECHOGS_XE) -a $(ldt_tr) -n -s $(GLOBJ)gslib.$(OBJ) -s
	$(ECHOGS_XE) -a $(ldt_tr) -n -s $(LIB_ONLY) -s
	cat $(ld_tr) >>$(ldt_tr)
	$(ECHOGS_XE) -a $(ldt_tr) -s - $(EXTRALIBS) $(STDLIBS)
	if [ x$(XLIBDIR) != x ]; then LD_RUN_PATH=$(XLIBDIR); export LD_RUN_PATH; fi; $(SH) <$(ldt_tr)


GSLIB_A=libgsgraph.a
lar_tr=$(GLOBJ)lar.tr
$(GSLIB_A):  $(obj_tr) $(ECHOGS_XE) $(LIB_ALL) $(DEVS_ALL) $(LIB_ONLY)
	rm -f $(GSLIB_A)
	$(ECHOGS_XE) -w $(lar_tr) -n - $(AR) $(ARFLAGS) $(GSLIB_A)
	$(ECHOGS_XE) -a $(lar_tr) -n -s $(LIB_ONLY) -s
	cat $(obj_tr) >>$(lar_tr)
	$(ECHOGS_XE) -a $(lar_tr) -s -
	$(SH) <$(lar_tr)
	$(RANLIB) $(GSLIB_A)

include $(GLSRCDIR)/unix-end.mak
