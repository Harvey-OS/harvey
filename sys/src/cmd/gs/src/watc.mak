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

# $Id: watc.mak,v 1.2 2000/03/10 15:48:58 lpd Exp $
# makefile for MS-DOS/Watcom C386 platform.
# We strongly recommend that you read the Watcom section of Make.htm
# before attempting to build Ghostscript with the Watcom compiler.

# ------------------------------- Options ------------------------------- #

###### This section is the only part of the file you should need to edit.

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

# Define the root directory for Ghostscript installation.

AROOTDIR=c:/Aladdin
GSROOTDIR=$(AROOTDIR)/gs$(GS_DOT_VERSION)

# Define the directory that will hold documentation at runtime.

GS_DOCDIR=$(GSROOTDIR)/doc

# Define the default directory/ies for the runtime
# initialization and font files.  Separate multiple directories with \;.
# Use / to indicate directories, not a single \.

GS_LIB_DEFAULT=$(GSROOTDIR)/lib\;$(AROOTDIR)/fonts

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

# Setting DEBUG=1 includes debugging features (-Z switch) in the code.
# Code runs substantially slower even if no debugging switches are set,
# and also takes about another 25K of memory.

DEBUG=0

# Setting TDEBUG=1 includes symbol table information for the Watcom debugger.
# (This option is NOT needed for using the Watcom profiler.)
# Code runs substantially slower, because some optimizations are disabled.

TDEBUG=0

# Setting NOPRIVATE=1 makes private (static) procedures and variables public,
# so they are visible to the debugger and profiler.
# No execution time or space penalty, just larger .OBJ and .EXE files.

NOPRIVATE=0

# Define the name of the executable file.

GS=gs386

# Define the name of a pre-built executable that can be invoked at build
# time.  Currently, this is only needed for compiled fonts.  The usual
# alternatives are:
#   - the standard name of Ghostscript on your system (typically `gs'):
BUILD_TIME_GS=gs386
#   - the name of the executable you are building now.  If you choose this
# option, then you must build the executable first without compiled fonts,
# and then again with compiled fonts.
#BUILD_TIME_GS=$(BINDIR)\$(GS) -I$(PSLIBDIR)

# Do not edit the next group of lines.
NUL=
DD=$(GLGENDIR)\$(NUL)
GLD=$(GLGENDIR)\$(NUL)
PSD=$(PSGENDIR)\$(NUL)

# Define the directory where the IJG JPEG library sources are stored,
# and the major version of the library that is stored there.
# You may need to change this if the IJG library version changes.
# See jpeg.mak for more information.

JSRCDIR=jpeg
JVERSION=6

# Define the directory where the PNG library sources are stored,
# and the version of the library that is stored there.
# You may need to change this if the libpng version changes.
# See libpng.mak for more information.

PSRCDIR=libpng
PVERSION=10005

# Define the directory where the zlib sources are stored.
# See zlib.mak for more information.

ZSRCDIR=zlib

# Define any other compilation flags.  Including -DA4 makes A4 paper size
# the default for most, but not, printer drivers.

CFLAGS=

# ------ Platform-specific options ------ #

# Define which version of Watcom C we are using.
# Possible values are 8.5, 9.0, 9.5, 10.0, 10.5, or 11.0.
# Unfortunately, wmake can only test identity, not compare magnitudes,
# so the version must be exactly one of those strings.
#
# 10.695 equates to version 10.6 on 95 or NT
WCVERSION=10.0

# Define the locations of the libraries.
LIBPATHS=LIBPATH $(%WATCOM)\lib386 LIBPATH $(%WATCOM)\lib386\dos

# Choose platform-specific options.

# Define the processor (CPU) type.  Options are 386,
# 485 (486SX or Cyrix 486SLC), 486 (486DX), or 586 (Pentium).
# Currently the only difference is that 486 and above assume
# the presence of a FPU, and the other processor types do not.

CPU_TYPE=386

# Define the math coprocessor (FPU) type.
# Options are -1 (optimize for no FPU), 0 (optimize for FPU present,
# but do not require a FPU), 87, 287, or 387.
# If CPU_TYPE is 486 or above, FPU_TYPE is implicitly set to 387,
# since 486DX and later processors include the equivalent of an 80387 on-chip.
# An xx87 option means that the executable will run only if a FPU
# of that type (or higher) is available: this is NOT currently checked
# at runtime.

FPU_TYPE=0

# Define the .dev module that implements thread and synchronization
# primitives for this platform.  Don't change this unless you really know
# what you're doing.

SYNC=winsync

# ---------------------------- End of options ---------------------------- #

# Define the platform name.

PLATFORM=watc_

# Define the name of the makefile -- used in dependencies.

MAKEFILE=$(GLSRCDIR)\watc.mak
TOP_MAKEFILES=$(MAKEFILE) $(GLSRCDIR)\wccommon.mak

# Define additional platform compilation flags.

PLATOPT=

!include $(GLSRCDIR)\wccommon.mak

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.
# Since we have a large address space, we include some optional features.

FEATURE_DEVS=$(PSD)psl3.dev $(PSD)pdf.dev $(PSD)dpsnext.dev $(PSD)ttfont.dev

# Choose whether to compile the .ps initialization files into the executable.
# See gs.mak for details.

COMPILE_INITS=0

# Choose whether to store band lists on files or in memory.
# The choices are 'file' or 'memory'.

BAND_LIST_STORAGE=file

# Choose which compression method to use when storing band lists in memory.
# The choices are 'lzw' or 'zlib'.  lzw is not recommended, because the
# LZW-compatible code in Ghostscript doesn't actually compress its input.

BAND_LIST_COMPRESSOR=zlib

# Choose the implementation of file I/O: 'stdio', 'fd', or 'both'.
# See gs.mak and sfxfd.c for more details.

FILE_IMPLEMENTATION=stdio

# Choose the device(s) to include.  See devs.mak for details,
# devs.mak and contrib.mak for the list of available devices.

# ****** NOTE: the PC frame buffer devices won't compile with the binnt
# ****** compiler.  If you know what needs to be changed, let us know!
# ****** Probably it's just a compiler command line switch.
#DEVICE_DEVS=$(DD)vga.dev $(DD)ega.dev $(DD)svga16.dev
DEVICE_DEVS=
#DEVICE_DEVS1=$(DD)atiw.dev $(DD)tseng.dev $(DD)tvga.dev
DEVICE_DEVS1=
DEVICE_DEVS2=
DEVICE_DEVS3=$(DD)deskjet.dev $(DD)djet500.dev $(DD)laserjet.dev $(DD)ljetplus.dev $(DD)ljet2p.dev
DEVICE_DEVS4=$(DD)cdeskjet.dev $(DD)cdjcolor.dev $(DD)cdjmono.dev $(DD)cdj550.dev
DEVICE_DEVS5=$(DD)uniprint.dev
DEVICE_DEVS6=$(DD)epson.dev $(DD)eps9high.dev $(DD)ibmpro.dev $(DD)bj10e.dev $(DD)bj200.dev $(DD)bjc600.dev $(DD)bjc800.dev
DEVICE_DEVS7=
DEVICE_DEVS8=$(DD)pcxmono.dev $(DD)pcxgray.dev $(DD)pcx16.dev $(DD)pcx256.dev $(DD)pcx24b.dev $(DD)pcxcmyk.dev
DEVICE_DEVS9=
DEVICE_DEVS10=$(DD)tiffcrle.dev $(DD)tiffg3.dev $(DD)tiffg32d.dev $(DD)tiffg4.dev $(DD)tifflzw.dev $(DD)tiffpack.dev
DEVICE_DEVS11=$(DD)bmpmono.dev $(DD)bmpgray.dev $(DD)bmp16.dev $(DD)bmp256.dev $(DD)bmp16m.dev $(DD)tiff12nc.dev $(DD)tiff24nc.dev
DEVICE_DEVS12=$(DD)psmono.dev $(DD)psgray.dev $(DD)bit.dev $(DD)bitrgb.dev $(DD)bitcmyk.dev
DEVICE_DEVS13=
DEVICE_DEVS14=$(DD)jpeg.dev $(DD)jpeggray.dev
DEVICE_DEVS15=$(DD)pdfwrite.dev
# Overflow for DEVS3,4,5,6,9
DEVICE_DEVS16=$(DD)ljet3.dev $(DD)ljet3d.dev $(DD)ljet4.dev $(DD)ljet4d.dev
DEVICE_DEVS17=$(DD)pj.dev $(DD)pjxl.dev $(DD)pjxl300.dev
DEVICE_DEVS18=
DEVICE_DEVS19=
DEVICE_DEVS20=

!include $(GLSRCDIR)\wctail.mak
!include $(GLSRCDIR)\devs.mak
!include $(GLSRCDIR)\contrib.mak
!include $(PSSRCDIR)\int.mak
!include $(PSSRCDIR)\cfonts.mak

# -------------------------------- Library -------------------------------- #

# make sure the target directories exist - use special Watcom .BEFORE
# (This is not the best way to do this, but we will have to wait until
# the makefiles get disentangled to do it better.)
.BEFORE
	@if not exist $(GLGENDIR) mkdir $(GLGENDIR)
	@if not exist $(GLOBJDIR) mkdir $(GLOBJDIR)
	@if not exist $(PSGENDIR) mkdir $(PSGENDIR)
	@if not exist $(PSOBJDIR) mkdir $(PSOBJDIR)

GLCCWIN=$(GLCC)

!include $(GLSRCDIR)\winplat.mak

# The Watcom C platform

watc_1=$(GLOBJ)gp_getnv.$(OBJ) $(GLOBJ)gp_iwatc.$(OBJ) $(GLOBJ)gp_dosfb.$(OBJ)
watc_2=$(GLOBJ)gp_mktmp.$(OBJ)
!ifeq WAT32 0
watc_3=$(GLOBJ)gp_dosfs.$(OBJ) $(GLOBJ)gp_dosfe.$(OBJ) $(GLOBJ)gp_msdos.$(OBJ)
watc_inc=
!else
watc_3=
watc_inc=$(GLD)winplat.dev
!endif
watc__=$(watc_1) $(watc_2) $(watc_3)
$(GLGEN)watc_.dev: $(watc__) $(GLD)nosync.dev $(watc_inc)
	$(SETMOD) $(GLGEN)watc_ $(watc_1)
	$(ADDMOD) $(GLGEN)watc_ $(watc_2)
	$(ADDMOD) $(GLGEN)watc_ -obj $(watc_3)
	$(ADDMOD) $(GLGEN)watc_ -include $(GLD)nosync $(watc_inc)

$(GLOBJ)gp_iwatc.$(OBJ): $(GLSRC)gp_iwatc.c $(stat__h) $(string__h)\
 $(gx_h) $(gp_h)
	$(GLCC) $(GLO_)gp_iwatc.$(OBJ) $(C_) $(GLSRC)gp_iwatc.c

$(GLOBJ)gp_mktmp.$(OBJ): $(GLSRC)gp_mktmp.c $(stat__h) $(string__h)
	$(GLCC) $(GLO_)gp_mktmp.$(OBJ) $(C_) $(GLSRC)gp_mktmp.c

# ----------------------------- Main program ------------------------------ #

BEGINFILES=*.err

LIBDOS=$(LIB_ALL) $(watc__) $(ld_tr)

# Interpreter main program

GS_ALL=$(GLOBJ)gs.$(OBJ) $(INT_ALL) $(INTASM) $(LIBDOS)

ll_tr=$(GLOBJ)ll.tr
$(ll_tr): $(TOP_MAKEFILES)
	echo OPTION STACK=64k >$(ll_tr)
!ifeq WAT32 0
	echo SYSTEM DOS4G >>$(ll_tr)
	echo OPTION STUB=$(STUB) >>$(ll_tr)
!endif

$(GS_XE): $(GS_ALL) $(DEVS_ALL) $(ll_tr)
	$(LINK) $(LCT) NAME $(GS) OPTION MAP=$(GS) FILE $(GLOBJ)gs @$(ld_tr) @$(ll_tr)
