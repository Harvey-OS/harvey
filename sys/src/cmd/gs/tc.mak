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

# makefile for MS-DOS/Turbo C platform.

# ------------------------------- Options ------------------------------- #

###### This section is the only part of the file you should need to edit.

# ------ Generic options ------ #

# Define the directory that will hold documentation at runtime.

GS_DOCDIR=c:/gs

# Define the default directory/ies for the runtime
# initialization and font files.  Separate multiple directories with \;.
# Use \\ or / to indicate directories, not a single \.

GS_LIB_DEFAULT=.;c:/gs\;c:/gs/fonts

# Define the name of the interpreter initialization file.
# (There is no reason to change this.)

GS_INIT=gs_init.ps

# Choose generic configuration options.

# Setting DEBUG=1 includes debugging features (-Z switch) in the code.
# Code runs substantially slower even if no debugging switches are set,
# and also takes about another 25K of memory.

DEBUG=0

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
ASM=masm

# Define the drive and directory for the Turbo C files.
# COMP is the compiler name (always tcc).
# COMPAUX is the compiler name for DOS utilities (same as COMP).
# COMPDIR contains the compiler and linker (normally \tc).
# BGIDIR contains the BGI files (normally \tc).
#   BGIDIRSTR must be the same as BGIDIR with / substituted for \.
# INCDIR contains the include files (normally \tc\include).
# LIBDIR contains the library files (normally \tc\lib).
# Note that these prefixes are always followed by a \,
#   so if you want to use the current directory, use an explicit '.'.

COMP=tcc
COMPAUX=$(COMP)
COMPDIR=c:\tc
BGIDIR=c:\tc
# BGIDIRSTR must be the same as BGIDIR with / substituted for \.
BGIDIRSTR=c:/tc
INCDIR=c:\tc\include
LIBDIR=c:\tc\lib

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

# Define the platform name.

PLATFORM=tbc_

# Define the name of the makefile -- used in dependencies.

MAKEFILE=tc.mak

# Define the ANSI-to-K&R dependency.  (Turbo C accepts ANSI syntax.)

AK=

# Define the compilation flags for an 80286.

F286=-1

# Define compiler switches appropriate to this compiler.

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

GENOPT=$(CP) $(CS) $(CD)

CCFLAGS0=$(GENOPT) $(PLATOPT) $(FPFLAGS) $(CFLAGS) $(XCFLAGS)
CCFLAGS=$(CCFLAGS0) -m$(MM)
CC=$(COMPDIR)\$(COMP) -d -r -y -G -I$(INCDIR)
CCC=$(CC) -a $(CCFLAGS) -O -c
CCD=$(CCC)
CCCF=$(CC) -a $(CCFLAGS0) -mh -O -c
CCINT=$(CC) -a $(CCFLAGS) -c

.c.obj:
	$(CCC) $<

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.
# Note that because Turbo C doesn't use overlays,
# we don't include any optional features.

FEATURE_DEVS=level1.dev

# Choose the device(s) to include.  See devs.mak for details.
# Note that because Turbo C doesn't use overlays,
# we only include a limited set of device drivers.

DEVICE_DEVS=vga.dev ega.dev svga16.dev
DEVICE_DEVS3=deskjet.dev
DEVICE_DEVS6=epson.dev eps9high.dev ibmpro.dev bj10e.dev bj200.dev
!include "gs.mak"
!include "jpeg.mak"
!include "devs.mak"

# -------------------------------- Library -------------------------------- #

# The Turbo/Borland C(++) platform

tbc__=gp_itbc.$(OBJ) gp_msdos.$(OBJ) gp_dosfb.$(OBJ) gp_dosfs.$(OBJ) gp_dosfe.$(OBJ)
tbc_.dev: $(tbc__)
	$(SETMOD) tbc_ $(tbc__)

# We have to compile gp_itbc without -1, because it includes a run-time
# check to make sure we are running on the right kind of processor.
gp_itbc.$(OBJ): gp_itbc.c $(string__h) $(gx_h) $(gp_h) $(MAKEFILE) makefile
	$(CC) -a -m$(MM) $(GENOPT) -DCPU_TYPE=$(CPU_TYPE) -c gp_itbc.c

# ----------------------------- Main program ------------------------------ #

BEGINFILES=
CCBEGIN=$(CCC) *.c

# Get around the fact that the DOS shell has a rather small limit on
# the length of a command line.  (sigh)

LIBCTR=libc$(MM).tr

$(LIBCTR): $(MAKEFILE) makefile echogs.exe
	echogs -w $(LIBCTR) $(LIBDIR)\$(FPLIB)+
	echogs -a $(LIBCTR) $(LIBDIR)\math$(MM) $(LIBDIR)\c$(MM)

# Interpreter main program

GS_ALL=gs.$(OBJ) $(INT_ALL) $(INTASM) $(LIB_ALL) $(LIBCTR) obj.tr lib.tr

$(GS)$(XE): $(GS_ALL) $(DEVS_ALL)
	$(COMPDIR)\tlink /m /l $(LIBDIR)\c0$(MM) gs @obj.tr $(INTASM) ,$(GS),$(GS),@lib.tr @$(LIBCTR)
