#    Copyright (C) 1989, 1990, 1991, 1992, 1993 Aladdin Enterprises.  All rights reserved.
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

# makefile for MS-DOS/Borland C++ platform.

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

# Setting TDEBUG=1 includes symbol table information for the Borland debugger,
# and also enables stack checking.  Code is substantially slower and larger.
# Setting TDEBUG=2 includes symbol table information for the MultiScope
# debugger.

TDEBUG=0

# Setting NOPRIVATE=1 makes private (static) procedures and variables public,
# so they are visible to the debugger and profiler.
# No execution time or space penalty, just larger .OBJ and .EXE files.

NOPRIVATE=0

# Define the name of the executable file.

GS=gs

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
# BGIDIR contains the BGI files (normally /bc/bgi).
#   BGIDIRSTR must be the same as BGIDIR with / substituted for \.
#   (These are only needed if you include the BGI driver.)
# INCDIR contains the include files (normally \bc\include).
# LIBDIR contains the library files (normally \bc\lib).
# Note that these prefixes are always followed by a \,
#   so if you want to use the current directory, use an explicit '.'.

COMP=bcc
COMPAUX=$(COMP)
COMPDIR=c:\bc\bin
BGIDIR=c:\bc\bgi
# BGIDIRSTR must be the same as BGIDIR with / substituted for \.
BGIDIRSTR=c:/bc/bgi
INCDIR=c:\bc\include
LIBDIR=c:\bc\lib

# Define whether you want to use the Borland code overlay mechanism
# (VROOMM).  Code overlays make it possible to process larger files,
# but code swapping will slow down the program even on smaller ones.
# See the file overlay.h to control details of overlaying such as
# the overlay buffer size, whether to use EMS and/or extended memory
# to store evicted overlays, and how much of that memory to use.

OVERLAY=1

# Choose platform-specific options.

# Define the processor (CPU) type.  Options are 86 (8086 or 8088),
# 186, 286, 386, 485 (486SX or Cyrix 486SLC), 486 (486DX), or 586 (Pentium).
# Higher numbers produce code that may be significantly smaller and faster,
# but the executable will bail out with an error message on any processor
# less capable than the designated one.

CPU_TYPE=286

# Define the math coprocessor (FPU) type.
# Options are -1 (optimize for no FPU), 0 (optimize for FPU present,
# but do not require a FPU), 87, 287, or 387.
# If CPU_TYPE is 486 or above, FPU_TYPE is implicitly set to 387,
# since 486DX and later processors include the equivalent of an 80387 on-chip.
# An xx87 option means that the executable will run only if a FPU
# of that type (or higher) is available: this is NOT currently checked
# at runtime.

FPU_TYPE=0

# ---------------------------- End of options ---------------------------- #

# Swapping `make' out of memory makes linking much faster.

.swap

# Define the platform name.

PLATFORM=bc_

# Define the name of the makefile -- used in dependencies.

MAKEFILE=bc.mak

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

!if $(__MAKE__) >= 0x360
CO0=-Obe -Z
COD0=-O
COINT0=-Obet
!else
CO0=-O
COD0=-O
COINT0=
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

!if $(TDEBUG) == 2
CO=-k -Od
COD=$(CO)
COINT=$(CO)
!else
CO=$(CO0)
COD=$(COD0)
COINT=$(COINT0)
!endif

GENOPT=$(CP) $(CS) $(CD) $(CT)

!if $(OVERLAY)
CY=-Y
CYO=-Yo -zAOVLY
LO=/oOVLY
LIBOVL=$(LIBDIR)\overlay
OVLH=overlay.h
!else
CY=
CYO=
LO=
LIBOVL=
OVLH=
!endif

CCFLAGS0=$(GENOPT) $(PLATOPT) $(FPFLAGS) $(CFLAGS) $(XCFLAGS)
CCFLAGS=$(CCFLAGS0) -m$(MM)
CC=$(COMPDIR)\$(COMP) -m$(MM) -zEGS_FAR_DATA @ccf.tr
CCC=$(CC) $(CYO) $(CO) -c
CCD=$(CC) $(CYO) $(COD) -c
CCCF=$(COMPDIR)\$(COMP) -mh @ccf.tr $(CYO) -O -c
CCINT=$(CC) $(CYO) $(COINT) -c

.c.obj:
	$(CCC) { $<}

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.
# Even though code overlays are available, we don't include most of the
# optional features, because they cost a significant amount of non-code space.

FEATURE_DEVS=level1.dev color.dev

# Choose the device(s) to include.  See devs.mak for details.
# We don't include the BGI driver, because it takes 10K of
# non-overlayable code space.

DEVICE_DEVS=vga.dev ega.dev svga16.dev vesa.dev
DEVICE_DEVS1=atiw.dev tseng.dev tvga.dev
DEVICE_DEVS3=deskjet.dev djet500.dev laserjet.dev ljetplus.dev ljet2p.dev ljet3.dev ljet4.dev
DEVICE_DEVS4=cdeskjet.dev cdjcolor.dev cdjmono.dev cdj550.dev pj.dev pjxl.dev pjxl300.dev
DEVICE_DEVS6=bj10e.dev bj200.dev epson.dev eps9high.dev ibmpro.dev
DEVICE_DEVS7=gifmono.dev gif8.dev
DEVICE_DEVS8=pcxmono.dev pcxgray.dev pcx16.dev pcx256.dev pcx24b.dev
!include "gs.mak"
!include "jpeg.mak"
!include "devs.mak"

# Build the compiler response file depending on the selected options.

ccf.tr: $(MAKEFILE) makefile
!if $(CPU_TYPE) < 286
	echo -a -d -r -G -X -I$(INCDIR) $(CCFLAGS0) >ccf.tr
!else
!  if $(CPU_TYPE) > 286
	echo -2 -a -d -r -G -X -I$(INCDIR) $(CCFLAGS0) >t.tr
	if not $(EXIST_BC3_1) copy t.tr ccf.tr
	echo -3 -a -d -r -G -X -I$(INCDIR) $(CCFLAGS0) >t.tr
	if $(EXIST_BC3_1) copy t.tr ccf.tr
	erase t.tr
!  else
	echo -2 -a -d -r -G -X -I$(INCDIR) $(CCFLAGS0) >ccf.tr
!  endif
!endif

ccf1.tr: $(MAKEFILE) makefile
	echo -I$(INCDIR) $(CCFLAGS) >ccf1.tr

# -------------------------------- Library -------------------------------- #

# The Turbo/Borland C(++) platform

bc__=gp_itbc.$(OBJ) gp_msdos.$(OBJ) gp_dosfb.$(OBJ) gp_dosfs.$(OBJ) gp_dosfe.$(OBJ)
bc_.dev: $(bc__)
	$(SETMOD) bc_ $(bc__)

# We have to compile gp_itbc without -1, because it includes a run-time
# check to make sure we are running on the right kind of processor.
gp_itbc.$(OBJ): gp_itbc.c $(string__h) $(gx_h) $(gp_h) \
 $(OVLH) $(MAKEFILE) makefile ccf1.tr
	$(CC) @ccf1.tr -1- $(CY) -DCPU_TYPE=$(CPU_TYPE) -c gp_itbc.c

# ----------------------------- Main program ------------------------------ #

BEGINFILES=ccf*.tr
CCBEGIN=$(CCC) *.c

# Get around the fact that the DOS shell has a rather small limit on
# the length of a command line.  (sigh)

LIBCTR=libc$(MM).tr

$(LIBCTR): $(MAKEFILE) makefile echogs$(XE)
	echogs -w $(LIBCTR) $(LIBOVL) $(LIBDIR)\$(FPLIB)+
	echogs -a $(LIBCTR) $(LIBDIR)\math$(MM) $(LIBDIR)\c$(MM)

# Interpreter main program

GS_ALL=gs.$(OBJ) $(INT_ALL) $(INTASM) $(LIB_ALL) $(LIBCTR) obj.tr lib.tr

$(GS)$(XE): $(GS_ALL) $(DEVS_ALL)
	$(COMPDIR)\tlink $(LCT) $(LO) $(LIBDIR)\c0$(MM) gs @obj.tr $(INTASM) ,$(GS),$(GS),@lib.tr @$(LIBCTR)
