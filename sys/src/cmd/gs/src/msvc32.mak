#    Copyright (C) 1991-2004 artofcode LLC.  All rights reserved.
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

# $Id: msvc32.mak,v 1.75 2005/08/31 05:59:58 ray Exp $
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
AROOTDIR=c:/gs
!endif
!ifndef GSROOTDIR
GSROOTDIR=$(AROOTDIR)/gs$(GS_DOT_VERSION)
!endif

# Define the directory that will hold documentation at runtime.

!ifndef GS_DOCDIR
GS_DOCDIR=$(GSROOTDIR)/doc
!endif

# Define the default directory/ies for the runtime initialization, resource and
# font files.  Separate multiple directories with ';'.
# Use / to indicate directories, not \.
# MSVC will not allow \'s here because it sees '\;' CPP-style as an
# illegal escape.

!ifndef GS_LIB_DEFAULT
GS_LIB_DEFAULT=$(GSROOTDIR)/lib;$(GSROOTDIR)/Resource;$(AROOTDIR)/fonts
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

# Setting DEBUG=1 includes debugging features in the build:
# 1. It defines the C preprocessor symbol DEBUG. The latter includes
#    tracing and self-validation code fragments into compilation.
#    Particularly it enables the -Z and -T switches in Ghostscript.
# 2. It compiles code fragments for C stack overflow checks.
# Code produced with this option is somewhat larger and runs 
# somewhat slower.

!ifndef DEBUG
DEBUG=0
!endif

# Setting TDEBUG=1 disables code optimization in the C compiler and
# includes symbol table information for the debugger.
# Code is substantially larger and slower.

# NOTE: The MSVC++ 5.0 compiler produces incorrect output code with TDEBUG=0.
# Also MSVC 6 must be service pack >= 3 to prevent INTERNAL COMPILER ERROR

# Default to 0 anyway since the execution times are so much better.
!ifndef TDEBUG
TDEBUG=0
!endif

# Setting DEBUGSYM=1 is only useful with TDEBUG=0.
# This option is for advanced developers. It includes symbol table
# information for the debugger in an optimized (release) build.
# NOTE: The debugging information generated for the optimized code may be
# significantly misleading. For general MSVC users we recommend TDEBUG=1.

!ifndef DEBUGSYM
DEBUGSYM=0
!endif


# Setting NOPRIVATE=1 makes private (static) procedures and variables public,
# so they are visible to the debugger and profiler.
# No execution time or space penalty, just larger .OBJ and .EXE files.

!ifndef NOPRIVATE
NOPRIVATE=0
!endif

# We can compile for a 32-bit or 64-bit target
# WIN32 and WIN64 are mutually exclusive.  WIN32 is the default.
!if !defined(WIN32) && !defined(Win64)
WIN32=0
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
PVERSION=10208
!endif

# Define the directory where the zlib sources are stored.
# See zlib.mak for more information.

!ifndef ZSRCDIR
ZSRCDIR=zlib
!endif

# Define the jbig2dec library source location.
# See jbig2.mak for more information.

!ifndef JBIG2SRCDIR
JBIG2SRCDIR=jbig2dec
!endif

# Define the jasper library source location.
# See jasper.mak for more information.

# Alternatively, you can build a separate DLL
# and define SHARE_JASPER=1 in src/winlib.mak

!ifndef JASPERSRCDIR
JASPERSRCDIR=jasper
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

# 1 --> Use 64 bits for gx_color_index.  This is required only for
# non standard devices or DeviceN process color model devices.
USE_LARGE_COLOR_INDEX=1

!if $(USE_LARGE_COLOR_INDEX) == 1
# Definitions to force gx_color_index to 64 bits
LARGEST_UINTEGER_TYPE=unsigned __int64
GX_COLOR_INDEX_TYPE=$(LARGEST_UINTEGER_TYPE)

CFLAGS=$(CFLAGS) /DGX_COLOR_INDEX_TYPE="$(GX_COLOR_INDEX_TYPE)"
!endif

# -W3 generates too much noise.
!ifndef WARNOPT
WARNOPT=-W2
!endif

#
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
# (currently, 4, 5, 6, 7, and 8 are supported).
# Define the minor version of MSVC, currently only
# used for Microsoft Visual Studio .NET 2003 (7.1)

#MSVC_VERSION=6
#MSVC_MINOR_VERSION=0

# Make a guess at the version of MSVC in use
# This will not work if service packs change the version numbers.

!if defined(_NMAKE_VER) && !defined(MSVC_VERSION)
!if "$(_NMAKE_VER)" == "162"
MSVC_VERSION=5
!endif
!if "$(_NMAKE_VER)" == "6.00.8168.0"
MSVC_VERSION=6
!endif
!if "$(_NMAKE_VER)" == "7.00.9466"
MSVC_VERSION=7
!endif
!if "$(_NMAKE_VER)" == "7.10.3077"
MSVC_VERSION=7
MSVC_MINOR_VERSION=1
!endif
!if "$(_NMAKE_VER)" == "8.00.40607.16"
MSVC_VERSION=8
!endif
!endif

!ifndef MSVC_VERSION
MSVC_VERSION=6
!endif
!ifndef MSVC_MINOR_VERSION
MSVC_MINOR_VERSION=0
!endif

# Define the drive, directory, and compiler name for the Microsoft C files.
# COMPDIR contains the compiler and linker (normally \msdev\bin).
# MSINCDIR contains the include files (normally \msdev\include).
# LIBDIR contains the library files (normally \msdev\lib).
# COMP is the full C compiler path name (normally \msdev\bin\cl).
# COMPCPP is the full C++ compiler path name (normally \msdev\bin\cl).
# COMPAUX is the compiler name for DOS utilities (normally \msdev\bin\cl).
# RCOMP is the resource compiler name (normallly \msdev\bin\rc).
# LINK is the full linker path name (normally \msdev\bin\link).
# Note that when MSINCDIR and LIBDIR are used, they always get a '\' appended,
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
DEVSTUDIO=C:\Program Files\Devstudio
! endif
!if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
!else
COMPBASE=$(DEVSTUDIO)\VC
SHAREDBASE=$(DEVSTUDIO)\SharedIDE
!endif
!endif

!if $(MSVC_VERSION) == 6
! ifndef DEVSTUDIO
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio
! endif
!if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
!else
COMPBASE=$(DEVSTUDIO)\VC98
SHAREDBASE=$(DEVSTUDIO)\Common\MSDev98
!endif
!endif

!if $(MSVC_VERSION) == 7
! ifndef DEVSTUDIO
!if $(MSVC_MINOR_VERSION) == 0
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio .NET
!else
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio .NET 2003
!endif
! endif
!if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
!else
COMPBASE=$(DEVSTUDIO)\Vc7
SHAREDBASE=$(DEVSTUDIO)\Vc7
!ifdef WIN64
# Windows Server 2003 DDK is needed for the 64-bit compiler
# but it won't install on Windows XP 64-bit.
DDKBASE=c:\winddk\3790
COMPDIR64=$(DDKBASE)\bin\win64\x86\amd64
LINKLIBPATH=/LIBPATH:"$(DDKBASE)\lib\wnet\amd64"
INCDIR64A=$(DDKBASE)\inc\wnet
INCDIR64B=$(DDKBASE)\inc\crt
!endif
!endif
!endif

!if $(MSVC_VERSION) == 8
! ifndef DEVSTUDIO
!ifdef WIN64
DEVSTUDIO=C:\Program Files (x86)\Microsoft Visual Studio 8
!else
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio 8
!endif
! endif
!if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
!else
COMPBASE=$(DEVSTUDIO)\VC
SHAREDBASE=$(DEVSTUDIO)\VC
!ifdef WIN64
COMPDIR64=$(COMPBASE)\bin\x86_amd64
LINKLIBPATH=/LIBPATH:"$(COMPBASE)\lib\amd64" /LIBPATH:"$(COMPBASE)\PlatformSDK\Lib\AMD64"
!endif
!endif
!endif

# Some environments don't want to specify the path names for the tools at all.
# Typical definitions for such an environment would be:
#   MSINCDIR= LIBDIR= COMP=cl COMPAUX=cl RCOMP=rc LINK=link
# COMPDIR, LINKDIR, and RCDIR are irrelevant, since they are only used to
# define COMP, LINK, and RCOMP respectively, but we allow them to be
# overridden anyway for completeness.
!ifndef COMPDIR
!if "$(COMPBASE)"==""
COMPDIR=
!else
!ifdef WIN64
COMPDIR=$(COMPDIR64)
!else
COMPDIR=$(COMPBASE)\bin
!endif
!endif
!endif

!ifndef LINKDIR
!if "$(COMPBASE)"==""
LINKDIR=
!else
!ifdef WIN64
LINKDIR=$(COMPDIR64)
!else
LINKDIR=$(COMPBASE)\bin
!endif
!endif
!endif

!ifndef RCDIR
!if "$(SHAREDBASE)"==""
RCDIR=
!else
RCDIR=$(SHAREDBASE)\bin
!endif
!endif

!ifndef MSINCDIR
!if "$(COMPBASE)"==""
MSINCDIR=
!else
MSINCDIR=$(COMPBASE)\include
!endif
!endif

!ifndef LIBDIR
!if "$(COMPBASE)"==""
LIBDIR=
!else
LIBDIR=$(COMPBASE)\lib
!endif
!endif

!ifndef COMP
!if "$(COMPDIR)"==""
COMP=cl
!else
COMP="$(COMPDIR)\cl"
!endif
!endif
!ifndef COMPCPP
COMPCPP=$(COMP)
!endif
!ifndef COMPAUX
!ifdef WIN64
COMPAUX="$(COMPBASE)\bin\cl"
!else
COMPAUX=$(COMP)
!endif
!endif

!ifndef RCOMP
!if "$(RCDIR)"==""
RCOMP=rc
!else
RCOMP="$(RCDIR)\rc"
!endif
!endif

!ifndef LINK
!if "$(LINKDIR)"==""
LINK=link
!else
LINK="$(LINKDIR)\link"
!endif
!endif

# nmake does not have a form of .BEFORE or .FIRST which can be used
# to specify actions before anything else is done.  If LIB and INCLUDE
# are not defined then we want to define them before we link or
# compile.  Here is a kludge which allows us to to do what we want.
# nmake does evaluate preprocessor directives when they are encountered.
# So the desired set statements are put into dummy preprocessor
# directives.
!ifndef INCLUDE
!if "$(MSINCDIR)"!=""
!if [set INCLUDE=$(MSINCDIR)]==0
!endif
!endif
!endif
!ifndef LIB
!if "$(LIBDIR)"!=""
!if [set LIB=$(LIBDIR)]==0
!endif
!endif
!endif

!ifndef LINKLIBPATH
LINKLIBPATH=
!endif

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
FPU_TYPE=387
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
FEATURE_DEVS=$(PSD)psl3.dev $(PSD)pdf.dev $(PSD)dpsnext.dev $(PSD)ttfont.dev $(PSD)epsf.dev $(PSD)mshandle.dev $(PSD)msprinter.dev $(PSD)mspoll.dev $(GLD)pipe.dev $(PSD)fapi.dev $(PSD)jbig2.dev $(PSD)jpx.dev
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
# The choices are 'lzw' or 'zlib'.

!ifndef BAND_LIST_COMPRESSOR
BAND_LIST_COMPRESSOR=zlib
!endif

# Choose the implementation of file I/O: 'stdio', 'fd', or 'both'.
# See gs.mak and sfxfd.c for more details.

!ifndef FILE_IMPLEMENTATION
FILE_IMPLEMENTATION=stdio
!endif

# Choose the implementation of stdio: '' for file I/O and 'c' for callouts
# See gs.mak and ziodevs.c/ziodevsc.c for more details.

!ifndef STDIO_IMPLEMENTATION
STDIO_IMPLEMENTATION=c
!endif

# Choose the device(s) to include.  See devs.mak for details,
# devs.mak and contrib.mak for the list of available devices.

!ifndef DEVICE_DEVS
DEVICE_DEVS=$(DD)display.dev $(DD)mswindll.dev $(DD)mswinpr2.dev
DEVICE_DEVS2=$(DD)epson.dev $(DD)eps9high.dev $(DD)eps9mid.dev $(DD)epsonc.dev $(DD)ibmpro.dev
DEVICE_DEVS3=$(DD)deskjet.dev $(DD)djet500.dev $(DD)laserjet.dev $(DD)ljetplus.dev $(DD)ljet2p.dev
DEVICE_DEVS4=$(DD)cdeskjet.dev $(DD)cdjcolor.dev $(DD)cdjmono.dev $(DD)cdj550.dev
DEVICE_DEVS5=$(DD)uniprint.dev $(DD)djet500c.dev $(DD)declj250.dev $(DD)lj250.dev $(DD)ijs.dev
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
!endif

# FAPI compilation options :
UFST_CFLAGS=-DMSVC

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

MAKEFILE=$(PSSRCDIR)\msvc32.mak
TOP_MAKEFILES=$(MAKEFILE) $(GLSRCDIR)\msvccmd.mak $(GLSRCDIR)\msvctail.mak $(GLSRCDIR)\winlib.mak $(PSSRCDIR)\winint.mak

# Define the files to be removed by `make clean'.
# nmake expands macros when encountered, not when used,
# so this must precede the !include statements.

BEGINFILES2=$(GLGENDIR)\lib32.rsp\
 $(GLOBJDIR)\*.exp $(GLOBJDIR)\*.ilk $(GLOBJDIR)\*.pdb $(GLOBJDIR)\*.lib\
 $(BINDIR)\*.exp $(BINDIR)\*.ilk $(BINDIR)\*.pdb $(BINDIR)\*.lib obj.pdb\
 obj.idb $(GLOBJDIR)\gs.pch

!include $(GLSRCDIR)\msvccmd.mak
!include $(GLSRCDIR)\winlib.mak
!include $(GLSRCDIR)\msvctail.mak
!include $(PSSRCDIR)\winint.mak

# ----------------------------- Main program ------------------------------ #

GSCONSOLE_XE=$(BINDIR)\$(GSCONSOLE).exe
GSDLL_DLL=$(BINDIR)\$(GSDLL).dll
GSDLL_OBJS=$(PSOBJ)gsdll.$(OBJ) $(GLOBJ)gp_msdll.$(OBJ)

!if $(DEBUGSYM) != 0
$(PSGEN)lib32.rsp: $(TOP_MAKEFILES)
	echo /NODEFAULTLIB:LIBC.lib > $(PSGEN)lib32.rsp
	echo /NODEFAULTLIB:LIBCMT.lib >> $(PSGEN)lib32.rsp
	echo LIBCMTD.lib >> $(PSGEN)lib32.rsp
!else
$(PSGEN)lib32.rsp: $(TOP_MAKEFILES)
	echo /NODEFAULTLIB:LIBC.lib > $(PSGEN)lib32.rsp
	echo /NODEFAULTLIB:LIBCMTD.lib >> $(PSGEN)lib32.rsp
	echo LIBCMT.lib >> $(PSGEN)lib32.rsp
!endif


!if $(MAKEDLL)
# The graphical small EXE loader
$(GS_XE): $(GSDLL_DLL)  $(DWOBJ) $(GSCONSOLE_XE) $(SETUP_XE) $(UNINSTALL_XE)
	echo /SUBSYSTEM:WINDOWS > $(PSGEN)gswin32.rsp
	echo /DEF:$(PSSRCDIR)\dwmain32.def /OUT:$(GS_XE) >> $(PSGEN)gswin32.rsp
	$(LINK) $(LCT) @$(PSGEN)gswin32.rsp $(DWOBJ) $(LINKLIBPATH) @$(LIBCTR) $(GS_OBJ).res
	del $(PSGEN)gswin32.rsp

# The console mode small EXE loader
$(GSCONSOLE_XE): $(OBJC) $(GS_OBJ).res $(PSSRCDIR)\dw32c.def
	echo /SUBSYSTEM:CONSOLE > $(PSGEN)gswin32.rsp
	echo  /DEF:$(PSSRCDIR)\dw32c.def /OUT:$(GSCONSOLE_XE) >> $(PSGEN)gswin32.rsp
	$(LINK) $(LCT) @$(PSGEN)gswin32.rsp $(OBJC) $(LINKLIBPATH) @$(LIBCTR) $(GS_OBJ).res
	del $(PSGEN)gswin32.rsp

# The big DLL
$(GSDLL_DLL): $(GS_ALL) $(DEVS_ALL) $(GSDLL_OBJS) $(GSDLL_OBJ).res $(PSGEN)lib32.rsp
	echo /DLL /DEF:$(PSSRCDIR)\gsdll32.def /OUT:$(GSDLL_DLL) > $(PSGEN)gswin32.rsp
	$(LINK) $(LCT) @$(PSGEN)gswin32.rsp $(GSDLL_OBJS) @$(ld_tr) $(INTASM) @$(PSGEN)lib32.rsp $(LINKLIBPATH) @$(LIBCTR) $(GSDLL_OBJ).res
	del $(PSGEN)gswin32.rsp

!else
# The big graphical EXE
$(GS_XE): $(GSCONSOLE_XE) $(GS_ALL) $(DEVS_ALL) $(GSDLL_OBJS) $(DWOBJNO) $(GSDLL_OBJ).res $(PSSRCDIR)\dwmain32.def $(PSGEN)lib32.rsp
	copy $(ld_tr) $(PSGEN)gswin32.tr
	echo $(PSOBJ)dwnodll.obj >> $(PSGEN)gswin32.tr
	echo $(GLOBJ)dwimg.obj >> $(PSGEN)gswin32.tr
	echo $(PSOBJ)dwmain.obj >> $(PSGEN)gswin32.tr
	echo $(GLOBJ)dwtext.obj >> $(PSGEN)gswin32.tr
	echo $(GLOBJ)dwreg.obj >> $(PSGEN)gswin32.tr
	echo /DEF:$(PSSRCDIR)\dwmain32.def /OUT:$(GS_XE) > $(PSGEN)gswin32.rsp
	$(LINK) $(LCT) @$(PSGEN)gswin32.rsp $(GLOBJ)gsdll @$(PSGEN)gswin32.tr $(LINKLIBPATH) @$(LIBCTR) $(INTASM) @$(PSGEN)lib32.rsp $(GSDLL_OBJ).res $(DWTRACE)
	del $(PSGEN)gswin32.tr
	del $(PSGEN)gswin32.rsp

# The big console mode EXE
$(GSCONSOLE_XE): $(GS_ALL) $(DEVS_ALL) $(GSDLL_OBJS) $(OBJCNO) $(GS_OBJ).res $(PSSRCDIR)\dw32c.def $(PSGEN)lib32.rsp
	copy $(ld_tr) $(PSGEN)gswin32c.tr
	echo $(PSOBJ)dwnodllc.obj >> $(PSGEN)gswin32c.tr
	echo $(GLOBJ)dwimg.obj >> $(PSGEN)gswin32c.tr
	echo $(PSOBJ)dwmainc.obj >> $(PSGEN)gswin32c.tr
	echo $(PSOBJ)dwreg.obj >> $(PSGEN)gswin32c.tr
	echo /SUBSYSTEM:CONSOLE > $(PSGEN)gswin32.rsp
	echo /DEF:$(PSSRCDIR)\dw32c.def /OUT:$(GSCONSOLE_XE) >> $(PSGEN)gswin32.rsp
	$(LINK) $(LCT) @$(PSGEN)gswin32.rsp $(GLOBJ)gsdll @$(PSGEN)gswin32c.tr $(LINKLIBPATH) @$(LIBCTR) $(INTASM) @$(PSGEN)lib32.rsp $(GS_OBJ).res $(DWTRACE)
	del $(PSGEN)gswin32.rsp
	del $(PSGEN)gswin32c.tr
!endif

# ---------------------- Setup and uninstall programs ---------------------- #

!if $(MAKEDLL)

$(SETUP_XE): $(PSOBJ)dwsetup.obj $(PSOBJ)dwinst.obj $(PSOBJ)dwsetup.res $(PSSRC)dwsetup.def
	echo /DEF:$(PSSRC)dwsetup.def /OUT:$(SETUP_XE) > $(PSGEN)dwsetup.rsp
	echo $(PSOBJ)dwsetup.obj $(PSOBJ)dwinst.obj >> $(PSGEN)dwsetup.rsp
	copy $(LIBCTR) $(PSGEN)dwsetup.tr
	echo ole32.lib >> $(PSGEN)dwsetup.tr
	echo uuid.lib >> $(PSGEN)dwsetup.tr
	$(LINK) $(LCT) @$(PSGEN)dwsetup.rsp $(LINKLIBPATH) @$(PSGEN)dwsetup.tr $(PSOBJ)dwsetup.res
	del $(PSGEN)dwsetup.rsp
	del $(PSGEN)dwsetup.tr

$(UNINSTALL_XE): $(PSOBJ)dwuninst.obj $(PSOBJ)dwuninst.res $(PSSRC)dwuninst.def
	echo /DEF:$(PSSRC)dwuninst.def /OUT:$(UNINSTALL_XE) > $(PSGEN)dwuninst.rsp
	echo $(PSOBJ)dwuninst.obj >> $(PSGEN)dwuninst.rsp
	copy $(LIBCTR) $(PSGEN)dwuninst.tr
	echo ole32.lib >> $(PSGEN)dwuninst.tr
	echo uuid.lib >> $(PSGEN)dwuninst.tr
	$(LINK) $(LCT) @$(PSGEN)dwuninst.rsp $(LINKLIBPATH) @$(PSGEN)dwuninst.tr $(PSOBJ)dwuninst.res
	del $(PSGEN)dwuninst.rsp
	del $(PSGEN)dwuninst.tr

!endif

DEBUGDEFS=BINDIR=.\debugbin GLGENDIR=.\debugobj GLOBJDIR=.\debugobj PSLIBDIR=.\lib PSGENDIR=.\debugobj PSOBJDIR=.\debugobj DEBUG=1 TDEBUG=1
debug:
	nmake -f $(MAKEFILE) $(DEBUGDEFS)

debugclean:
	nmake -f $(MAKEFILE) $(DEBUGDEFS) clean

# end of makefile
