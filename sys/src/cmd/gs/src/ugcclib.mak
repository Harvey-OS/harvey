#    Copyright (C) 1995, 2000 Aladdin Enterprises.  All rights reserved.
# 
# This file is part of AFPL Ghostscript.
# 
# AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
# distributor accepts any responsibility for the consequences of using it, or
# for whether it serves any particular purpose or works at all, unless he or
# she says so in writing.  Refer to the Aladdin Free Public License (the
# "License") for full details.
# 
# Every copy of AFPL Ghostscript must include a copy of the License, normally
# in a plain ASCII text file named PUBLIC.  The License grants you the right
# to copy, modify and redistribute AFPL Ghostscript, but only under certain
# conditions described in the License.  Among other things, the License
# requires that the copyright notice and this notice be preserved on all
# copies.

# $Id: ugcclib.mak,v 1.5.2.1 2000/11/28 00:14:34 raph Exp $
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
GS_LIB_DEFAULT=$(gsdatadir)/lib:$(gsdir)/fonts
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
# DON'T SET THIS TO 1!
SHARE_JPEG=0
JPEG_NAME=jpeg

PSRCDIR=libpng
PVERSION=10008
SHARE_LIBPNG=1
LIBPNG_NAME=png

ZSRCDIR=zlib
SHARE_ZLIB=1
ZLIB_NAME=z

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
XINCLUDE=-I/usr/local/X/include
XLIBDIRS=-L/usr/X11/lib
XLIBDIR=
XLIBS=Xt Xext X11

FPU_TYPE=1
SYNC=posync

FEATURE_DEVS=$(GLD)dps2lib.dev $(GLD)psl2cs.dev $(GLD)cielib.dev\
 $(GLD)imasklib.dev $(GLD)patlib.dev $(GLD)htxlib.dev $(GLD)roplib.dev\
 $(GLD)devcmap.dev
COMPILE_INITS=0
BAND_LIST_STORAGE=file
BAND_LIST_COMPRESSOR=zlib
FILE_IMPLEMENTATION=stdio
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
CC_LEAF=$(CC_)
CC_NO_WARN=$(CC_) -Wno-cast-qual -Wno-traditional
# When using gcc, CCA2K isn't needed....
CCA2K=$(CC)

include $(GLSRCDIR)/unixhead.mak
include $(GLSRCDIR)/gs.mak
include $(GLSRCDIR)/lib.mak
include $(GLSRCDIR)/jpeg.mak
# zlib.mak must precede libpng.mak
include $(GLSRCDIR)/zlib.mak
include $(GLSRCDIR)/libpng.mak
include $(GLSRCDIR)/devs.mak
include $(GLSRCDIR)/contrib.mak
include $(GLSRCDIR)/unix-aux.mak

# The following replaces unixlink.mak

LIB_ONLY=$(GLOBJ)gslib.$(OBJ) $(GLOBJ)gsnogc.$(OBJ) $(GLOBJ)gconfig.$(OBJ) $(GLOBJ)gscdefs.$(OBJ)
ldt_tr=$(GLOBJ)ldt.tr
$(GS_XE): $(ld_tr) $(ECHOGS_XE) $(LIB_ALL) $(DEVS_ALL) $(LIB_ONLY)
	$(ECHOGS_XE) -w $(ldt_tr) -n - $(CCLD) $(LDFLAGS) -o $(GS_XE)
	$(ECHOGS_XE) -a $(ldt_tr) -n -s $(LIB_ONLY) -s
	cat $(ld_tr) >>$(ldt_tr)
	$(ECHOGS_XE) -a $(ldt_tr) -s - $(EXTRALIBS) $(STDLIBS)
	if [ x$(XLIBDIR) != x ]; then LD_RUN_PATH=$(XLIBDIR); export LD_RUN_PATH; fi; $(SH) <$(ldt_tr)

include $(GLSRCDIR)/unix-end.mak
