#    Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: os2.mak,v 1.47 2005/08/31 05:52:32 ray Exp $
# makefile for MS-DOS or OS/2 GCC/EMX platform.
# Uses Borland (MSDOS) MAKER or 
# Uses IBM NMAKE.EXE Version 2.000.000 Mar 27 1992

# ------------------------------- Options ------------------------------- #

###### This section is the only part of the file you should need to edit.

# ------ Generic options ------ #

# Define the directory for the final executable, and the
# source, generated intermediate file, and object directories
# for the graphics library (GL) and the PostScript/PDF interpreter (PS).

# This makefile has never been tested with any other values than these,
# and almost certainly won't work with other values.
BINDIR=bin
GLSRCDIR=src
GLGENDIR=obj
GLOBJDIR=obj
PSSRCDIR=src
PSLIBDIR=lib
PSGENDIR=obj
PSOBJDIR=obj

# Define the root directory for Ghostscript installation.

AROOTDIR=c:/gs
GSROOTDIR=$(AROOTDIR)/gs$(GS_DOT_VERSION)

# Define the directory that will hold documentation at runtime.

GS_DOCDIR=$(GSROOTDIR)/doc

# Define the default directory/ies for the runtime
# initialization, resource and font files.  Separate multiple directories with ;.
# Use / to indicate directories, not a single \.

GS_LIB_DEFAULT=$(GSROOTDIR)/lib;$(GSROOTDIR)/Resource;$(AROOTDIR)/fonts

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

# Setting GDEBUG=1 includes symbol table information for GDB.
# Produces larger .OBJ and .EXE files.

GDEBUG=0

# Setting NOPRIVATE=1 makes private (static) procedures and variables public,
# so they are visible to the debugger and profiler.
# No execution time or space penalty, just larger .OBJ and .EXE files.

NOPRIVATE=0

# Setting MAKEDLL=1 makes the target a DLL instead of an EXE
MAKEDLL=1

# Setting EMX=1 uses GCC/EMX
# Setting IBMCPP=1 uses IBM C++
EMX=1
IBMCPP=0

# Setting BUILD_X11=1 builds X11 client using Xfree86
BUILD_X11=0
!if $(BUILD_X11)
X11INCLUDE=-I$(X11ROOT)\XFree86\include
X11LIBS=$(X11ROOT)\XFree86\lib\Xt.lib $(X11ROOT)\XFree86\lib\X11.lib 
MT_OPT=-Zmtd
!endif

# Define the name of the executable file.

GS=gsos2
GSDLL=gsdll2

# Define the name of a pre-built executable that can be invoked at build
# time.  Currently, this is only needed for compiled fonts.  The usual
# alternatives are:
#   - the standard name of Ghostscript on your system (typically `gs'):
BUILD_TIME_GS=gsos2
#   - the name of the executable you are building now.  If you choose this
# option, then you must build the executable first without compiled fonts,
# and then again with compiled fonts.
#BUILD_TIME_GS=$(BINDIR)\$(GS) -I$(PSLIBDIR)

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
PVERSION=10208

# Define the directory where the zlib sources are stored.
# See zlib.mak for more information.

ZSRCDIR=zlib

# Define the jbig2dec library source location.
# See jbig2.mak for more information.

JBIG2SRCDIR=jbig2dec

# Define the directory where the icclib source are stored.
# See icclib.mak for more information

ICCSRCDIR=icclib

# IJS has not been ported to OS/2. If you do the port,
# you'll need to set these values. You'll also need to
# include the ijs.mak makefile (right after icclib.mak).
#
# Define the directory where the ijs source is stored,
# and the process forking method to use for the server.
# See ijs.mak for more information.

#IJSSRCDIR=ijs
#IJSEXECTYPE=win

# 1 --> Use 64 bits for gx_color_index.  This is required only for
# non standard devices or DeviceN process color model devices.
USE_LARGE_COLOR_INDEX=1

!if $(USE_LARGE_COLOR_INDEX) == 1
# Definitions to force gx_color_index to 64 bits
LARGEST_UINTEGER_TYPE=unsigned long long
GX_COLOR_INDEX_TYPE=$(LARGEST_UINTEGER_TYPE)
GCIFLAGS=-DGX_COLOR_INDEX_TYPE="$(GX_COLOR_INDEX_TYPE)"
!else
GCIFLAGS=
!endif


# The following is a hack to get around the special treatment of \ at
# the end of a line.
NUL=
DD=$(GLGENDIR)\$(NUL)
GLD=$(GLGENDIR)\$(NUL)
PSD=$(PSGENDIR)\$(NUL)


# ------ Platform-specific options ------ #

# If you don't have an assembler, set USE_ASM=0.  Otherwise, set USE_ASM=1,
# and set ASM to the name of the assembler you are using.  This can be
# a full path name if you want.  Normally it will be masm or tasm.

USE_ASM=0
ASM= 

# Define the drive, directory, and compiler name for the EMX files.
# COMP is the compiler name (gcc)
# COMPDIR contains the compiler and linker (normally \emx\bin).
# EMXPATH contains the path to the EMX directory (normally /emx)
# INCDIR contains the include files (normally /emx/include).
# LIBDIR contains the library files (normally /emx/lib).
# Note that these prefixes are always followed by a \,
#   so if you want to use the current directory, use an explicit '.'.

!if $(EMX)
COMP=gcc $(X11INCLUDE)
COMPBASE=\emx
EMXPATH=/emx
COMPDIR=$(COMPBASE)\bin
INCDIR=$(EMXPATH)/include
LIBDIR=$(EMXPATH)/lib
!if $(PVERSION) >= 10009
CPNG=-DPNGAPI=
!endif
!endif

!if $(IBMCPP)
COMP=icc /Q
COMPBASE=\ibmcpp
TOOLPATH=\toolkit
COMPDIR=$(COMPBASE)\bin
INCDIR=$(TOOLPATH)\h;$(COMPBASE)\include
LIBDIR=$(TOOLPATH)\lib;$(COMPBASE)\lib
!endif

# Choose platform-specific options.

# Define the processor (CPU) type.  Options are 86 (8086 or 8088),
# 186, 286, 386, 485 (486SX or Cyrix 486SLC), 486 (486DX), or 586 (Pentium).
# Higher numbers produce code that may be significantly smaller and faster,
# but the executable will bail out with an error message on any processor
# less capable than the designated one.

# EMX requires 386 or higher
CPU_TYPE=386

# Define the math coprocessor (FPU) type.
# Options are -1 (optimize for no FPU), 0 (optimize for FPU present,
# but do not require a FPU), 87, 287, or 387.
# If CPU_TYPE is 486 or above, FPU_TYPE is implicitly set to 387,
# since 486DX and later processors include the equivalent of an 80387 on-chip.
# An xx87 option means that the executable will run only if a FPU
# of that type (or higher) is available: this is NOT currently checked
# at runtime.

FPU_TYPE=387

# Define the .dev module that implements thread and synchronization
# primitives for this platform.  Don't change this unless you really know
# what you're doing.

SYNC=winsync

# ---------------------------- End of options ---------------------------- #

# Note that built-in libpng and zlib aren't available.

SHARE_JPEG=0
SHARE_LIBPNG=0
SHARE_ZLIB=0
SHARE_JBIG2=0

# Swapping `make' out of memory makes linking much faster.
# only used by Borland MAKER.EXE

#.swap

# Define the platform name.

PLATFORM=os2_

# Define the name of the makefile -- used in dependencies.

MAKEFILE=$(PSSRCDIR)\os2.mak
TOP_MAKEFILES=$(MAKEFILE)

# Define the files to be deleted by 'make clean'.

BEGINFILES=$(BINDIR)\gspmdrv.exe $(GLOBJDIR)\gspmdrv.o $(GLOBJDIR)\gs*.res $(GLOBJDIR)\gs*.ico $(BINDIR)\$(GSDLL).dll

# Define the auxiliary program dependency. We don't use this.

AK=

#Compiler Optimiser option
!if $(EMX)
CO=-O
!endif
!if $(IBMCPP)
#CO=/O+
CO=/O-
!endif

# Make sure we get the right default target for make.

dosdefault: default $(BINDIR)\gspmdrv.exe

# Define a rule for invoking just the preprocessor.

.c.i:
	$(COMPDIR)\cpp $(CCFLAGS) $<

# Define the extensions for command, object, and executable files.

# Work around the fact that some `make' programs drop trailing spaces
# or interpret == as a special definition operator.
NULL=

CMD=.cmd
C_=-c
D_=-D
_D_=$(NULL)=
_D=
I_=-I
II=-I
_I=
O_=-o $(NULL)
!if $(MAKEDLL)
OBJ=obj
!else
OBJ=o
!endif
Q=
XE=.exe
XEAUX=.exe

# Define the current directory prefix and shell name.

D=\#

EXP=
SH=

# Define generic commands.

# We use cp.cmd rather than copy /B so that we update the write date.
CP_=$(GLSRCDIR)\cp.cmd
# We use rm.cmd rather than erase because rm.cmd never generates
# a non-zero return code.
RM_=$(GLSRCDIR)\rm.cmd
# OS/2 erase, unlike MS-DOS erase, accepts multiple files or patterns.
RMN_=$(GLSRCDIR)\rm.cmd

# Define the arguments for genconf.

!if $(MAKEDLL)
CONFILES=-p %%s+
!else
CONFILES=
!endif
CONFLDTR=-ol

# Define the generic compilation flags.

!if $(CPU_TYPE) >= 486
ASMCPU=/DFOR80386 /DFOR80486
PLATOPT=-DFOR80386 -DFOR80486
!else
!if $(CPU_TYPE) >= 386
ASMCPU=/DFOR80386
PLATOPT=-DFOR80386
!endif
!endif

!if $(FPU_TYPE) > 0
ASMFPU=/DFORFPU
!else
ASMFPU=
!endif

!if $(USE_ASM)
INTASM=iutilasm.$(OBJ)
PCFBASM=gdevegaa.$(OBJ)
!else
INTASM=
PCFBASM=
!endif

# Define the generic compilation rules.

ASMFLAGS=$(ASMCPU) $(ASMFPU) $(ASMDEBUG)

.asm.o:
	$(ASM) $(ASMFLAGS) $<;

# ---------------------- MS-DOS I/O debugging option ---------------------- #

dosio_=$(PSOBJ)zdosio.$(OBJ)
dosio.dev: $(PSGEN)dosio.dev
	$(NO_OP)

$(PSGEN)dosio.dev: $(dosio_)
	$(SETMOD) $(PSGEN)dosio $(dosio_)
	$(ADDMOD) $(PSGEN)dosio -oper zdosio

$(PSOBJ)zdosio.$(OBJ): $(PSSRC)zdosio.c $(OP) $(store_h)
	$(PSCC) $(PSO_)zdosio.$(OBJ) $(C_) $(PSSRC)zdosio.c

# ----------------------------- Assembly code ----------------------------- #

$(PSOBJ)iutilasm.$(OBJ): $(PSSRC)iutilasm.asm

#################  END

# Define the compilation flags.

!if $(NOPRIVATE)
CP=-DNOPRIVATE
!else
CP=
!endif

!if $(DEBUG)
CD=-DDEBUG
!else
CD=
!endif

!if $(GDEBUG)
!if $(EMX)
CGDB=-g
!endif
!if $(IBMCPP)
CGDB=/Ti+
!endif
!else
CGDB=
!endif

!if $(MAKEDLL)
!if $(EMX)
CDLL=-Zdll -Zso -Zsys -Zomf $(MT_OPT) -D__DLL__
!endif
!if $(IBMCPP)
CDLL=/Gd- /Ge- /Gm+ /Gs+ /D__DLL__
!endif
!else
CDLL=
!endif

!if $(EMX)
CEXE=-Zomf
!endif

GENOPT=$(CP) $(CD) $(CGDB) $(CDLL) $(CO) $(CPNG)

CCFLAGS0=$(GENOPT) $(PLATOPT) -D__OS2__ $(GCIFLAGS)
CCFLAGS=$(CCFLAGS0) 
CC=$(COMPDIR)\$(COMP) $(CCFLAGS0)
CC_=$(CC)
CC_D=$(CC) $(CO)
CC_INT=$(CC)
CC_NO_WARN=$(CC_)

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.
# Since we have a large address space, we include some optional features.

FEATURE_DEVS=$(PSD)psl3.dev $(PSD)pdf.dev $(PSD)dpsnext.dev $(PSD)ttfont.dev $(PSD)epsf.dev $(PSD)os2print.dev

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
# devs.mak, pcwin.mak, and contrib.mak for the list of available devices.

!if $(MAKEDLL)
DEVICE_DEVS=$(DD)display.dev $(DD)os2pm.dev $(DD)os2dll.dev $(DD)os2prn.dev
!else
DEVICE_DEVS=$(DD)os2pm.dev
!endif
!if $(BUILD_X11)
DEVICE_DEVS1=$(DD)x11.dev $(DD)x11alpha.dev $(DD)x11cmyk.dev $(DD)x11gray2.dev $(DD)x11gray4.dev $(DD)x11mono.dev
!else
DEVICE_DEVS1=
!endif
DEVICE_DEVS2=$(DD)epson.dev $(DD)eps9high.dev $(DD)eps9mid.dev $(DD)epsonc.dev $(DD)ibmpro.dev
DEVICE_DEVS3=$(DD)deskjet.dev $(DD)djet500.dev $(DD)laserjet.dev $(DD)ljetplus.dev $(DD)ljet2p.dev
DEVICE_DEVS4=$(DD)cdeskjet.dev $(DD)cdjcolor.dev $(DD)cdjmono.dev $(DD)cdj550.dev
DEVICE_DEVS5=$(DD)uniprint.dev $(DD)djet500c.dev $(DD)declj250.dev $(DD)lj250.dev
DEVICE_DEVS6=$(DD)st800.dev $(DD)stcolor.dev $(DD)bj10e.dev $(DD)bj200.dev
DEVICE_DEVS7=$(DD)t4693d2.dev $(DD)t4693d4.dev $(DD)t4693d8.dev $(DD)tek4696.dev
DEVICE_DEVS8=$(DD)pcxmono.dev $(DD)pcxgray.dev $(DD)pcx16.dev $(DD)pcx256.dev $(DD)pcx24b.dev $(DD)pcxcmyk.dev
DEVICE_DEVS9=$(DD)pbm.dev $(DD)pbmraw.dev $(DD)pgm.dev $(DD)pgmraw.dev $(DD)pgnm.dev $(DD)pgnmraw.dev $(DD)pkmraw.dev
DEVICE_DEVS10=$(DD)tiffcrle.dev $(DD)tiffg3.dev $(DD)tiffg32d.dev $(DD)tiffg4.dev $(DD)tifflzw.dev $(DD)tiffpack.dev
DEVICE_DEVS11=$(DD)bmpmono.dev $(DD)bmpgray.dev $(DD)bmp16.dev $(DD)bmp256.dev $(DD)bmp16m.dev $(DD)tiff12nc.dev $(DD)tiff24nc.dev $(DD)tiffgray.dev $(DD)tiff32nc.dev $(DD)tiffsep.dev
DEVICE_DEVS12=$(DD)psmono.dev $(DD)bit.dev $(DD)bitrgb.dev $(DD)bitcmyk.dev
DEVICE_DEVS13=$(DD)pngmono.dev $(DD)pnggray.dev $(DD)png16.dev $(DD)png256.dev $(DD)png16m.dev $(DD)pngalpha.dev
DEVICE_DEVS14=$(DD)jpeg.dev $(DD)jpeggray.dev $(DD)jpegcmyk.dev
DEVICE_DEVS15=$(DD)pdfwrite.dev $(DD)pswrite.dev $(DD)ps2write.dev $(DD)epswrite.dev $(DD)pxlmono.dev $(DD)pxlcolor.dev
DEVICE_DEVS16=$(DD)bbox.dev
# Overflow for DEVS3,4,5,6,9
DEVICE_DEVS17=$(DD)ljet3.dev $(DD)ljet3d.dev $(DD)ljet4.dev $(DD)ljet4d.dev 
DEVICE_DEVS18=$(DD)pj.dev $(DD)pjxl.dev $(DD)pjxl300.dev $(DD)jetp3852.dev $(DD)r4081.dev
DEVICE_DEVS19=$(DD)lbp8.dev $(DD)m8510.dev $(DD)necp6.dev $(DD)bjc600.dev $(DD)bjc800.dev
DEVICE_DEVS20=$(DD)pnm.dev $(DD)pnmraw.dev $(DD)ppm.dev $(DD)ppmraw.dev
DEVICE_DEVS21= $(DD)spotcmyk.dev $(DD)devicen.dev $(DD)bmpsep1.dev $(DD)bmpsep8.dev $(DD)bmp16m.dev $(DD)bmp32b.dev $(DD)psdcmyk.dev $(DD)psdrgb.dev

# Include the generic makefiles.
!include "$(GLSRCDIR)\version.mak"
!include "$(GLSRCDIR)\gs.mak"
!include "$(GLSRCDIR)\lib.mak"
!include "$(GLSRCDIR)\jpeg.mak"
# zlib.mak must precede libpng.mak
!include "$(GLSRCDIR)\zlib.mak"
!include "$(GLSRCDIR)\libpng.mak"
!include "$(GLSRCDIR)\jbig2.mak"
!include "$(GLSRCDIR)\icclib.mak"
!include "$(GLSRCDIR)\devs.mak"
!include "$(GLSRCDIR)\pcwin.mak"
!include "$(GLSRCDIR)\contrib.mak"
!include "$(PSSRCDIR)\int.mak"
!include "$(PSSRCDIR)\cfonts.mak"

# -------------------------------- Library -------------------------------- #

# The GCC/EMX platform

os2__=$(GLOBJ)gp_getnv.$(OBJ) $(GLOBJ)gp_os2.$(OBJ) $(GLOBJ)gp_stdia.$(OBJ)
$(GLGEN)os2_.dev: $(os2__) $(GLD)nosync.dev
	$(SETMOD) $(GLGEN)os2_ $(os2__) -include $(GLD)nosync

$(GLOBJ)gp_os2.$(OBJ): $(GLSRC)gp_os2.c $(GLSRC)gp_os2.h\
 $(dos__h) $(pipe__h) $(string__h) $(time__h)\
 $(gsdll_h) $(gx_h) $(gsexit_h) $(gsutil_h) $(gp_h) $(gpmisc_h)
	$(GLCC) $(GLO_)gp_os2.$(OBJ) $(C_) $(GLSRC)gp_os2.c

$(GLOBJ)gp_stdia.$(OBJ): $(GLSRC)gp_stdia.c $(AK)\
  $(stdio__h) $(time__h) $(unistd__h) $(gx_h) $(gp_h)
	$(GLCC) $(GLO_)gp_stdia.$(OBJ) $(C_) $(GLSRC)gp_stdia.c

# Define OS/2 printer (file system) as a separable feature.

os2print_=$(GLOBJ)gp_os2pr.$(OBJ)
$(GLD)os2print.dev: $(ECHOGS_XE) $(os2print_)
	$(SETMOD) $(GLD)os2print $(os2print_)
	$(ADDMOD) $(GLD)os2print -iodev printer

$(GLOBJ)gp_os2pr.$(OBJ): $(GLSRC)gp_os2pr.c $(GLSRC)gp_os2.h $(AK)\
 $(ctype__h) $(errno__h) $(stdio__h) $(string__h)\
 $(gserror_h) $(gsmemory_h) $(gstypes_h) $(gxiodev_h)
	$(GLCC) $(GLO_)gp_os2pr.$(OBJ) $(C_) $(GLSRC)gp_os2pr.c


# -------------------------- Auxiliary programs --------------------------- #

#CCAUX=$(COMPDIR)\$(COMP) $(CO)
# emx 0.9d (gcc 2.8.1) crashes when compiling genarch.c with optimizer
CCAUX=$(COMPDIR)\$(COMP)

$(ECHOGS_XE): $(GLSRCDIR)\echogs.c
!if $(EMX)
	$(CCAUX) -o $(AUXGEN)echogs $(GLSRCDIR)\echogs.c
	$(COMPDIR)\emxbind $(EMXPATH)/bin/emxl.exe $(AUXGEN)echogs $(ECHOGS_XE)
	del $(AUXGEN)echogs
!endif
!if $(IBMCPP)
	$(CCAUX) /Fe$(ECHOGS_XE) $(GLSRCDIR)\echogs.c
!endif

$(GENARCH_XE): $(GLSRCDIR)\genarch.c $(GENARCH_DEPS)
	-mkdir $(GLGENDIR)
	-mkdir $(BINDIR)
!if $(EMX)
	$(CCAUX) -DHAVE_LONG_LONG -o $(AUXGEN)genarch $(GLSRCDIR)\genarch.c
	$(COMPDIR)\emxbind $(EMXPATH)/bin/emxl.exe $(AUXGEN)genarch $(GENARCH_XE)
	del $(AUXGEN)genarch
!endif
!if $(IBMCPP)
	$(CCAUX) /Fe$(GENARCH_XE) $(GLSRCDIR)\genarch.c
!endif

$(GENCONF_XE): $(GLSRCDIR)\genconf.c $(GENCONF_DEPS)
!if $(EMX)
	$(CCAUX) -o $(AUXGEN)genconf $(GLSRCDIR)\genconf.c
	$(COMPDIR)\emxbind $(EMXPATH)/bin/emxl.exe $(AUXGEN)genconf $(GENCONF_XE)
	del $(AUXGEN)genconf
!endif
!if $(IBMCPP)
	$(CCAUX) /Fe$(GENCONF_XE) $(GLSRCDIR)\genconf.c
!endif

$(GENDEV_XE): $(GLSRCDIR)\gendev.c $(GENDEV_DEPS)
!if $(EMX)
	$(CCAUX) -o $(AUXGEN)gendev $(GLSRCDIR)\gendev.c
	$(COMPDIR)\emxbind $(EMXPATH)/bin/emxl.exe $(AUXGEN)gendev $(GENDEV_XE)
	del $(AUXGEN)gendev
!endif
!if $(IBMCPP)
	$(CCAUX) /Fe$(GENDEV_XE) $(GLSRCDIR)\gendev.c
!endif

$(GENHT_XE): $(PSSRC)genht.c $(GENHT_DEPS)
!if $(EMX)
	$(CCAUX) -o $(AUXGEN)genht $(GENHT_CFLAGS) $(PSSRC)genht.c
	$(COMPDIR)\emxbind $(EMXPATH)/bin/emxl.exe $(AUXGEN)genht $(GENHT_XE)
	del $(AUXGEN)genht
!endif
!if $(IBMCPP)
	$(CCAUX) /Fe$(GENHT_XE) genht.c
!endif

$(GENINIT_XE): $(PSSRC)geninit.c $(GENINIT_DEPS)
!if $(EMX)
	$(CCAUX) -o $(AUXGEN)geninit $(PSSRC)geninit.c
	$(COMPDIR)\emxbind $(EMXPATH)/bin/emxl.exe $(AUXGEN)geninit $(GENINIT_XE)
	del $(AUXGEN)geninit
!endif
!if $(IBMCPP)
	$(CCAUX) /Fe$(GENINIT_XE) geninit.c
!endif

# No special gconfig_.h is needed.
$(gconfig__h): $(TOP_MAKEFILES) $(ECHOGS_XE)
	$(ECHOGS_XE) -w $(gconfig__h) /* This file deliberately left blank. */

$(gconfigv_h): $(PSSRCDIR)\os2.mak $(TOP_MAKEFILES) $(ECHOGS_XE)
	$(ECHOGS_XE) -w $(gconfigv_h) -x 23 define USE_ASM -x 2028 -q $(USE_ASM)-0 -x 29
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define USE_FPU -x 2028 -q $(FPU_TYPE)-0 -x 29
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define EXTEND_NAMES 0$(EXTEND_NAMES)
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define SYSTEM_CONSTANTS_ARE_WRITABLE 0$(SYSTEM_CONSTANTS_ARE_WRITABLE)

# ----------------------------- Main program ------------------------------ #

gsdllos2_h=$(GLSRC)gsdllos2.h

# Interpreter main program

ICONS=$(PSOBJ)gsos2.ico $(GLOBJ)gspmdrv.ico

$(PSOBJ)dpmain.$(OBJ): $(PSSRC)dpmain.c $(AK)\
 $(gdevdsp_h) $(iapi_h) $(gscdefs_h) $(ierrors_h)
	$(CC) $(CEXE) -I$(PSSRCDIR) -I$(GLSRCDIR) -I$(GLGENDIR) $(PSO_)dpmain.$(OBJ) $(C_) $(PSSRC)dpmain.c

!if $(MAKEDLL)
#making a DLL
GS_ALL=$(PSOBJ)gsdll.$(OBJ) $(INT_ALL) $(INTASM)\
  $(LIB_ALL) $(LIBCTR) $(ld_tr) $(PSOBJ)$(GS).res $(ICONS)

$(GS_XE): $(BINDIR)\$(GSDLL).dll $(PSSRC)dpmain.c $(gsdll_h) $(gsdllos2_h) $(PSSRC)gsos2.rc $(GLOBJ)gscdefs.$(OBJ)
!if $(EMX)
	$(COMPDIR)\$(COMP) $(CGDB) $(CO) -Zomf $(MT_OPT) -I$(PSSRCDIR) -I$(GLSRCDIR) -I$(PSOBJDIR) -I$(GLOBJDIR) -o$(GS_XE) $(PSSRC)dpmain.c $(GLOBJ)gscdefs.$(OBJ) $(PSSRC)gsos2.def
!endif
!if $(IBMCPP)
	$(CCAUX) -I$(PSSRCDIR) -I$(GLSRCDIR) -I$(PSOBJDIR) -I$(GLOBJDIR) /Fe$(GX_XE) $(PSSRC)dpmain.c $(GLOBJ)gscdefs.$(OBJ)
!endif
	rc $(PSOBJ)$(GS).res $(GS_XE)

$(PSOBJ)gsdll.$(OBJ): $(PSSRC)gsdll.c $(gsdll_h) $(ghost_h) $(gscdefs_h)
	$(PSCC) $(PSO_)gsdll.$(OBJ) $(C_) $(PSSRC)gsdll.c

$(BINDIR)\$(GSDLL).dll: $(GS_ALL) $(ALL_DEVS) $(PSOBJ)gsdll.$(OBJ)
!if $(EMX)
	LINK386 /DEBUG $(COMPBASE)\lib\dll0.obj $(COMPBASE)\lib\end.lib @$(ld_tr) $(PSOBJ)gsdll.obj, $(BINDIR)\$(GSDLL).dll, ,$(X11LIBS) $(COMPBASE)\lib\gcc.lib $(COMPBASE)\lib\st\c.lib $(COMPBASE)\lib\st\c_dllso.lib $(COMPBASE)\lib\st\sys.lib $(COMPBASE)\lib\c_alias.lib $(COMPBASE)\lib\os2.lib, $(PSSRC)gsdll2.def
!endif
!if $(IBMCPP)
	LINK386 /NOE /DEBUG @$(ld_tr) $(PSOBJ)gsdll.obj, $(BINDIR)\$(GSDLL).dll, , , $(PSSRC)gsdll2.def
!endif

!else
#making an EXE
GS_ALL=$(PSOBJ)gs.$(OBJ) $(INT_ALL) $(INTASM)\
  $(LIB_ALL) $(LIBCTR) $(ld_tr) $(PSOBJ)$(GS).res $(ICONS)

$(GS_XE): $(GS_ALL) $(ALL_DEVS)
	$(COMPDIR)\$(COMP) $(CGDB) I$(PSSRCDIR) -I$(GLSRCDIR) -o $(PSOBJ)$(GS) $(PSOBJ)gs.$(OBJ) @$(ld_tr) $(INTASM) -lm
	$(COMPDIR)\emxbind -r$(PSOBJ)$(GS).res $(COMPDIR)\emxl.exe $(PSOBJ)$(GS) $(GS_XE) -ac
	del $(PSOBJ)$(GS)
!endif

# Make the icons from their text form.

$(PSOBJ)gsos2.ico: $(PSSRC)gsos2.icx $(ECHOGS_XE)
	$(ECHOGS_XE) -wb $(PSOBJ)gsos2.ico -n -X -r $(PSSRC)gsos2.icx

$(GLOBJ)gspmdrv.ico: $(GLSRC)gspmdrv.icx $(ECHOGS_XE)
	$(ECHOGS_XE) -wb $(GLOBJ)gspmdrv.ico -n -X -r $(GLSRC)gspmdrv.icx

$(PSOBJ)$(GS).res: $(PSSRC)$(GS).rc $(PSOBJ)gsos2.ico
	rc -i $(COMPBASE)\include -i $(PSSRCDIR) -i $(PSOBJDIR) -r $(PSSRC)$(GS).rc $(PSOBJ)$(GS).res
	

# PM driver program

$(GLOBJ)gspmdrv.o: $(GLSRC)gspmdrv.c $(GLSRC)gspmdrv.h
	$(COMPDIR)\$(COMP) $(CGDB) $(CO) -I$(GLSRCDIR) -o $(GLOBJ)gspmdrv.o -c $(GLSRC)gspmdrv.c

$(GLOBJ)gspmdrv.res: $(GLSRC)gspmdrv.rc $(GLSRC)gspmdrv.h $(GLOBJ)gspmdrv.ico
	rc -i $(COMPBASE)\include -i $(GLSRCDIR) -i $(GLOBJDIR) -r $(GLSRC)gspmdrv.rc $(GLOBJ)gspmdrv.res

$(BINDIR)\gspmdrv.exe: $(GLOBJ)gspmdrv.o $(GLOBJ)gspmdrv.res $(GLSRC)gspmdrv.def
	$(COMPDIR)\$(COMP) $(CGDB) $(CO) -o $(GLOBJ)gspmdrv $(GLOBJ)gspmdrv.o 
	$(COMPDIR)\emxbind -p -r$(GLOBJ)gspmdrv.res -d$(GLSRC)gspmdrv.def $(COMPDIR)\emxl.exe $(GLOBJ)gspmdrv $(BINDIR)\gspmdrv.exe
	del $(GLOBJ)gspmdrv

# Create a ZIP archive
# This assumes that the current directory is named gs#.## relative to its
# parent, where #.## is the Ghostscript version.

ZIP_XE=zip
ZIPPROGFILE1=gs$(GS_DOT_VERSION)\bin\gsos2.exe
ZIPPROGFILE2=gs$(GS_DOT_VERSION)\bin\gsdll2.dll
ZIPPROGFILE3=gs$(GS_DOT_VERSION)\bin\gspmdrv.exe
ZIPPROGFILE4=gs$(GS_DOT_VERSION)\doc
ZIPPROGFILE5=gs$(GS_DOT_VERSION)\examples
ZIPPROGFILE6=gs$(GS_DOT_VERSION)\lib
ZIPFONTDIR=fonts

# Make the zip archive.
zip:
	cd ..
	-del gs$(GS_VERSION)os2.zip
	$(ZIP_XE) -9 -r gs$(GS_VERSION)os2.zip $(ZIPFONTDIR)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)os2.zip $(ZIPPROGFILE1)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)os2.zip $(ZIPPROGFILE2)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)os2.zip $(ZIPPROGFILE3)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)os2.zip $(ZIPPROGFILE4)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)os2.zip $(ZIPPROGFILE5)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)os2.zip $(ZIPPROGFILE6)
	cd gs$(GS_DOT_VERSION)
