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

# makefile for MS-DOS or OS/2 GCC/EMX platform.
# Uses Borland (MSDOS) MAKER or 
# Uses IBM NMAKE.EXE Version 2.000.000 Mar 27 1992

# ------------------------------- Options ------------------------------- #

###### This section is the only part of the file you should need to edit.

# ------ Generic options ------ #

# Define the directory that will hold documentation at runtime.

GS_DOCDIR=c:/gs

# Define the default directory/ies for the runtime
# initialization and font files.  Separate multiple directories with ;.
# Use / to indicate directories, not a single \.

GS_LIB_DEFAULT=c:/gs;c:/gs/fonts

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

# Setting GSDLL=1 makes the target a DLL instead of an EXE
GSDLL=0

# Define the name of the executable file.

!if $(GSDLL)
GS=gsdll2
!else
GS=gsos2
!endif

# Define the directory where the IJG JPEG library sources are stored.
# You may have to change this if the IJG library version changes.
# See jpeg.mak for more information.

JSRCDIR=jpeg-5

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

COMP=gcc
COMPBASE=\emx
EMXPATH=/emx
COMPDIR=$(COMPBASE)\bin
INCDIR=$(EMXPATH)/include
LIBDIR=$(EMXPATH)/lib

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

FPU_TYPE=0

# ---------------------------- End of options ---------------------------- #

# Swapping `make' out of memory makes linking much faster.
# only used by Borland MAKER.EXE

#.swap

# Define the platform name.

PLATFORM=os2_

# Define the name of the makefile -- used in dependencies.

MAKEFILE=os2.mak

# Define the files to be deleted by 'make begin' and 'make clean'.

BEGINFILES=gspmdrv.exe gs*.res gs*.ico

# Define the ANSI-to-K&R dependency.

AK=

#Compiler Optimiser option
CO=-O

# Make sure we get the right default target for make.

dosdefault: default gspmdrv.exe

# Define a rule for invoking just the preprocessor.

.c.i:
	$(COMPDIR)\cpp $(CCFLAGS) $<

# Define the extensions for command, object, and executable files.

CMD=.cmd
O=-o ./
OBJ=o
XE=.exe

# Define the current directory prefix, shell quote string, and shell name.

D=\#

EXP=
QQ="
SH=
SHP=

# Define the arguments for genconf.

#CONFILES=-p %s -o obj.tr -l lib.tr
CONFILES=-o obj.tr -l lib.tr

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

# -------------------------- Auxiliary programs --------------------------- #

CCAUX=$(COMPDIR)\$(COMP) -O

echogs$(XE): echogs.c
	$(CCAUX) -o echogs echogs.c
	$(COMPDIR)\emxbind $(EMXPATH)/bin/emxl.exe echogs echogs.exe
	del echogs

genarch$(XE): genarch.c
	$(CCAUX) -o genarch genarch.c
	$(COMPDIR)\emxbind $(EMXPATH)/bin/emxl.exe genarch genarch.exe
	del genarch

genconf$(XE): genconf.c
	$(CCAUX) -o genconf genconf.c
	$(COMPDIR)\emxbind $(EMXPATH)/bin/emxl.exe genconf genconf.exe
	del genconf

# No special gconfig_.h is needed.
gconfig_.h: echogs$(XE)
	echogs -w gconfig_.h /* This file deliberately left blank. */

# ---------------------- MS-DOS I/O debugging option ---------------------- #

dosio_=zdosio.$(OBJ)
dosio.dev: $(dosio_)
	$(SETMOD) dosio $(dosio_)
	$(ADDMOD) dosio -oper zdosio

zdosio.$(OBJ): zdosio.c $(OP) $(store_h)

# ----------------------------- Assembly code ----------------------------- #

iutilasm.$(OBJ): iutilasm.asm

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
CGDB=-g
!else
CGDB=
!endif

!if $(GSDLL)
CDLL=-Zdll -D__DLL__
!else
CDLL=
!endif

GENOPT=$(CP) $(CD) $(CGDB) $(CDLL)

CCFLAGS0=$(GENOPT) $(PLATOPT)
CCFLAGS=$(CCFLAGS0) 
CC=$(COMPDIR)\$(COMP) $(CCFLAGS0)
CCC=$(CC) -c
CCD=$(CC) -O -c
CCCF=$(COMPDIR)\$(COMP) -O $(CCFLAGS0) -c
CCINT=$(CC) -c

.c.o:
#	$(CCC) { $<}
	$(CCC) $<

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.
# Since we have a large address space, we include some optional features.

FEATURE_DEVS=level2.dev dps.dev

# Choose the device(s) to include.  See devs.mak for details.

!if $(GSDLL)
DEVICE_DEVS=os2dll.dev os2pm.dev
!else
DEVICE_DEVS=os2pm.dev
!endif
DEVICE_DEVS3=deskjet.dev djet500.dev laserjet.dev ljetplus.dev ljet2p.dev ljet3.dev ljet4.dev
DEVICE_DEVS4=cdeskjet.dev cdjcolor.dev cdjmono.dev cdj550.dev pj.dev pjxl.dev pjxl300.dev
DEVICE_DEVS5=djet500c.dev declj250.dev lj250.dev jetp3852.dev r4081.dev t4693d2.dev t4693d4.dev t4693d8.dev tek4696.dev lbp8.dev
DEVICE_DEVS6=epson.dev eps9high.dev eps9mid.dev epsonc.dev ibmpro.dev st800.dev bj10e.dev bj200.dev m8510.dev necp6.dev
DEVICE_DEVS7=gifmono.dev gif8.dev tiffg3.dev tiffg4.dev dfaxhigh.dev dfaxlow.dev
DEVICE_DEVS8=bmpmono.dev bmp16.dev bmp256.dev bmp16m.dev pcxmono.dev pcxgray.dev pcx16.dev pcx256.dev pcx24b.dev
DEVICE_DEVS9=pbm.dev pbmraw.dev pgm.dev pgmraw.dev ppm.dev ppmraw.dev
!include "gs.mak"
!include "jpeg.mak"
!include "devs.mak"

# -------------------------------- Library -------------------------------- #

# The GCC/EMX platform

os2__=gp_nofb.$(OBJ) gp_os2.$(OBJ)
os2_.dev: $(os2__)
	$(SETMOD) os2_ $(os2__)
!if $(GSDLL)
# Using a file device resource to get the console streams re-initialized 
# is bad architecture (an upward reference to ziodev),                   
# but it will have to do for the moment.                                 
#   We need to redirect stdin/out/err to gsdll_callback
        $(ADDMOD) os2_ -iodev wstdio                                   
gsdll_h=gsdll.h
!endif
  

gp_os2.$(OBJ): gp_os2.c $(dos__h) $(string__h) $(time__h) $(gsdll_h) \
  $(gx_h) $(gsexit_h) $(gp_h)

# ----------------------------- Main program ------------------------------ #

CCBEGIN=$(CCC) *.c

# Get around the fact that the DOS shell has a rather small limit on
# the length of a command line.  (sigh)

LIBDOS=$(LIBGS)

# Interpreter main program

ICONS=gsos2.ico gspmdrv.ico

!if $(GSDLL)
#making a DLL
GS_ALL=gsdll.$(OBJ) $(INT_ALL) $(INTASM)\
  $(LIB_ALL) $(LIBCTR) obj.tr lib.tr $(GS).res $(ICONS)

$(GS)$(XE): $(GS).dll

gsdll.$(OBJ): gsdll.c gsdll.h $(ghost_h)

$(GS).dll: $(GS_ALL) $(ALL_DEVS)
	$(COMPDIR)\gcc $(CGDB) $(CDLL) -o $(GS) gsdll.$(OBJ) @obj.tr $(INTASM) -lm
	$(COMPDIR)\emxbind -r$*.res -d$*.def $(COMPDIR)\emxl.exe $(GS) $(GS).dll -ac
	del $(GS)
	emximp gsdll2.imp
!else
#making an EXE
GS_ALL=gs.$(OBJ) $(INT_ALL) $(INTASM)\
  $(LIB_ALL) $(LIBCTR) obj.tr lib.tr $(GS).res $(ICONS)

$(GS)$(XE): $(GS_ALL) $(ALL_DEVS)
	$(COMPDIR)\gcc $(CGDB) -o $(GS) gs.$(OBJ) @obj.tr $(INTASM) -lm
	$(COMPDIR)\emxbind -r$*.res $(COMPDIR)\emxl.exe $(GS) $(GS)$(XE) -ac
	del $(GS)
!endif

# Make the icons from their text form.

gsos2.ico: gsos2.icx echogs$(XE)
	echogs -wb gsos2.ico -n -X -r gsos2.icx

gspmdrv.ico: gspmdrv.icx echogs$(XE)
	echogs -wb gspmdrv.ico -n -X -r gspmdrv.icx

$(GS).res: $(GS).rc gsos2.ico
	rc -i $(COMPBASE)\include -r $*.rc

# PM driver program

gspmdrv.o: gspmdrv.c gspmdrv.h
	$(COMPDIR)\gcc $(CGDB) -c $*.c

gspmdrv.res: gspmdrv.rc gspmdrv.h gspmdrv.ico
	rc -i $(COMPBASE)\include -r $*.rc

gspmdrv.exe: gspmdrv.o gspmdrv.res gspmdrv.def
	$(COMPDIR)\gcc $(CGDB) -o $* $*.o
	$(COMPDIR)\emxbind -p -r$*.res -d$*.def $(COMPDIR)\emxl.exe $* $*.exe
	del $*
