#    Copyright (C) 1991, 1992, 1993 Aladdin Enterprises.  All rights reserved.
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

# makefile for MS-DOS MSC 7.0 platform.
# Thanks to Phil Conrad and Thomas Hiller for earlier versions of this file.

# NOTE: Do NOT compile iscan.c or gdevpcfb.c with optimization.

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

# Setting TDEBUG=1 includes symbol table information for the debugger,
# and also enables stack checking.  Code is substantially slower and larger.

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

# Define the drive, directory, and compiler name for the Microsoft C files.
# COMP is the full compiler path name (normally \msc\bin\wcc386p).
# LINK is the full linker path name (normally \msc\bin\link).
# CLINK is the compile-and-link utility full path name (normally
#   \msc\bin\link).
# INCDIR contains the include files (normally \msc\include).
# LIBDIR contains the library files (normally \msc\lib).
# Note that INCDIR and LIBDIR are always followed by a \,
#   so if you want to use the current directory, use an explicit '.'.

COMP=\msc\bin\cl
LINK=\msc\bin\link /NOI /BATCH /CPARMAX:1 /ONERROR:NOEXE
CLINK=\msc\bin\cl /batch
INCDIR=\msc\include
LIBDIR=\msc\lib

# Define the processor (CPU) type.  Currently the only acceptable value
# is 286.  (386, 486, and 586 should be supported, but MSC apparently
# doesn't provide any way to generate 16-bit code if you tell it you have
# a 32-bit processor.)

CPU_TYPE=286

# Define the math coprocessor (FPU) type.
# Options are -1 (optimize for no FPU), 0 (optimize for FPU present,
# but do not require a FPU), 87, 287, or 387.
# If you have a 486 or Pentium CPU, you should normally set FPU_TYPE to 387,
# since most of these CPUs include the equivalent of an 80387 on-chip;
# however, the 486SX and the Cyrix 486SLC do not have an on-chip FPU, so if
# you have one of these CPUs and no external FPU, set FPU_TYPE to -1 or 0.
# An xx87 option means that the executable will run only if a FPU
# of that type (or higher) is available: this is NOT currently checked
# at runtime.

FPU_TYPE=0

# ---------------------------- End of options ---------------------------- #

# Define the platform name.

PLATFORM=msc_

# Define the name of the makefile -- used in dependencies.

#MAKEFILE=msc.mak
MAKEFILE=makefile

# Define the ANSI-to-K&R dependency.  Microsoft C accepts ANSI syntax,
# but we need to preconstruct ccf.tr to get around the limit on
# the maximum length of a command line.

AK=ccf.tr

# Define the syntax for command, object, and executable files.

CMD=.bat
O=-o
OBJ=obj
XE=.exe

# Define the current directory prefix and shell invocations.

D=\\

EXPP=
SH=
SHP=

# Define the arguments for genconf.

CONFILES=-p %s+ -o obj.tr

# Define the generic compilation flags.

PLATOPT=

INTASM=
PCFBASM=

# Define the generic compilation rules.

.asm.obj:
	$(ASM) $(ASMFLAGS) $<;

# Make sure we get the right default target for make.

all: gs$(XE)

# Define the compilation flags.

!if $(CPU_TYPE)>400
CPFLAGS=/G4
!else if $(CPU_TYPE)>300
CPFLAGS=/G3
!else if $(CPU_TYPE)>200
CPFLAGS=/G2
!else if $(CPU_TYPE)>100
CPFLAGS=/G1
!else
CPFLAGS=/G0
!endif

!if $(FPU_TYPE)>0
FPFLAGS=/FPi87
!else
FPFLAGS=/FPi
!endif

!if $(NOPRIVATE)!=0
CP=/DNOPRIVATE
!else
CP=
!endif

!if $(DEBUG)!=0
CD=/DDEBUG /Gt128
!else
CD=
!endif

!if $(TDEBUG)!=0
CT=/f /Zi /Od
LCT=/CO /FAR /PACKC
!else
CT=
#CT=/f- /Ot /Oi /Ol /Oe /Og /Gs
LCT=/EXE /FAR /PACKC
!endif

!if $(DEBUG)!=0 || $(TDEBUG)!=0
CS=/Ge
!else
CS=/Gs
!endif

GENOPT=$(CP) $(CD) $(CT) $(CS) /AL /W2 /batch /nologo

CCFLAGS=$(PLATOPT) $(FPFLAGS) $(CPFLAGS) $(CFLAGS) $(XCFLAGS)
CC=$(COMP) /c $(CCFLAGS) @ccf.tr
CCL=$(CLINK)
CCAUX=$(CCL) $(CCFLAGS)
CCC=$(CC) /Za
CCD=$(CC)
CCINT=$(CC) /Za

.c.obj:
	$(CCC) $<

# Define the files to be removed by `make clean'.
# nmake expands macros when encountered, not when used,
# so this must precede the !include statements.

BEGINFILES=ccf.tr

# -------------------------- Auxiliary programs --------------------------- #

ccf.tr: $(MAKEFILE)
	echo $(GENOPT) /I$(INCDIR) >ccf.tr

# No special gconfig_.h is needed.
# Microsoft `make' supports output redirection.
gconfig_.h:
	echo /* This file deliberately left blank. */ >gconfig_.h

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.

FEATURE_DEVS=level1.dev filter.dev

# Choose the device(s) to include.  See devs.mak for details.

DEVICE_DEVS=vga.dev ega.dev svga16.dev
DEVICE_DEVS1=atiw.dev tseng.dev tvga.dev
DEVICE_DEVS3=deskjet.dev djet500.dev laserjet.dev ljetplus.dev ljet2p.dev ljet3.dev ljet4.dev
DEVICE_DEVS4=cdeskjet.dev cdjcolor.dev cdjmono.dev cdj550.dev pj.dev pjxl.dev pjxl300.dev
DEVICE_DEVS6=epson.dev eps9high.dev ibmpro.dev bj10e.dev bj200.dev
DEVICE_DEVS7=gifmono.dev gif8.dev
DEVICE_DEVS8=pcxmono.dev pcxgray.dev pcx16.dev pcx256.dev pcx24b.dev
!include gs.mak
!include jpeg.mak
!include devs.mak

# -------------------------------- Library -------------------------------- #

# The Microsoft C platform

# Eventually we need a gp_imsc.$(OBJ)....
msc__=gp_iwatc.$(OBJ) gp_msdos.$(OBJ) gp_dosfb.$(OBJ) gp_dosfs.$(OBJ) gp_dosfe.$(OBJ)
msc_.dev: $(msc__)
	$(SETMOD) msc_ $(msc__)

gp_iwatc.$(OBJ): gp_iwatc.c $(string__h) $(gx_h) $(gp_h)

# ----------------------------- Main program ------------------------------ #

CCBEGIN=$(CCC) *.c

# Interpreter main program

GS_ALL=gs.$(OBJ) $(INT_ALL) $(INTASM) $(LIB_ALL) obj.tr lib.tr

$(GS)$(XE): $(GS_ALL) $(DEVS_ALL)
	$(LINK) /SEG:256 /STACK:8192 $(LCT) gs @obj.tr ,,$(GS),$(GS); 
