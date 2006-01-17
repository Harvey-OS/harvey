#    Copyright (C) 1991, 1995, 1996, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: watclib.mak,v 1.24 2004/12/10 23:48:48 giles Exp $
# makefile for MS-DOS / Watcom C/C++ library testing.

libdefault: $(GLOBJ)gslib.exe
	$(NO_OP)

AROOTDIR=c:/gs
GSROOTDIR=$(AROOTDIR)/gs$(GS_DOT_VERSION)
GS_DOCDIR=$(GSROOTDIR)/doc
GS_LIB_DEFAULT=$(GSROOTDIR)/lib\;$(GSROOTDIR)/resource\;$(AROOTDIR)/fonts
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
PVERSION=10208
!endif

!ifndef ZSRCDIR
ZSRCDIR=zlib
!endif

# Define the jbig2dec library source location.
# See jbig2.mak for more information.

!ifndef JBIG2SRCDIR
JBIG2SRCDIR=jbig2dec
!endif

# Define the directory where the icclib source are stored.
# See icclib.mak for more information

!ifndef ICCSRCDIR
ICCSRCDIR=icclib
!endif

# Define the directory where the ijs source is stored,
# and the process forking method to use for the server.
# See ijs.mak for more information.

!ifndef IJSSRCDIR
IJSSRCDIR=ijs
IJSEXECTYPE=win
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
FEATURE_DEVS=$(GLD)patlib.dev $(GLD)path1lib.dev
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
!ifndef STDIO_IMPLEMENTATION
STDIO_IMPLEMENTATION=
!endif

!include $(GLSRCDIR)\wctail.mak
!include $(GLSRCDIR)\devs.mak
!include $(GLSRCDIR)\contrib.mak

GLCCWIN=$(GLCC)
!include $(GLSRCDIR)\winplat.mak

watclib_1=$(GLOBJ)gp_getnv.$(OBJ) $(GLOBJ)gp_iwatc.$(OBJ)
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
 $(gx_h) $(gp_h) $(gpmisc_h)
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
