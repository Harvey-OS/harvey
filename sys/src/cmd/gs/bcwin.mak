#    Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
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

# makefile for MS-Windows 3.n/Borland C++ 3.0 or 3.1 platform.

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
GS=gsdll16
!else
GS=gswin
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

USE_ASM=1
ASM=tasm

# Define the drive, directory, and compiler name for the Turbo C files.
# COMP is the compiler name (tcc for Turbo C++, bcc for Borland C++).
# COMPAUX is the compiler name for DOS utilities (same as COMP).
# COMPDIR contains the compiler and linker (normally \bc\bin).
# INCDIR contains the include files (normally \bc\include).
# LIBDIR contains the library files (normally \bc\lib).
# Note that these prefixes are always followed by a \,
#   so if you want to use the current directory, use an explicit '.'.

COMP=bcc
COMPAUX=$(COMP)
COMPBASE=c:\bc
COMPDIR=$(COMPBASE)\bin
INCDIR=$(COMPBASE)\include
LIBDIR=$(COMPBASE)\lib

# Define the Windows directory.

WINDIR=c:\windows

# Windows 3.n requires a 286 CPU or higher.

CPU_TYPE=286

# Don't rely on FPU for Windows: Options are -1 (optimize for no FPU) or
# 0 (optimize for FPU present, but do not require a FPU).

FPU_TYPE=0

# ---------------------------- End of options ---------------------------- #

# Swapping `make' out of memory makes linking much faster.

.swap

# Define the platform name.

PLATFORM=bcwin_

# Define the name of the makefile -- used in dependencies.

MAKEFILE=bcwin.mak

# Define the ANSI-to-K&R dependency.  Turbo C accepts ANSI syntax,
# but we need to preconstruct ccf.tr to get around the limit on
# the maximum length of a command line.

AK=ccf.tr

# Define the compilation flags for an 80286.
# This is void because we handle -2 and -3 in ccf.tr (see below).

F286=

# Figure out which version of Borland C++ we are running.
# In the MAKE program that comes with Borland C++ 2.0, __MAKE__ is 0x300;
# in the MAKE that comes with Borland C++ 3.0 and 3.1, it is 0x360.
# We care because 3.0 has additional optimization features.
# There are also some places below where we distinguish BC++ 3.1 from 3.0
#   by testing whether $(INCDIR)\win30.h exists (true in 3.1, false in 3.0).

EXIST_BC3_1=exist $(INCDIR)\win30.h

# Figure out which version of Windows we are running, 3.0 or 3.1.
# I *think* 3.0 doesn't have the 256COLOR.BMP file.

WINDOWS_3_1=exist $(WINDIR)\256color.bmp

# Define compiler switches appropriate to this compiler.

!if $(__MAKE__) >= 0x360
CO=-Obe -Z
!else
CO=-O
!endif
CAOPT=-a
WINCOMP=0

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
LCT=/v
!else
CT=-y
LCT=/m /l
!endif

GENOPT=$(CP) $(CS) $(CD) $(CT)

CCFLAGS0=$(GENOPT) $(PLATOPT) $(FPFLAGS) $(CFLAGS) $(XCFLAGS)
CCFLAGS=$(CCFLAGS0) -m$(MM)
CC=$(COMPDIR)\$(COMP) -m$(MM) -zEGS_FAR_DATA @ccf.tr
# We want a Windows DLL with only selected functions exported:
!if $(GSDLL)
WX=-WDE
!else
# We want a Windows executable with only selected functions exported:
WX=-WE
!endif
CCC=$(CC) $(WX) $(CO) -c
CCD=$(CC) $(WX) -O -c
# Should be -mh !!!
CCCF=$(COMPDIR)\$(COMP) -ml -zEGS_FAR_DATA @ccf.tr $(WX) -c
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
DEVICE_DEVS6=epson.dev eps9high.dev ibmpro.dev bj10e.dev bj200.dev
DEVICE_DEVS7=gifmono.dev gif8.dev
# Including DEVS8 overflows the default data segment.
#DEVICE_DEVS8=bmpmono.dev bmp16.dev bmp256.dev bmp16m.dev
!include "gs.mak"
!include "jpeg.mak"
!include "devs.mak"

# Build the compiler response file depending on the selected options.

ccf.tr: $(MAKEFILE) makefile
	echo -2 -a -d -r -G -N -X -I$(INCDIR) $(CCFLAGS0) -DCHECK_INTERRUPTS >ccf.tr
!if $(CPU_TYPE) > 286
	echo -3 -a -d -r -G -N -X -I$(INCDIR) $(CCFLAGS0) -DCHECK_INTERRUPTS >_temp_
	if $(EXIST_BC3_1) copy _temp_ ccf.tr
!endif

# -------------------------------- Library -------------------------------- #

# The Turbo/Borland C(++), Microsoft Windows platform

# Using a file device resource to get the console streams re-initialized
# is bad architecture (an upward reference to ziodev),
# but it will have to do for the moment.
bcwin__=gp_mswin.$(OBJ) gp_mswtx.$(OBJ) gp_msdos.$(OBJ) gp_nofb.$(OBJ) gp_dosfs.$(OBJ) gp_dosfe.$(OBJ)
bcwin_.dev: $(bcwin__)
	$(SETMOD) bcwin_ $(bcwin__)
	$(ADDMOD) bcwin_ -iodev wstdio

gp_mswin.$(OBJ): gp_mswin.c $(AK) gp_mswin.h gp_mswtx.h \
 $(ctype__h) $(dos__h) $(malloc__h) $(memory__h) $(stdio__h) $(string__h) $(windows__h) \
 $(gx_h) $(gp_h) $(gpcheck_h) $(gserrors_h) $(gsexit_h) $(gxdevice_h) $(gxiodev_h) $(stream_h)

gp_mswtx.$(OBJ): gp_mswtx.c $(AK) gp_mswtx.h \
 $(ctype__h) $(dos__h) $(memory__h) $(string__h) $(windows__h)

# ----------------------------- Main program ------------------------------ #

BEGINFILES=gs*.ico ccf.tr
CCBEGIN=$(CCC) *.c

# Get around the fact that the DOS shell has a rather small limit on
# the length of a command line.  (sigh)

LIBCTR=libc$(MM).tr

$(LIBCTR): $(MAKEFILE) makefile echogs$(XE)
	echogs -w $(LIBCTR) $(LIBDIR)\import+
	echogs -a $(LIBCTR) $(LIBDIR)\mathw$(MM) $(LIBDIR)\cw$(MM)

# Interpreter main program

ICONS=gsgraph.ico gstext.ico

GS_ALL=gs.$(OBJ) $(INT_ALL) $(INTASM)\
  $(LIB_ALL) $(LIBCTR) obj.tr lib.tr $(GS).def $(ICONS)

# Make the icons from their text form.

gsgraph.ico: gsgraph.icx echogs$(XE)
	echogs -wb gsgraph.ico -n -X -r gsgraph.icx

gstext.ico: gstext.icx echogs$(XE)
	echogs -wb gstext.ico -n -X -r gstext.icx

!if $(GSDLL)
$(GS)$(XE): $(GS).dll

gsdll.$(OBJ): gsdll.c gsdll.h $(ghost_h)

$(GS).dll: $(GS_ALL) $(DEVS_ALL) gsdll.$(OBJ)
	tlink $(LCT) /Twd $(LIBDIR)\c0d$(MM) gsdll @obj.tr $(INTASM) ,$(GS),$(GS),@lib.tr @$(LIBCTR),$(GS).def
	$(COMPDIR)\rc -K -30 -i$(INCDIR) $(GS).rc $(GS).dll
!else
$(GS)$(XE): $(GS_ALL) $(DEVS_ALL) $(GS).rc gp_mswin.h $(ICONS)
	tlink $(LCT) /Twe $(LIBDIR)\c0w$(MM) gs @obj.tr $(INTASM) ,$(GS),$(GS),@lib.tr @$(LIBCTR),$(GS).def
	echo $(COMPDIR)\rc -30 -i$(INCDIR) -K $(GS).rc $(GS).exe >_temp_.bat
	echo $(COMPDIR)\rc -i$(INCDIR) -K $(GS).rc $(GS).exe >_temp_
	if not $(EXIST_BC3_1) copy _temp_ _temp_.bat
	if $(WINDOWS_3_1) copy _temp_ _temp_.bat
	_temp_.bat
!endif
