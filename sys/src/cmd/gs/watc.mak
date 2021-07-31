#    Copyright (C) 1991, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
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

# makefile for MS-DOS/Watcom C386 platform.
# We strongly recommend that you read the Watcom section of make.doc
# before attempting to build Ghostscript with the Watcom compiler.

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

# Define the directory where the IJG JPEG library sources are stored.
# You may have to change this if the IJG library version changes.
# See jpeg.mak for more information.

JSRCDIR=jpeg-5

# Define any other compilation flags.  Including -DA4 makes A4 paper size
# the default for most, but not, printer drivers.

CFLAGS=

# ------ Platform-specific options ------ #

# Define the drive, directory, and compiler name for the Watcom C files.
# $(%WATCOM) means use the WATCOM environment variable, which is also used
#   by the Watcom C programs themselves.
# COMP is the full compiler path name (normally $(%WATCOM)\bin\wcc386p,
#   but possibly $(%WATCOM)\binb\wcc386p for version 10.0 or later).
# LINK is the full linker path name (normally $(%WATCOM)\bin\wlinkp).
# CLINK is the compile-and-link utility full path name (normally
#   $(%WATCOM)\binb\wcl386).
# STUB is the full path name for the DOS extender stub (normally
#   $(%WATCOM)\binb\wstub.exe).
# INCDIR contains the include files (normally $(%WATCOM)\h).
# LIBDIR contains the library files (normally $(%WATCOM)\lib386).
# Note that INCDIR and LIBDIR are always followed by a \,
#   so if you want to use the current directory, use an explicit '.'.

COMP=$(%WATCOM)\bin\wcc386p
LINK=$(%WATCOM)\bin\wlinkp
CLINK=$(%WATCOM)\binb\wcl386
STUB=$(%WATCOM)\binb\wstub.exe
INCDIR=$(%WATCOM)\h
LIBDIR=$(%WATCOM)\lib386

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

# ---------------------------- End of options ---------------------------- #

# Define the platform name.

PLATFORM=watc_

# Define the name of the makefile -- used in dependencies.

MAKEFILE=watc.mak

# Define additional platform compilation flags.

PLATOPT=

!include wccommon.mak

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.
# Since we have a large address space, we include the optional features.

FEATURE_DEVS=level2.dev

# Choose the device(s) to include.  See devs.mak for details.

DEVICE_DEVS=vga.dev ega.dev svga16.dev
DEVICE_DEVS1=atiw.dev tseng.dev tvga.dev
DEVICE_DEVS3=deskjet.dev djet500.dev laserjet.dev ljetplus.dev ljet2p.dev ljet3.dev ljet4.dev
DEVICE_DEVS4=cdeskjet.dev cdjcolor.dev cdjmono.dev cdj550.dev pj.dev pjxl.dev pjxl300.dev
DEVICE_DEVS6=epson.dev eps9high.dev ibmpro.dev bj10e.dev bj200.dev
DEVICE_DEVS7=gifmono.dev gif8.dev tiffg3.dev faxg3.dev
DEVICE_DEVS8=pcxmono.dev pcxgray.dev pcx16.dev pcx256.dev pcx24b.dev
DEVICE_DEVS9=pbm.dev pbmraw.dev pgm.dev pgmraw.dev ppm.dev ppmraw.dev bitcmyk.dev
!include gs.mak
!include jpeg.mak
!include devs.mak

# -------------------------------- Library -------------------------------- #

# The Watcom C platform

watc__=gp_iwatc.$(OBJ) gp_msdos.$(OBJ) gp_dosfb.$(OBJ) gp_dosfs.$(OBJ) gp_dosfe.$(OBJ)
watc_.dev: $(watc__)
	$(SETMOD) watc_ $(watc__)

gp_iwatc.$(OBJ): gp_iwatc.c $(stat__h) $(string__h) $(gx_h) $(gp_h)

# ----------------------------- Main program ------------------------------ #

BEGINFILES=*.err
# The Watcom compiler doesn't recognize wildcards;
# we don't want any compilation to fail.
CCBEGIN=for %%f in (gs*.c gx*.c z*.c) do $(CCC) %%f

LIBDOS=$(LIB_ALL) gp_iwatc.$(OBJ) gp_msdos.$(OBJ) gp_dosfb.$(OBJ) gp_dosfs.$(OBJ) objw.tr

# Interpreter main program

GS_ALL=gs.$(OBJ) gsmain.$(OBJ) $(INT_ALL) $(INTASM) $(LIBDOS)

objwl.tr: $(MAKEFILE)
	echo SYSTEM DOS4G >objwl.tr
	echo OPTION STUB=$(STUB) >>objwl.tr
	echo OPTION STACK=8k >>objwl.tr

$(GS)$(XE): $(GS_ALL) $(DEVS_ALL) objwl.tr
	$(LINK) $(LCT) NAME $(GS) OPTION MAP=$(GS) FILE gs @objw.tr @objwl.tr
