#    Copyright (C) 1989-1999 Aladdin Enterprises.  All rights reserved.
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

# $Id: bcwin32.mak,v 1.38 2005/08/31 05:52:32 ray Exp $
# makefile for (MS-Windows 3.1/Win32s / Windows 95 / Windows NT) +
#   Borland C++ 4.5 platform.
#   Borland C++Builder 3 platform (need BC++ 4.5 for 16-bit code)

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

!ifndef AROOTDIR
AROOTDIR=c:/gs
!endif

!ifndef GSROOTDIR
GSROOTDIR=$(AROOTDIR)/gs$(GS_DOT_VERSION)
!endif

# Define the directory that will hold documentation at runtime.

GS_DOCDIR=$(GSROOTDIR)/doc

# Define the default directory/ies for the runtime
# initialization, resource and font files.  Separate multiple directories with \;.
# Use / to indicate directories, not a single \.

!ifndef GS_LIB_DEFAULT
GS_LIB_DEFAULT=$(GSROOTDIR)/lib\;$(GSROOTDIR)/Resource\;$(AROOTDIR)/fonts
!endif

# Define whether or not searching for initialization files should always
# look in the current directory first.  This leads to well-known security
# and confusion problems, but users insist on it.
# NOTE: this also affects searching for files named on the command line:
# see the "File searching" section of Use.htm for full details.
# Because of this, setting SEARCH_HERE_FIRST to 0 is not recommended.

!ifndef SEARCH_HERE_FIRST
SEARCH_HERE_FIRST=1
!endif

# Define the name of the interpreter initialization file.
# (There is no reason to change this.)

!ifndef GS_INIT
GS_INIT=gs_init.ps
!endif

# Choose generic configuration options.

# Setting DEBUG=1 includes debugging features (-Z switch) in the code.  The
# compiled code is substantially slower and larger.

!ifndef DEBUG
DEBUG=0
!endif

# Setting TDEBUG=1 includes symbol table information for the debugger, and
# also enables stack checking.  The compiled code is substantially slower
# and larger.

!ifndef TDEBUG
TDEBUG=0
!endif

# Setting NOPRIVATE=1 makes private (static) procedures and variables
# public, so they are visible to the debugger and profiler.  There is no
# execution time or space penalty, just larger .OBJ and .EXE files.

!ifndef NOPRIVATE
NOPRIVATE=0
!endif

# Define the names of the executable files.

GS=gswin32
GSCONSOLE=gswin32c
GSDLL=gsdll32

# Define the name of a pre-built executable that can be invoked at build
# time.  Currently, this is only needed for compiled fonts.  The usual
# alternatives are:
#   - the standard name of Ghostscript on your system (typically `gs'):
BUILD_TIME_GS=gswin32c
#   - the name of the executable you are building now.  If you choose this
# option, then you must build the executable first without compiled fonts,
# and then again with compiled fonts.
#BUILD_TIME_GS=$(BINDIR)\$(GS) -I$(PSLIBDIR)

# To build two small executables and a large DLL, use MAKEDLL=1.
# To build two large executables, use MAKEDLL=0.

!ifndef MAKEDLL
MAKEDLL=1
!endif

# If you want multi-thread-safe compilation, set MULTITHREAD=1; if not, set
# MULTITHREAD=0.  MULTITHREAD=0 produces slightly smaller and faster code,
# but MULTITHREAD=1 is required if you use any "asynchronous" output
# drivers.

!ifndef MULTITHREAD
MULTITHREAD=1
!endif

# Define the directory where the IJG JPEG library sources are stored,
# and the major version of the library that is stored there.
# You may need to change this if the IJG library version changes.
# See jpeg.mak for more information.

!ifndef JSRCDIR
JSRCDIR=jpeg
JVERSION=6
!endif

# Define the directory where the PNG library sources are stored,
# and the version of the library that is stored there.
# You may need to change this if the libpng version changes.
# See libpng.mak for more information.

!ifndef PSRCDIR
PSRCDIR=libpng
PVERSION=10208
!endif

# Define the directory where the zlib sources are stored.
# See zlib.mak for more information.

!ifndef ZSRCDIR
ZSRCDIR=zlib
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

# Define any other compilation flags.

!ifndef CFLAGS
CFLAGS=
!endif

# Do not edit the next group of lines.

#!include $(COMMONDIR)\bcdefs.mak
#!include $(COMMONDIR)\pcdefs.mak
#!include $(COMMONDIR)\generic.mak
!include $(GLSRCDIR)\version.mak
# The following is a hack to get around the special treatment of \ at
# the end of a line.
NUL=
DD=$(GLGENDIR)\$(NUL)
GLD=$(GLGENDIR)\$(NUL)
PSD=$(PSGENDIR)\$(NUL)

# ------ Platform-specific options ------ #

# Define the drive, directory, and compiler name for the Borland C files.
# BUILDER_VERSION=0 for BC++4.5, 3 for C++Builder3, 4 for C++Builder4,
#  5 for C++Builder5.
# COMPDIR contains the compiler and linker (normally \bc\bin).
# INCDIR contains the include files (normally \bc\include).
# LIBDIR contains the library files (normally \bc\lib).
# COMP is the full C compiler name (bcc32 for Borland C++).
# COMPCPP is the full C++ compiler path name (bcc32 for Borland C++).
# COMPAUX is the compiler name for DOS utilities (bcc for Borland C++).
# RCOMP is the resource compiler name (brcc32 for Borland C++).
# LINK is the full linker path name (normally \bc\bin\tlink32).
# Note that these prefixes are always followed by a \,
#   so if you want to use the current directory, use an explicit '.'.

# Rod Webster (rodw)
# If C++Builder is later than 4 then you need to 
# define BUILDER_VERSION explicity uisng BUILDER_VERSION=5 because 
# C++Builder 4 and above all use Make Version 5.2 so point we can no 
# longer tell the Compiler version from the __MAKE__ version number.

!ifndef BUILDER_VERSION
BUILDER_VERSION=5
!endif

!ifndef BUILDER_VERSION
!if $(__MAKE__) >= 0x520
# C++Builder4
BUILDER_VERSION=4
!elif $(__MAKE__) >= 0x510
# C++Builder3
BUILDER_VERSION=3
!else
# BC++4.5
BUILDER_VERSION=0
!endif
!endif

!ifndef COMPBASE
!if $(BUILDER_VERSION) == 0
COMPBASE=c:\bc
COMPBASE16=$(COMPBASE)
!endif
!if $(BUILDER_VERSION) == 3
COMPBASE=c:\Progra~1\Borland\CBuilder3
COMPBASE16=c:\bc
!endif
!if $(BUILDER_VERSION) == 4
COMPBASE=c:\Progra~1\Borland\CBuilder4
COMPBASE16=c:\bc
!endif
!if $(BUILDER_VERSION) == 5
COMPBASE=c:\Borland\BCC55
#COMPBASE16=$(COMPBASE)
!endif
!endif

COMPDIR=$(COMPBASE)\bin
INCDIR=$(COMPBASE)\include
LIBDIR=$(COMPBASE)\lib
COMP=$(COMPDIR)\bcc32
COMPCPP=$(COMP)
RCOMP=$(COMPDIR)\brcc32

!if $(BUILDER_VERSION) == 0
COMPAUX=$(COMPDIR)\bcc
!else
COMPAUX=$(COMPDIR)\bcc32
!endif

!if $(BUILDER_VERSION) == 4
LINK=$(COMPDIR)\ilink32
!endif
!if $(BUILDER_VERSION) == 5
LINK=$(COMPDIR)\ilink32
!endif

# If you don't have an assembler, set USE_ASM=0.  Otherwise, set USE_ASM=1,
# and set ASM to the name of the assembler you are using.  This can be
# a full path name if you want.  Normally it will be masm or tasm.

USE_ASM=0
ASM=tasm

# Define the processor architecture. (always i386)

CPU_FAMILY=i386

# Define the processor (CPU) type.  (386, 486 or 586)

CPU_TYPE=586

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

FPU_TYPE=387

# Define the .dev module that implements thread and synchronization
# primitives for this platform.  Don't change this unless you really know
# what you're doing.

SYNC=winsync

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.

FEATURE_DEVS=$(PSD)psl3.dev $(PSD)pdf.dev $(PSD)dpsnext.dev $(PSD)ttfont.dev $(PSD)epsf.dev $(PSD)mshandle.dev $(PSD)mspoll.dev $(GLD)pipe.dev $(PSD)fapi.dev

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
# devs.mak and contrib.mak for the list of available devices.

DEVICE_DEVS=$(DD)display.dev $(DD)mswindll.dev $(DD)mswinpr2.dev
DEVICE_DEVS2=$(DD)epson.dev $(DD)eps9high.dev $(DD)eps9mid.dev $(DD)epsonc.dev $(DD)ibmpro.dev
DEVICE_DEVS3=$(DD)deskjet.dev $(DD)djet500.dev $(DD)laserjet.dev $(DD)ljetplus.dev $(DD)ljet2p.dev
DEVICE_DEVS4=$(DD)cdeskjet.dev $(DD)cdjcolor.dev $(DD)cdjmono.dev $(DD)cdj550.dev
DEVICE_DEVS5=$(DD)djet500c.dev $(DD)declj250.dev $(DD)lj250.dev
DEVICE_DEVS6=$(DD)st800.dev $(DD)stcolor.dev $(DD)bj10e.dev $(DD)bj200.dev
DEVICE_DEVS7=$(DD)t4693d2.dev $(DD)t4693d4.dev $(DD)t4693d8.dev $(DD)tek4696.dev
DEVICE_DEVS8=$(DD)pcxmono.dev $(DD)pcxgray.dev $(DD)pcx16.dev $(DD)pcx256.dev $(DD)pcx24b.dev
DEVICE_DEVS9=$(DD)pbm.dev $(DD)pbmraw.dev $(DD)pgm.dev $(DD)pgmraw.dev $(DD)pgnm.dev $(DD)pgnmraw.dev
DEVICE_DEVS10=$(DD)tiffcrle.dev $(DD)tiffg3.dev $(DD)tiffg32d.dev $(DD)tiffg4.dev $(DD)tifflzw.dev $(DD)tiffpack.dev
DEVICE_DEVS11=$(DD)bmpmono.dev $(DD)bmp16.dev $(DD)bmp256.dev $(DD)bmp16m.dev $(DD)tiff12nc.dev $(DD)tiff24nc.dev
DEVICE_DEVS12=$(DD)psmono.dev $(DD)bit.dev $(DD)bitrgb.dev $(DD)bitcmyk.dev
DEVICE_DEVS13=$(DD)pngmono.dev $(DD)pnggray.dev $(DD)png16.dev $(DD)png256.dev $(DD)png16m.dev $(DD)pngalpha.dev
DEVICE_DEVS14=$(DD)jpeg.dev $(DD)jpeggray.dev  $(DD)jpegcmyk.dev
DEVICE_DEVS15=$(DD)pdfwrite.dev $(DD)pswrite.dev $(DD)ps2write.dev $(DD)epswrite.dev $(DD)pxlmono.dev $(DD)pxlcolor.dev
# Overflow for DEVS3,4,5,6,9
DEVICE_DEVS16=$(DD)ljet3.dev $(DD)ljet3d.dev $(DD)ljet4.dev $(DD)ljet4d.dev
DEVICE_DEVS17=$(DD)pj.dev $(DD)pjxl.dev $(DD)pjxl300.dev
DEVICE_DEVS18=$(DD)jetp3852.dev $(DD)r4081.dev $(DD)lbp8.dev $(DD)uniprint.dev
DEVICE_DEVS19=$(DD)m8510.dev $(DD)necp6.dev $(DD)bjc600.dev $(DD)bjc800.dev
DEVICE_DEVS20=$(DD)pnm.dev $(DD)pnmraw.dev $(DD)ppm.dev $(DD)ppmraw.dev

# ---------------------------- End of options ---------------------------- #

# Define the name of the makefile -- used in dependencies.

MAKEFILE=$(PSSRCDIR)\bcwin32.mak
TOP_MAKEFILES=$(MAKEFILE) $(GLSRCDIR)\winlib.mak $(PSSRCDIR)\winint.mak

# Define the current directory prefix and shell invocations.

D=\\

EXP=
SH=

# Define the arguments for genconf.

CONFILES=-p %s+
CONFLDTR=-ol

# Define the generic compilation flags.

PLATOPT=

INTASM=
PCFBASM=

# Make sure we get the right default target for make.

# Rod Webster (rodw)
# CBuilder 5 does not support 16 bit compilation 
# so add conditional to skip attempts to build 16 bit version
!if $(BUILDER_VERSION) !=5
dosdefault: default $(BINDIR)\gs16spl.exe
!endif
# Define the switches for the compilers.

C_=-c
O_=-o
RO_=-fo

# Define the compilation flags.

!if $(CPU_TYPE)>500
ASMCPU=/DFOR80386 /DFOR80486
CPFLAGS=-DFOR80486 -DFOR80386
!else if $(CPU_TYPE)>400
ASMCPU=/DFOR80386 /DFOR80486
CPFLAGS=-DFOR80486 -DFOR80386
!else
ASMCPU=/DFOR80386
CPFLAGS=-DFOR80386
!endif

!if $(CPU_TYPE) >= 486 || $(FPU_TYPE) > 0
ASMFPU=/DFORFPU
!else
!if $(FPU_TYPE) < 0
ASMFPU=/DNOFPU
!else
ASMFPU=
!endif
!endif
FPFLAGS=
FPLIB=

!if $(NOPRIVATE)!=0
CP=-DNOPRIVATE
!else
CP=
!endif

!if $(DEBUG)!=0
CD=-DDEBUG
!else
CD=
!endif

!if $(TDEBUG)!=0
CT=-v
LCT=-v -m -s
CO=    # no optimization when debugging
ASMDEBUG=/DDEBUG
!else
CT=
LCT=
CO=-Z -O2
!endif

!if $(DEBUG)!=0 || $(TDEBUG)!=0
CS=-N
!else
CS=
!endif

!if $(MULTITHREAD)!=0
CMT=-tWM
CLIB=cw32mt.lib
!else
CMT=
CLIB=cw32.lib
!endif

# Specify function prolog type
COMPILE_FOR_DLL=-WDE
COMPILE_FOR_EXE=-WE
COMPILE_FOR_CONSOLE_EXE=-WC

# The -tWM is for multi-thread-safe compilation.
GENOPT=$(CP) $(CD) $(CT) $(CS) $(CMT)

CCFLAGS0=$(GENOPT) $(PLATOPT) $(CPFLAGS) $(FPFLAGS) $(CFLAGS) $(XCFLAGS)
CCFLAGS=$(CCFLAGS0)
CC=$(COMP) @$(GLGENDIR)\ccf32.tr
CPP=$(COMPCPP) @$(GLGENDIR)\ccf32.tr
!if $(MAKEDLL)
WX=$(COMPILE_FOR_DLL)
!else
WX=$(COMPILE_FOR_EXE)
!endif
CC_WX=$(CC) $(WX)
CC_=$(CC_WX) $(CO)
CC_D=$(CC_WX)
CC_INT=$(CC_WX)
CC_NO_WARN=$(CC_)

# No additional flags are needed for Windows compilation.
CCWINFLAGS=

# Define the files to be removed by `make clean'.
# nmake expands macros when encountered, not when used,
# so this must precede the !include statements.

# ****** HACK ****** *.tr is still created in the current directory.
BEGINFILES2=$(BINDIR)\gs16spl.exe *.tr

# Include the generic makefiles.

!include $(GLSRCDIR)\winlib.mak
!include $(PSSRCDIR)\winint.mak

# -------------------------- Auxiliary programs --------------------------- #

# Compiler for auxiliary programs

!if $(BUILDER_VERSION) == 0
CCAUX=$(COMPAUX) -ml -I$(GLSRCDIR) -I$(INCDIR) -L$(LIBDIR) -n$(AUXGENDIR) -O
!else
CCAUX=$(COMPAUX) -I$(GLSRCDIR) -I$(INCDIR) -L$(LIBDIR) -n$(AUXGENDIR) -O
!endif
CCAUX_TAIL=

$(GLGENDIR)\ccf32.tr: $(TOP_MAKEFILES)
	echo -a1 -d -r -w-par -w-stu -G -N -X -I$(INCDIR) > $(GLGENDIR)\ccf32.tr
	echo $(CCFLAGS0) -DCHECK_INTERRUPTS >> $(GLGENDIR)\ccf32.tr

$(ECHOGS_XE): $(GLSRCDIR)\echogs.c
	$(CCAUX) $(GLSRCDIR)\echogs.c $(CCAUX_TAIL)

# Since we are running in a Windows environment with a different compiler
# for the DOS utilities, we have to invoke genarch by hand.
# For unfathomable reasons, the 'win' program requires /, not \,
# in the name of the program to be run, and apparently also in any
# file names passed on the command line (?!).
$(GENARCH_XE): $(GLSRCDIR)\genarch.c $(GENARCH_DEPS) $(GLGENDIR)\ccf32.tr
	$(COMP) -I$(GLSRCDIR) -I$(INCDIR) -L$(LIBDIR) -n$(AUXGENDIR) -O $(GLSRCDIR)\genarch.c
	echo win $(AUXGENDIR)/genarch $(GLGENDIR)/arch.h >_genarch.bat
	echo ***** Run "_genarch.bat", then continue make. *****

$(GENCONF_XE): $(GLSRCDIR)\genconf.c $(GENCONF_DEPS)
	$(CCAUX) $(GLSRCDIR)\genconf.c $(CCAUX_TAIL)

$(GENDEV_XE): $(GLSRCDIR)\gendev.c $(GENDEV_DEPS)
	$(CCAUX) $(GLSRCDIR)\gendev.c $(CCAUX_TAIL)

$(GENHT_XE): $(PSSRCDIR)\genht.c $(GENHT_DEPS)
	$(CCAUX) $(GENHT_CFLAGS) $(PSSRCDIR)\genht.c $(CCAUX_TAIL)

$(GENINIT_XE): $(PSSRCDIR)\geninit.c $(GENINIT_DEPS)
	$(CCAUX) $(PSSRCDIR)\geninit.c $(CCAUX_TAIL)

# -------------------------------- Library -------------------------------- #

# See winlib.mak

# ----------------------------- Main program ------------------------------ #

LIBCTR=$(PSGEN)libc32.tr
GSCONSOLE_XE=$(BINDIR)\$(GSCONSOLE).exe
GSDLL_DLL=$(BINDIR)\$(GSDLL).dll

$(LIBCTR): $(TOP_MAKEFILES) $(ECHOGS_XE)
	echo $(LIBDIR)\import32.lib $(LIBDIR)\$(CLIB) >$(LIBCTR)

!if $(BUILDER_VERSION) == 0
# Borland C++ 4.5 will not compile the setup program,
# since a later Windows header file is required.
SETUP_TARGETS=
!else
SETUP_TARGETS=$(SETUP_XE) $(UNINSTALL_XE)
!endif

!if $(MAKEDLL)
# The graphical small EXE loader
$(GS_XE): $(GSDLL_DLL)  $(DWOBJ) $(GSCONSOLE_XE)\
 $(GS_OBJ).res $(PSSRCDIR)\dwmain32.def $(SETUP_TARGETS)
	$(LINK) /L$(LIBDIR) /Tpe /aa $(LCT) @&&!
$(LIBDIR)\c0w32 +
$(DWOBJ) +
,$(GS_XE),$(PSOBJ)$(GS), +
$(LIBDIR)\import32 +
$(LIBDIR)\cw32, +
$(PSSRCDIR)\dwmain32.def, +
$(GS_OBJ).res
!

# The console mode small EXE loader
!if $(BUILDER_VERSION) == 5
$(GSCONSOLE_XE): $(OBJC) $(GS_OBJ).res $(PSSRCDIR)\dw32c.def
	$(LINK) /L$(LIBDIR) /Tpe /ap $(LCT) $(DEBUGLINK) @&&!
$(LIBDIR)\c0x32 +
$(OBJC) +
,$(GSCONSOLE_XE),$(PSOBJ)$(GSCONSOLE), +
$(LIBDIR)\import32 +
$(LIBDIR)\cw32mt, +
$(PSSRCDIR)\dw32c.def, +
$(GS_OBJ).res
!
!else

$(GSCONSOLE_XE): $(OBJC) $(GS_OBJ).res $(PSSRCDIR)\dw32c.def
	$(LINK) /L$(LIBDIR) /Tpe /ap $(LCT) $(DEBUGLINK) @&&!
$(LIBDIR)\c0w32 +
$(OBJC) +
,$(GSCONSOLE_XE),$(PSOBJ)$(GSCONSOLE), +
$(LIBDIR)\import32 +
$(LIBDIR)\cw32, +
$(PSSRCDIR)\dw32c.def, +
$(GS_OBJ).res
!
!endif

# The big DLL
$(GSDLL_DLL): $(GS_ALL) $(DEVS_ALL) $(PSOBJ)gsdll.$(OBJ)\
 $(GSDLL_OBJ).res $(PSSRCDIR)\gsdll32.def
	-del $(PSGEN)gswin32.tr
	copy $(ld_tr) $(PSGEN)gswin32.tr
	echo $(LIBDIR)\c0d32 $(PSOBJ)gsdll + >> $(PSGEN)gswin32.tr
	$(LINK) /L$(LIBDIR) $(LCT) /Tpd /aa @$(PSGEN)gswin32.tr $(INTASM) ,$(GSDLL_DLL),$(PSOBJ)$(GSDLL) @$(LIBCTR),$(PSSRCDIR)\gsdll32.def,$(GSDLL_OBJ).res

!else
# The big graphical EXE
$(GS_XE):   $(GSCONSOLE_XE) $(GS_ALL) $(DEVS_ALL)\
 $(PSOBJ)gsdll.$(OBJ) $(DWOBJNO) $(GS_OBJ).res $(PSSRCDIR)\dwmain32.def
	-del $(PSGEN)gswin32.tr
	copy $(ld_tr) $(PSGEN)gswin32.tr
	echo $(LIBDIR)\c0w32 $(PSOBJ)gsdll + >> $(PSGEN)gswin32.tr
	echo $(DWOBJNO) $(INTASM) >> $(PSGEN)gswin32.tr
	$(LINK) /L$(LIBDIR) $(LCT) /Tpe /aa @$(PSGEN)gswin32.tr ,$(GS_XE),$(PSOBJ)$(GS) @$(LIBCTR),$(PSSRCDIR)\dwmain32.def,$(GS_OBJ).res

# The big console mode EXE
$(GSCONSOLE_XE):  $(GS_ALL) $(DEVS_ALL)\
 $(PSOBJ)gsdll.$(OBJ) $(OBJCNO) $(GS_OBJ).res $(PSSRCDIR)\dw32c.def
	-del $(PSGEN)gswin32.tr
	copy $(ld_tr) $(PSGEN)gswin32.tr
	echo $(LIBDIR)\c0w32 $(PSOBJ)gsdll + >> $(PSGEN)gswin32.tr
	echo $(OBJCNO) $(INTASM) >> $(PSGEN)gswin32.tr
	$(LINK) /L$(LIBDIR) $(LCT) /Tpe /ap @$(PSGEN)gswin32.tr ,$(GSCONSOLE_XE),$(PSOBJ)$(GSCONSOLE) @$(LIBCTR),$(PSSRCDIR)\dw32c.def,$(GS_OBJ).res
!endif

# Access to 16 spooler from Win32s
# Rod Webster (rodw)
# CBuilder 5 does not support 16 bit compilation 
# so add conditional to skip attempts to build 16 bit version
!if $(BUILDER_VERSION !=5)

GSSPL_XE=$(BINDIR)\gs16spl.exe

$(GSSPL_XE): $(GLSRCDIR)\gs16spl.c $(GLSRCDIR)\gs16spl.rc $(GLGENDIR)/gswin.ico
	$(ECHOGS_XE) -w $(GLGEN)_spl.rc -x 23 define -s gstext_ico $(GLGENDIR)/gswin.ico
	$(ECHOGS_XE) -a $(GLGEN)_spl.rc -x 23 define -s gsgraph_ico $(GLGENDIR)/gswin.ico
	$(ECHOGS_XE) -a $(GLGEN)_spl.rc -R $(GLSRC)gs16spl.rc
	$(COMPBASE16)\bin\bcc -W -ms -v -I$(COMPBASE16)\include $(GLO_)gs16spl.obj -c $(GLSRCDIR)\gs16spl.c
	$(COMPBASE16)\bin\brcc -i$(COMPBASE16)\include -r -fo$(GLOBJ)gs16spl.res $(GLGEN)_spl.rc
	$(COMPBASE16)\bin\tlink /Twe /c /m /s /l @&&!
$(COMPBASE16)\lib\c0ws +
$(GLOBJ)gs16spl.obj, +
$(GSSPL_XE),$(GLOBJ)gs16spl, +
$(COMPBASE16)\lib\import +
$(COMPBASE16)\lib\mathws +
$(COMPBASE16)\lib\cws, +
$(GLSRCDIR)\gs16spl.def
!
	$(COMPBASE16)\bin\rlink -t $(GLOBJ)gs16spl.res $(GSSPL_XE)
!endif

# ---------------------- Setup and uninstall programs ---------------------- #

!if $(MAKEDLL)

$(SETUP_XE): $(PSOBJ)dwsetup.obj $(PSOBJ)dwinst.obj $(PSOBJ)dwsetup.res $(PSSRC)dwsetup.def
	$(LINK) /Tpe /aa $(LCT) $(DEBUGLINK) -L$(LIBDIR) @&&!
$(LIBDIR)\c0w32 +
$(PSOBJ)dwsetup.obj $(PSOBJ)dwinst.obj +
,$(SETUP_XE),$(PSOBJ)dwsetup, +
$(LIBDIR)\import32 +
$(LIBDIR)\ole2w32 +
$(LIBDIR)\cw32, +
$(PSSRCDIR)\dwsetup.def, +
$(PSOBJ)dwsetup.res
!

$(UNINSTALL_XE): $(PSOBJ)dwuninst.obj $(PSOBJ)dwuninst.res $(PSSRC)dwuninst.def
	$(LINK) /Tpe /aa $(LCT) $(DEBUGLINK) -L$(LIBDIR) @&&!
$(LIBDIR)\c0w32 +
$(PSOBJ)dwuninst.obj +
,$(UNINSTALL_XE),$(PSOBJ)dwuninst, +
$(LIBDIR)\import32 +
$(LIBDIR)\ole2w32 +
$(LIBDIR)\cw32, +
$(PSSRCDIR)\dwuninst.def, +
$(PSOBJ)dwuninst.res
!


!endif


# end of makefile

