#    Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
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

# makefile for (MS-Windows 3.1 / Win32s / Windows NT) +
#   Borland C++ 4.0 platform.

# ------------------------------- Options ------------------------------- #

###### This section is the only part of the file you should need to edit.

# ------ Generic options ------ #

# Define the directory that will hold documentation at runtime.

GS_DOCDIR=c:/gs

# Define the default directory/ies for the runtime
# initialization and font files.  Separate multiple directories with \;.
# Use / to indicate directories, not a single \.

GS_LIB_DEFAULT=.;c:/gs\;c:/gs/fonts

# Define the name of the interpreter initialization file.
# (There is no reason to change this.)

GS_INIT=gs_init.ps

# Choose generic configuration options.

# Setting DEBUG=1 includes debugging features (-Z switch) in the code.
# Code runs substantially slower even if no debugging switches are set,
# and also takes about another 25K of memory.

DEBUG=0

# Setting TDEBUG=1 includes symbol table information for the Borland debugger.
# No execution time or space penalty, just larger .OBJ and .EXE files
# and slower linking.

TDEBUG=0

# Setting NOPRIVATE=1 makes private (static) procedures and variables public,
# so they are visible to the debugger and profiler.
# No execution time or space penalty, just larger .OBJ and .EXE files.

NOPRIVATE=0

# Setting GSDLL=1 makes a Dynamic Link Library
GSDLL=0

# Define the name of the executable file.

!if $(GSDLL)
GS=gsdll32
!else
GS=gswin32
!endif

# Define the directory where the IJG JPEG library sources are stored.
# You may have to change this if the IJG library version changes.
# See jpeg.mak for more information.

JSRCDIR=jpeg-5

# Define any other compilation flags.  Including -DA4 makes A4 paper size
# the default for most, but not, printer drivers.

CFLAGS=

# ------ Platform-specific options ------ #

# If you don't have an assembler, set USE_ASM=0.  Otherwise, set USE_ASM=1,
# and set ASM to the name of the assembler you are using.  This can be
# a full path name if you want.  Normally it will be masm or tasm.

USE_ASM=0
ASM=tasm

# Define the drive, directory, and compiler name for the Turbo C files.
# COMP is the compiler name (bcc32 for Borland C++).
# COMPAUX is the compiler name for DOS utilities (bcc for Borland C++).
# COMPDIR contains the compiler and linker (normally \bc4\bin).
# INCDIR contains the include files (normally \bc4\include).
# LIBDIR contains the library files (normally \bc4\lib).
# Note that these prefixes are always followed by a \,
#   so if you want to use the current directory, use an explicit '.'.

COMP=bcc32
COMPAUX=bcc
COMPBASE=d:\bc4
COMPDIR=$(COMPBASE)\bin
INCDIR=$(COMPBASE)\include
LIBDIR=$(COMPBASE)\lib

# Define the Windows directory.

WINDIR=c:\windows

# Windows 32s requires a 386 CPU or higher.

CPU_TYPE=386

# Don't rely on FPU for Windows: Options are -1 (optimize for no FPU) or
# 0 (optimize for FPU present, but do not require a FPU).

FPU_TYPE=0

# ---------------------------- End of options ---------------------------- #

# Swapping `make' out of memory makes linking much faster.

.swap

# Define the platform name.

PLATFORM=bcwin32_

# Define the name of the makefile -- used in dependencies.

MAKEFILE=bcwin32.mak

# Define the ANSI-to-K&R dependency.  Turbo C accepts ANSI syntax,
# but we need to preconstruct ccf32.tr to get around the limit on
# the maximum length of a command line.

AK=ccf32.tr

# Define the compilation flags for an 80286.
# This is only needed because it is used in tccommon.mak.

F286=

# Define compiler switches appropriate to this version of Borland C++.
# We know we are running C++ 4.0 or later, because earlier versions
# don't support Windows 32s.

# -Obe is causing bcc32.exe to crash on gsimage2.c
#CO=-Obe -Z	
CO=-Z
CAOPT=-a1
WINCOMP=1

!include "tccommon.mak"

# Define the compilation flags.

!if $(NOPRIVATE)
CP=-DNOPRIVATE
!else
CP=
!endif

!if $(DEBUG) | $(TDEBUG)
CS=-N
!else
CS=
!endif

!if $(DEBUG)
CD=-DDEBUG
!else
CD=
!endif

!if $(TDEBUG)
CT=-v
LCT=-v -m -s
!else
CT=
LCT=-m
!endif

GENOPT=$(CP) $(CS) $(CD) $(CT)

CCFLAGS0=$(GENOPT) $(PLATOPT) $(FPFLAGS) $(CFLAGS) $(XCFLAGS)
CCFLAGS=$(CCFLAGS0)
CC=$(COMPDIR)\$(COMP) @ccf32.tr
!if $(GSDLL)
WX=-WDE
!else
# We want a Windows executable with only selected functions exported:
WX=-WE
!endif
CCC=$(CC) $(WX) $(CO) -c
CCD=$(CC) $(WX) -O -c
CCCF=$(CCC)
CCINT=$(CC) $(WX) -c

.c.obj:
	$(CCC) { $<}

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.
# Since we only support running in enhanced mode, which provides
# virtual memory, we include a fair number of drivers, but we still can't
# exceed 64K of static data.

FEATURE_DEVS=level2.dev

# Choose the device(s) to include.  See devs.mak for details.

!if $(GSDLL)
DEVICE_DEVS=mswindll.dev mswin.dev mswinprn.dev
!else
DEVICE_DEVS=mswin.dev mswinprn.dev
!endif
DEVICE_DEVS3=deskjet.dev djet500.dev laserjet.dev ljetplus.dev ljet2p.dev ljet3.dev ljet4.dev
DEVICE_DEVS4=cdeskjet.dev cdjcolor.dev cdjmono.dev cdj550.dev pj.dev pjxl.dev pjxl300.dev
DEVICE_DEVS6=epson.dev eps9high.dev ibmpro.dev st800.dev bj10e.dev bj200.dev
DEVICE_DEVS7=gifmono.dev gif8.dev tiffg3.dev tiffg4.dev
DEVICE_DEVS8=bmpmono.dev bmp16.dev bmp256.dev bmp16m.dev pcxmono.dev pcx16.dev pcx256.dev
DEVICE_DEVS9=pbm.dev pbmraw.dev pgm.dev pgmraw.dev ppm.dev ppmraw.dev
!include "gs.mak"
!include "jpeg.mak"
!include "devs.mak"

# Build the compiler response file depending on the selected options.

ccf32.tr: $(MAKEFILE) makefile
	echo -a1 -d -r -G -N -X -I$(INCDIR) $(CCFLAGS0) -DCHECK_INTERRUPTS>ccf32.tr

# -------------------------------- Library -------------------------------- #

# The Turbo/Borland C(++), Microsoft Windows platform

# Using a file device resource to get the console streams re-initialized
# is bad architecture (an upward reference to ziodev),
# but it will have to do for the moment.
bcwin32__=gp_mswin.$(OBJ) gp_mswtx.$(OBJ) gp_win32.$(OBJ) gp_nofb.$(OBJ) gp_ntfs.$(OBJ)
bcwin32_.dev: $(bcwin32__)
	$(SETMOD) bcwin32_ $(bcwin32__)
	$(ADDMOD) bcwin32_ -iodev wstdio

gp_mswin.$(OBJ): gp_mswin.c $(AK) gp_mswin.h gp_mswtx.h \
 $(ctype__h) $(dos__h) $(malloc__h) $(memory__h) $(stdio__h) $(string__h) $(windows__h) \
 $(gx_h) $(gp_h) $(gpcheck_h) $(gserrors_h) $(gsexit_h) $(gxdevice_h) $(gxiodev_h) $(stream_h)

gp_mswtx.$(OBJ): gp_mswtx.c $(AK) gp_mswtx.h \
 $(ctype__h) $(dos__h) $(memory__h) $(string__h) $(windows__h)

# ----------------------------- Main program ------------------------------ #

BEGINFILES=gs*.res gs*.ico ccf32.tr
CCBEGIN=$(CCC) *.c

# Get around the fact that the DOS shell has a rather small limit on
# the length of a command line.  (sigh)

LIBCTR=libc32.tr

$(LIBCTR): $(MAKEFILE) makefile echogs$(XE)
	echogs -w $(LIBCTR) $(LIBDIR)\import32+
	echogs -a $(LIBCTR) $(LIBDIR)\cw32

# Interpreter main program

ICONS=gsgraph.ico gstext.ico

GS_ALL=gs.$(OBJ) $(INT_ALL) $(INTASM)\
  $(LIB_ALL) $(LIBCTR) obj.tr lib.tr $(GS).res $(GS).def $(ICONS)

# Make the icons from their text form.

gsgraph.ico: gsgraph.icx echogs$(XE)
	echogs -wb gsgraph.ico -n -X -r gsgraph.icx

gstext.ico: gstext.icx echogs$(XE)
	echogs -wb gstext.ico -n -X -r gstext.icx

$(GS).res: $(GS).rc gp_mswin.h $(ICONS)
	$(COMPDIR)\brc32 -i$(INCDIR) -r $(GS)

!if $(GSDLL)
$(GS)$(XE): $(GS).dll

gsdll.$(OBJ): gsdll.c gsdll.h $(ghost_h)

$(GS).dll: $(GS_ALL) $(DEVS_ALL) gsdll.$(OBJ)
	tlink32 $(LCT) /Tpd $(LIBDIR)\c0d32 gsdll @obj.tr $(INTASM) ,$(GS),$(GS),@lib.tr @$(LIBCTR),$(GS).def,$(GS).res
!else
$(GS)$(XE): $(GS_ALL) $(DEVS_ALL)
	tlink32 $(LCT) /Tpe $(LIBDIR)\c0w32 gs @obj.tr $(INTASM) ,$(GS),$(GS),@lib.tr @$(LIBCTR),$(GS).def,$(GS).res
!endif
