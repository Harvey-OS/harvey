#    Copyright (C) 1991, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
# 
# This file is part of Aladdin Ghostscript.
# 
# Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
# or distributor accepts any responsibility for the consequences of using it,
# or for whether it serves any particular purpose or works at all, unless he
# or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
# License (the "License") for full details.
# 
# Every copy of Aladdin Ghostscript must include a copy of the License,
# normally in a plain ASCII text file named PUBLIC.  The License grants you
# the right to copy, modify and redistribute Aladdin Ghostscript, but only
# under certain conditions described in the License.  Among other things, the
# License requires that the copyright notice and this notice be preserved on
# all copies.

# $Id: watclib.mak,v 1.2 2000/03/10 15:48:58 lpd Exp $
# makefile for MS-DOS / Watcom C/C++ library testing.

libdefault: $(GLOBJ)gslib.exe
	$(NO_OP)

AROOTDIR=c:/Aladdin
GSROOTDIR=$(AROOTDIR)/gs$(GS_DOT_VERSION)
GS_DOCDIR=$(GSROOTDIR)/doc
GS_LIB_DEFAULT=$(GSROOTDIR)/lib\;$(AROOTDIR)/fonts
SEARCH_HERE_FIRST=1
GS_INIT=gs_init.ps

!ifndef DEBUG
DEBUG=1
!endif
!ifndef TDEBUG
TDEBUG=1
!endif
!ifndef NOPRIVATE
NOPRIVATE=1
!endif

GS=gslib

!ifndef BINDIR
BINDIR=.\debugobj
!endif
!ifndef GLSRCDIR
GLSRCDIR=.\src
!endif
!ifndef GLGENDIR
GLGENDIR=.\debugobj
!endif
!ifndef GLOBJDIR
GLOBJDIR=.\debugobj
!endif

# Do not edit the next group of lines.
NUL=
DD=$(GLGENDIR)\$(NUL)
GLD=$(GLGENDIR)\$(NUL)

!ifndef JSRCDIR
JSRCDIR=jpeg
!endif
!ifndef JVERSION
JVERSION=6
!endif

!ifndef PSRCDIR
PSRCDIR=libpng
!endif
!ifndef PVERSION
PVERSION=10005
!endif

!ifndef ZSRCDIR
ZSRCDIR=zlib
!endif

CFLAGS=

!ifndef WCVERSION
WCVERSION=10.0
!endif
LIBPATHS=LIBPATH $(%WATCOM)\lib386 LIBPATH $(%WATCOM)\lib386\dos
STUB=$(%WATCOM)\binb\wstub.exe

!ifndef CPU_TYPE
CPU_TYPE=386
!endif
!ifndef FPU_TYPE
FPU_TYPE=0
!endif

!ifndef SYNC
SYNC=winsync
!endif

PLATFORM=watclib_
MAKEFILE=$(GLSRCDIR)\watclib.mak
TOP_MAKEFILES=$(MAKEFILE) $(GLSRCDIR)\wccommon.mak
PLATOPT=

!include $(GLSRCDIR)\wccommon.mak

!ifndef FEATURE_DEVS
FEATURE_DEVS=$(GLD)patlib.dev $(GLD)path1lib.dev $(GLD)hsblib.dev
!endif
!ifndef DEVICE_DEVS
DEVICE_DEVS=$(DD)vga.dev
!endif
!ifndef COMPILE_INITS
COMPILE_INITS=0
!endif
!ifndef BAND_LIST_STORAGE
BAND_LIST_STORAGE=file
!endif
!ifndef BAND_LIST_COMPRESSOR
BAND_LIST_COMPRESSOR=zlib
!endif
!ifndef FILE_IMPLEMENTATION
FILE_IMPLEMENTATION=stdio
!endif

!include $(GLSRCDIR)\wctail.mak
!include $(GLSRCDIR)\devs.mak
!include $(GLSRCDIR)\contrib.mak

GLCCWIN=$(GLCC)
!include $(GLSRCDIR)\winplat.mak

watclib_1=$(GLOBJ)gp_getnv.$(OBJ) $(GLOBJ)gp_iwatc.$(OBJ) $(GLOBJ)gp_dosfb.$(OBJ)
!ifeq WAT32 0
watclib_2=$(GLOBJ)gp_dosfs.$(OBJ) $(GLOBJ)gp_dosfe.$(OBJ) $(GLOBJ)gp_msdos.$(OBJ)
watclib_inc=
!else
watclib_2=
watclib_inc=$(GLD)winplat.dev
!endif
watclib__=$(watclib_1) $(watclib_2)
$(GLGEN)watclib_.dev: $(watclib__) $(GLGEN)nosync.dev $(watclib_inc)
	$(SETMOD) $(GLGEN)watclib_ $(watclib_1)
	$(ADDMOD) $(GLGEN)watclib_ -obj $(watclib_2)
	$(ADDMOD) $(GLGEN)watclib_ -include $(GLGEN)nosync $(watclib_inc)

$(GLOBJ)gp_iwatc.$(OBJ): $(GLSRC)gp_iwatc.c $(stat__h) $(string__h)\
 $(gx_h) $(gp_h)
	$(GLCC) $(GLO_)gp_iwatc.$(OBJ) $(C_) $(GLSRC)gp_iwatc.c

BEGINFILES=*.err

LIB_ONLY=$(GLOBJ)gslib.obj $(GLOBJ)gsnogc.obj $(GLOBJ)gconfig.obj $(GLOBJ)gscdefs.obj
ll_tr=ll.tr
$(ll_tr): $(TOP_MAKEFILES)
	echo OPTION STACK=64k >$(ll_tr)
!ifeq WAT32 0
	echo SYSTEM DOS4G >>$(ll_tr)
	echo OPTION STUB=$(STUB) >>$(ll_tr)
!endif
	echo FILE $(GLOBJ)gsnogc.obj >>$(ll_tr)
	echo FILE $(GLOBJ)gconfig.obj >>$(ll_tr)
	echo FILE $(GLOBJ)gscdefs.obj >>$(ll_tr)

$(GLOBJ)gslib.exe: $(LIB_ALL) $(LIB_ONLY) $(ld_tr) $(ll_tr)
	$(LINK) $(LCT) NAME gslib OPTION MAP=gslib FILE $(GLOBJ)gslib @$(ld_tr) @$(ll_tr)
