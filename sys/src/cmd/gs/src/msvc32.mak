#    Copyright (C) 1991-2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: msvc32.mak,v 1.2 2000/03/10 15:48:58 lpd Exp $
# makefile for 32-bit Microsoft Visual C++, Windows NT or Windows 95 platform.
#
# All configurable options are surrounded by !ifndef/!endif to allow 
# preconfiguration from within another makefile.
#
# Optimization /O2 seems OK with MSVC++ 4.1, but not with 5.0.
# Created 1997-01-24 by Russell Lang from MSVC++ 2.0 makefile.
# Enhanced 97-05-15 by JD
# Common code factored out 1997-05-22 by L. Peter Deutsch.
# Made pre-configurable by JD 6/4/98
# Revised to use subdirectories 1998-11-13 by lpd.

# Note: If you are planning to make self-extracting executables,
# see winint.mak to find out about third-party software you will need.

# ------------------------------- Options ------------------------------- #

###### This section is the only part of the file you should need to edit.

# ------ Generic options ------ #

# Define the directory for the final executable, and the
# source, generated intermediate file, and object directories
# for the graphics library (GL) and the PostScript/PDF interpreter (PS).

!ifndef BINDIR
BINDIR=.\bin
!endif
!ifndef GLSRCDIR
GLSRCDIR=.\src
!endif
!ifndef GLGENDIR
GLGENDIR=.\obj
!endif
!ifndef GLOBJDIR
GLOBJDIR=.\obj
!endif
!ifndef PSSRCDIR
PSSRCDIR=.\src
!endif
!ifndef PSLIBDIR
PSLIBDIR=.\lib
!endif
!ifndef PSGENDIR
PSGENDIR=.\obj
!endif
!ifndef PSOBJDIR
PSOBJDIR=.\obj
!endif

# Define the root directory for Ghostscript installation.

!ifndef AROOTDIR
AROOTDIR=c:/Aladdin
!endif
!ifndef GSROOTDIR
GSROOTDIR=$(AROOTDIR)/gs$(GS_DOT_VERSION)
!endif

# Define the directory that will hold documentation at runtime.

!ifndef GS_DOCDIR
GS_DOCDIR=$(GSROOTDIR)/doc
!endif

# Define the default directory/ies for the runtime initialization and
# font files.  Separate multiple directories with ';'.
# Use / to indicate directories, not \.
# MSVC will not allow \'s here because it sees '\;' CPP-style as an
# illegal escape.

!ifndef GS_LIB_DEFAULT
GS_LIB_DEFAULT=$(GSROOTDIR)/lib;$(AROOTDIR)/fonts
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

# Setting DEBUG=1 includes debugging features (-Z switch) in the code.
# Code runs substantially slower even if no debugging switches are set,
# and is also substantially larger.

!ifndef DEBUG
DEBUG=0
!endif

# Setting TDEBUG=1 includes symbol table information for the debugger,
# and also enables stack checking.  Code is substantially slower and larger.

# NOTE: The MSVC++ 5.0 compiler produces incorrect output code with TDEBUG=0.
# Leave TDEBUG set to 1.

!ifndef TDEBUG
TDEBUG=1
!endif

# Setting NOPRIVATE=1 makes private (static) procedures and variables public,
# so they are visible to the debugger and profiler.
# No execution time or space penalty, just larger .OBJ and .EXE files.

!ifndef NOPRIVATE
NOPRIVATE=0
!endif

# Define the name of the executable file.

!ifndef GS
GS=gswin32
!endif
!ifndef GSCONSOLE
GSCONSOLE=gswin32c
!endif
!ifndef GSDLL
GSDLL=gsdll32
!endif

!ifndef BUILD_TIME_GS
# Define the name of a pre-built executable that can be invoked at build
# time.  Currently, this is only needed for compiled fonts.  The usual
# alternatives are:
#   - the standard name of Ghostscript on your system (typically `gs'):
BUILD_TIME_GS=gswin32c
#   - the name of the executable you are building now.  If you choose this
# option, then you must build the executable first without compiled fonts,
# and then again with compiled fonts.
#BUILD_TIME_GS=$(BINDIR)\$(GS) -I$(PSLIBDIR)
!endif

# To build two small executables and a large DLL use MAKEDLL=1
# To build two large executables use MAKEDLL=0

!ifndef MAKEDLL
MAKEDLL=1
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
PVERSION=10005
!endif

# Define the directory where the zlib sources are stored.
# See zlib.mak for more information.

!ifndef ZSRCDIR
ZSRCDIR=zlib
!endif

# Define any other compilation flags.

!ifndef CFLAGS
CFLAGS=
!endif

# Do not edit the next group of lines.

#!include $(COMMONDIR)\msvcdefs.mak
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

# Define which major version of MSVC is being used
# (currently, 4 & 5 are supported).

!ifndef MSVC_VERSION 
MSVC_VERSION=5
!endif

# Define the drive, directory, and compiler name for the Microsoft C files.
# COMPDIR contains the compiler and linker (normally \msdev\bin).
# INCDIR contains the include files (normally \msdev\include).
# LIBDIR contains the library files (normally \msdev\lib).
# COMP is the full C compiler path name (normally \msdev\bin\cl).
# COMPCPP is the full C++ compiler path name (normally \msdev\bin\cl).
# COMPAUX is the compiler name for DOS utilities (normally \msdev\bin\cl).
# RCOMP is the resource compiler name (normallly \msdev\bin\rc).
# LINK is the full linker path name (normally \msdev\bin\link).
# Note that when INCDIR and LIBDIR are used, they always get a '\' appended,
#   so if you want to use the current directory, use an explicit '.'.

!if $(MSVC_VERSION) == 4
! ifndef DEVSTUDIO
DEVSTUDIO=c:\msdev
! endif
COMPBASE=$(DEVSTUDIO)
SHAREDBASE=$(DEVSTUDIO)
!endif

!if $(MSVC_VERSION) == 5
! ifndef DEVSTUDIO
#DEVSTUDIO=c:\program files\devstudio
DEVSTUDIO=c:\progra~1\devstu~1
! endif
COMPBASE=$(DEVSTUDIO)\VC
SHAREDBASE=$(DEVSTUDIO)\SharedIDE
!endif

!if $(MSVC_VERSION) == 6
! ifndef DEVSTUDIO
#DEVSTUDIO=c:\program files\microsoft visual studio
DEVSTUDIO=c:\progra~1\micros~2
! endif
COMPBASE=$(DEVSTUDIO)\VC98
SHAREDBASE=$(DEVSTUDIO)\Common\MSDev98
!endif

COMPDIR=$(COMPBASE)\bin
LINKDIR=$(COMPBASE)\bin
RCDIR=$(SHAREDBASE)\bin
INCDIR=$(COMPBASE)\include
LIBDIR=$(COMPBASE)\lib
COMP=$(COMPDIR)\cl
COMPCPP=$(COMP)
COMPAUX=$(COMPDIR)\cl
RCOMP=$(RCDIR)\rc
LINK=$(LINKDIR)\link

# Define the processor architecture. (i386, ppc, alpha)

!ifndef CPU_FAMILY
CPU_FAMILY=i386
#CPU_FAMILY=ppc
#CPU_FAMILY=alpha  # not supported yet - we need someone to tweak
!endif

# Define the processor (CPU) type. Allowable values depend on the family:
#   i386: 386, 486, 586
#   ppc: 601, 604, 620
#   alpha: not currently used.

!ifndef CPU_TYPE
CPU_TYPE=486
#CPU_TYPE=601
!endif

!if "$(CPU_FAMILY)"=="i386"

# Intel(-compatible) processors are the only ones for which the CPU type
# doesn't indicate whether a math coprocessor is present.
# For Intel processors only, define the math coprocessor (FPU) type.
# Options are -1 (optimize for no FPU), 0 (optimize for FPU present,
# but do not require a FPU), 87, 287, or 387.
# If you have a 486 or Pentium CPU, you should normally set FPU_TYPE to 387,
# since most of these CPUs include the equivalent of an 80387 on-chip;
# however, the 486SX and the Cyrix 486SLC do not have an on-chip FPU, so if
# you have one of these CPUs and no external FPU, set FPU_TYPE to -1 or 0.
# An xx87 option means that the executable will run only if a FPU
# of that type (or higher) is available: this is NOT currently checked
# at runtime.

! ifndef FPU_TYPE
FPU_TYPE=0
! endif

!endif

# Define the .dev module that implements thread and synchronization
# primitives for this platform.  Don't change this unless you really know
# what you're doing.

!ifndef SYNC
SYNC=winsync
!endif

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.

!ifndef FEATURE_DEVS
FEATURE_DEVS=$(PSD)psl3.dev $(PSD)pdf.dev $(PSD)dpsnext.dev $(PSD)ttfont.dev $(PSD)mshandle.dev $(GLD)pipe.dev
!endif

# Choose whether to compile the .ps initialization files into the executable.
# See gs.mak for details.

!ifndef COMPILE_INITS
COMPILE_INITS=0
!endif

# Choose whether to store band lists on files or in memory.
# The choices are 'file' or 'memory'.

!ifndef BAND_LIST_STORAGE
BAND_LIST_STORAGE=file
!endif

# Choose which compression method to use when storing band lists in memory.
# The choices are 'lzw' or 'zlib'.  lzw is not recommended, because the
# LZW-compatible code in Ghostscript doesn't actually compress its input.

!ifndef BAND_LIST_COMPRESSOR
BAND_LIST_COMPRESSOR=zlib
!endif

# Choose the implementation of file I/O: 'stdio', 'fd', or 'both'.
# See gs.mak and sfxfd.c for more details.

!ifndef FILE_IMPLEMENTATION
FILE_IMPLEMENTATION=stdio
!endif

# Choose the device(s) to include.  See devs.mak for details,
# devs.mak and contrib.mak for the list of available devices.

!ifndef DEVICE_DEVS
DEVICE_DEVS=$(DD)mswindll.dev $(DD)mswinprn.dev $(DD)mswinpr2.dev
DEVICE_DEVS2=$(DD)epson.dev $(DD)eps9high.dev $(DD)eps9mid.dev $(DD)epsonc.dev $(DD)ibmpro.dev
DEVICE_DEVS3=$(DD)deskjet.dev $(DD)djet500.dev $(DD)laserjet.dev $(DD)ljetplus.dev $(DD)ljet2p.dev
DEVICE_DEVS4=$(DD)cdeskjet.dev $(DD)cdjcolor.dev $(DD)cdjmono.dev $(DD)cdj550.dev
DEVICE_DEVS5=$(DD)djet500c.dev $(DD)declj250.dev $(DD)lj250.dev
DEVICE_DEVS6=$(DD)st800.dev $(DD)stcolor.dev $(DD)bj10e.dev $(DD)bj200.dev
DEVICE_DEVS7=$(DD)t4693d2.dev $(DD)t4693d4.dev $(DD)t4693d8.dev $(DD)tek4696.dev
DEVICE_DEVS8=$(DD)pcxmono.dev $(DD)pcxgray.dev $(DD)pcx16.dev $(DD)pcx256.dev $(DD)pcx24b.dev $(DD)pcxcmyk.dev
DEVICE_DEVS9=$(DD)pbm.dev $(DD)pbmraw.dev $(DD)pgm.dev $(DD)pgmraw.dev $(DD)pgnm.dev $(DD)pgnmraw.dev
DEVICE_DEVS10=$(DD)tiffcrle.dev $(DD)tiffg3.dev $(DD)tiffg32d.dev $(DD)tiffg4.dev $(DD)tifflzw.dev $(DD)tiffpack.dev
DEVICE_DEVS11=$(DD)bmpmono.dev $(DD)bmpgray.dev $(DD)bmp16.dev $(DD)bmp256.dev $(DD)bmp16m.dev $(DD)tiff12nc.dev $(DD)tiff24nc.dev
DEVICE_DEVS12=$(DD)psmono.dev $(DD)bit.dev $(DD)bitrgb.dev $(DD)bitcmyk.dev
DEVICE_DEVS13=$(DD)pngmono.dev $(DD)pnggray.dev $(DD)png16.dev $(DD)png256.dev $(DD)png16m.dev
DEVICE_DEVS14=$(DD)jpeg.dev $(DD)jpeggray.dev
DEVICE_DEVS15=$(DD)pdfwrite.dev $(DD)pswrite.dev $(DD)epswrite.dev $(DD)pxlmono.dev $(DD)pxlcolor.dev
# Overflow for DEVS3,4,5,6,9
DEVICE_DEVS16=$(DD)ljet3.dev $(DD)ljet3d.dev $(DD)ljet4.dev $(DD)ljet4d.dev
DEVICE_DEVS17=$(DD)pj.dev $(DD)pjxl.dev $(DD)pjxl300.dev
DEVICE_DEVS18=$(DD)jetp3852.dev $(DD)r4081.dev $(DD)lbp8.dev $(DD)uniprint.dev
DEVICE_DEVS19=$(DD)m8510.dev $(DD)necp6.dev $(DD)bjc600.dev $(DD)bjc800.dev
DEVICE_DEVS20=$(DD)pnm.dev $(DD)pnmraw.dev $(DD)ppm.dev $(DD)ppmraw.dev
!endif

# ---------------------------- End of options ---------------------------- #

# Derive values for FPU_TYPE for non-Intel processors.

!if "$(CPU_FAMILY)"=="ppc"
! if $(CPU_TYPE)>601
FPU_TYPE=2
! else
FPU_TYPE=1
! endif
!endif

!if "$(CPU_FAMILY)"=="alpha"
# *** alpha *** This needs fixing
FPU_TYPE=1
!endif

# Define the name of the makefile -- used in dependencies.

MAKEFILE=$(GLSRCDIR)\msvc32.mak
TOP_MAKEFILES=$(MAKEFILE) $(GLSRCDIR)\msvccmd.mak $(GLSRCDIR)\msvctail.mak $(GLSRCDIR)\winlib.mak $(GLSRCDIR)\winint.mak

# Define the files to be removed by `make clean'.
# nmake expands macros when encountered, not when used,
# so this must precede the !include statements.

BEGINFILES2=$(GLOBJDIR)\*.exp $(GLOBJDIR)\*.ilk $(GLOBJDIR)\*.pdb $(GLOBJDIR)\*.lib $(GLGENDIR)\lib32.rsp $(GLOBJDIR)\dw*.res $(SETUP_XE) $(UNINSTALL_XE) $(BINDIR)\*.exp $(BINDIR)\*.ilk $(BINDIR)\*.pdb $(BINDIR)\*.lib

!include $(GLSRCDIR)\msvccmd.mak
!include $(GLSRCDIR)\winlib.mak
!include $(GLSRCDIR)\msvctail.mak
!include $(GLSRCDIR)\winint.mak

# ----------------------------- Main program ------------------------------ #

GSCONSOLE_XE=$(BINDIR)\$(GSCONSOLE).exe
GSDLL_DLL=$(BINDIR)\$(GSDLL).dll

$(GLGEN)lib32.rsp: $(TOP_MAKEFILES)
	echo /NODEFAULTLIB:LIBC.lib > $(GLGEN)lib32.rsp
	echo $(LIBDIR)\libcmt.lib >> $(GLGEN)lib32.rsp

!if $(MAKEDLL)
# The graphical small EXE loader
$(GS_XE): $(GSDLL_DLL)  $(DWOBJ) $(GSCONSOLE_XE) $(SETUP_XE) $(UNINSTALL_XE)
	echo /SUBSYSTEM:WINDOWS > $(GLGEN)gswin32.rsp
	echo /DEF:$(GLSRCDIR)\dwmain32.def /OUT:$(GS_XE) >> $(GLGEN)gswin32.rsp
        $(LINK) $(LCT) @$(GLGEN)gswin32.rsp $(DWOBJ) @$(LIBCTR) $(GS_OBJ).res
	del $(GLGEN)gswin32.rsp

# The console mode small EXE loader
$(GSCONSOLE_XE): $(OBJC) $(GS_OBJ).res $(GLSRCDIR)\dw32c.def
	echo /SUBSYSTEM:CONSOLE > $(GLGEN)gswin32.rsp
	echo  /DEF:$(GLSRCDIR)\dw32c.def /OUT:$(GSCONSOLE_XE) >> $(GLGEN)gswin32.rsp
	$(LINK_SETUP)
        $(LINK) $(LCT) @$(GLGEN)gswin32.rsp $(OBJC) @$(LIBCTR) $(GS_OBJ).res
	del $(GLGEN)gswin32.rsp

# The big DLL
$(GSDLL_DLL): $(GS_ALL) $(DEVS_ALL) $(GLOBJ)gsdll.$(OBJ) $(GSDLL_OBJ).res $(GLGEN)lib32.rsp
	echo /DLL /DEF:$(GLSRCDIR)\gsdll32.def /OUT:$(GSDLL_DLL) > $(GLGEN)gswin32.rsp
	$(LINK_SETUP)
        $(LINK) $(LCT) @$(GLGEN)gswin32.rsp $(GLOBJ)gsdll @$(ld_tr) $(INTASM) @$(GLGEN)lib.tr @$(GLGEN)lib32.rsp @$(LIBCTR) $(GSDLL_OBJ).res
	del $(GLGEN)gswin32.rsp

!else
# The big graphical EXE
$(GS_XE): $(GSCONSOLE_XE) $(GS_ALL) $(DEVS_ALL) $(GLOBJ)gsdll.$(OBJ) $(DWOBJNO) $(GSDLL_OBJ).res $(GLSRCDIR)\dwmain32.def $(GLGEN)lib32.rsp
	copy $(ld_tr) $(GLGEN)gswin32.tr
	echo $(GLOBJ)dwnodll.obj >> $(GLGEN)gswin32.tr
	echo $(GLOBJ)dwimg.obj >> $(GLGEN)gswin32.tr
	echo $(GLOBJ)dwmain.obj >> $(GLGEN)gswin32.tr
	echo $(GLOBJ)dwtext.obj >> $(GLGEN)gswin32.tr
	echo /DEF:$(GLSRCDIR)\dwmain32.def /OUT:$(GS_XE) > $(GLGEN)gswin32.rsp
	$(LINK_SETUP)
        $(LINK) $(LCT) @$(GLGEN)gswin32.rsp $(GLOBJ)gsdll @$(GLGEN)gswin32.tr @$(LIBCTR) $(INTASM) @$(GLGEN)lib.tr @$(GLGEN)lib32.rsp $(GSDLL_OBJ).res
	del $(GLGEN)gswin32.tr
	del $(GLGEN)gswin32.rsp

# The big console mode EXE
$(GSCONSOLE_XE): $(GS_ALL) $(DEVS_ALL) $(GLOBJ)gsdll.$(OBJ) $(OBJCNO) $(GS_OBJ).res $(GLSRCDIR)\dw32c.def $(GLGEN)lib32.rsp
	copy $(ld_tr) $(GLGEN)gswin32c.tr
	echo $(GLOBJ)dwnodllc.obj >> $(GLGEN)gswin32c.tr
	echo $(GLOBJ)dwmainc.obj >> $(GLGEN)gswin32c.tr
	echo /SUBSYSTEM:CONSOLE > $(GLGEN)gswin32.rsp
	echo /DEF:$(GLSRCDIR)\dw32c.def /OUT:$(GSCONSOLE_XE) >> $(GLGEN)gswin32.rsp
	$(LINK_SETUP)
        $(LINK) $(LCT) @$(GLGEN)gswin32.rsp $(GLOBJ)gsdll @$(GLGEN)gswin32c.tr @$(LIBCTR) $(INTASM) @$(GLGEN)lib.tr @$(GLGEN)lib32.rsp $(GS_OBJ).res
	del $(GLGEN)gswin32.rsp
	del $(GLGEN)gswin32c.tr
!endif

# ---------------------- Setup and uninstall programs ---------------------- #

!if $(MAKEDLL)

$(SETUP_XE): $(GLOBJ)dwsetup.obj $(GLOBJ)dwinst.obj $(GLOBJ)dwsetup.res $(GLSRC)dwsetup.def
	echo /DEF:$(GLSRC)dwsetup.def /OUT:$(SETUP_XE) > $(GLGEN)dwsetup.rsp
	echo $(GLOBJ)dwsetup.obj $(GLOBJ)dwinst.obj >> $(GLGEN)dwsetup.rsp
	copy $(LIBCTR) $(GLGEN)dwsetup.tr
        echo $(LIBDIR)\ole32.lib >> $(GLGEN)dwsetup.tr
        echo $(LIBDIR)\uuid.lib >> $(GLGEN)dwsetup.tr
	$(LINK_SETUP)
        $(LINK) $(LCT) @$(GLGEN)dwsetup.rsp @$(GLGEN)dwsetup.tr $(GLOBJ)dwsetup.res
	del $(GLGEN)dwsetup.rsp
	del $(GLGEN)dwsetup.tr

$(UNINSTALL_XE): $(GLOBJ)dwuninst.obj $(GLOBJ)dwuninst.res $(GLSRC)dwuninst.def
	echo /DEF:$(GLSRC)dwuninst.def /OUT:$(UNINSTALL_XE) > $(GLGEN)dwuninst.rsp
	echo $(GLOBJ)dwuninst.obj >> $(GLGEN)dwuninst.rsp
	copy $(LIBCTR) $(GLGEN)dwuninst.tr
        echo $(LIBDIR)\ole32.lib >> $(GLGEN)dwuninst.tr
        echo $(LIBDIR)\uuid.lib >> $(GLGEN)dwuninst.tr
	$(LINK_SETUP)
        $(LINK) $(LCT) @$(GLGEN)dwuninst.rsp @$(GLGEN)dwuninst.tr $(GLOBJ)dwuninst.res
	del $(GLGEN)dwuninst.rsp
	del $(GLGEN)dwuninst.tr

!endif


# end of makefile
