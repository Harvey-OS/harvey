#    Copyright (C) 1991-2001 Aladdin Enterprises.  All rights reserved.
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

# $Id: msvclib.mak,v 1.29 2004/12/20 22:17:39 igor Exp $
# makefile for Microsoft Visual C++ 4.1 or later, Windows NT or Windows 95 LIBRARY.
#
# All configurable options are surrounded by !ifndef/!endif to allow 
# preconfiguration from within another makefile.

# ------------------------------- Options ------------------------------- #

###### This section is the only part of the file you should need to edit.

# ------ Generic options ------ #

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

GS_INIT=gs_init.ps

# Choose generic configuration options.

# Setting DEBUG=1 includes debugging features (-Z switch) in the code.
# Code runs substantially slower even if no debugging switches are set,
# and also takes about another 25K of memory.

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
GS=gslib
!endif

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

# Do not edit the next group of lines.
NUL=
DD=$(GLGENDIR)\$(NUL)
GLD=$(GLGENDIR)\$(NUL)

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

# Define the directory where the icclib source are stored.
# See icclib.mak for more information

!ifndef ICCSRCDIR
ICCSRCDIR=icclib
!endif

# Define any other compilation flags.

!ifndef CFLAGS
CFLAGS=
!endif

# ------ Platform-specific options ------ #

# Define which major version of MSVC is being used (currently, 4, 5 & 6 supported)

!ifndef MSVC_VERSION
MSVC_VERSION = 6
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
COMPDIR=$(COMPBASE)\bin
!endif
!endif

!ifndef LINKDIR
!if "$(COMPBASE)"==""
LINKDIR=
!else
LINKDIR=$(COMPBASE)\bin
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
COMPAUX=$(COMP)
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
FEATURE_DEVS=$(GLD)psl3lib.dev $(GLD)path1lib.dev $(GLD)dps2lib.dev $(GLD)psl2cs.dev $(GLD)cielib.dev $(GLD)imasklib.dev $(GLD)patlib.dev $(GLD)htxlib.dev $(GLD)roplib.dev $(GLD)devcmap.dev $(GLD)bbox.dev $(GLD)pipe.dev
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
# See gs.mak and sfilefd.c for more details.

!ifndef FILE_IMPLEMENTATION
FILE_IMPLEMENTATION=stdio
!endif

# Choose the implementation of stdio: Only '' is allowed for library.
# See gs.mak and ziodevs.c/ziodevsc.c for more details.

STDIO_IMPLEMENTATION= 

# Choose the device(s) to include.  See devs.mak for details,
# devs.mak and contrib.mak for the list of available devices.
!ifndef DEVICE_DEVS
DEVICE_DEVS=$(DD)ljet2p.dev $(DD)bbox.dev
DEVICE_DEVS1=
DEVICE_DEVS2=
DEVICE_DEVS3=
DEVICE_DEVS4=
DEVICE_DEVS5=
DEVICE_DEVS6=
DEVICE_DEVS7=
DEVICE_DEVS8=
DEVICE_DEVS9=
DEVICE_DEVS10=
DEVICE_DEVS11=
DEVICE_DEVS12=
DEVICE_DEVS13=
DEVICE_DEVS14=
DEVICE_DEVS15=
DEVICE_DEVS16=
DEVICE_DEVS17=
DEVICE_DEVS18=
DEVICE_DEVS19=
DEVICE_DEVS20=
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

MAKEFILE=$(GLSRCDIR)\msvclib.mak
TOP_MAKEFILES=$(MAKEFILE) $(GLSRCDIR)\msvccmd.mak $(GLSRCDIR)\msvctail.mak $(GLSRCDIR)\winlib.mak

# Define the files to be removed by `make clean'.
# nmake expands macros when encountered, not when used,
# so this must precede the !include statements.

BEGINFILES2=$(GLOBJDIR)\$(GS).ilk $(GLOBJDIR)\$(GS).pdb $(GLOBJDIR)\genarch.ilk $(GLOBJDIR)\genarch.pdb 

# Define these right away because they modify the behavior of
# msvccmd.mak, msvctail.mak & winlib.mak.

LIB_ONLY=$(GLOBJDIR)\gslib.obj $(GLOBJDIR)\gsnogc.obj $(GLOBJDIR)\gconfig.obj $(GLOBJDIR)\gscdefs.obj
MAKEDLL=0
PLATFORM=mslib32_

!include $(GLSRCDIR)\version.mak
!include $(GLSRCDIR)\msvccmd.mak
!include $(GLSRCDIR)\winlib.mak
!include $(GLSRCDIR)\msvctail.mak

# -------------------------------- Library -------------------------------- #

# The Windows Win32 platform for library

# For some reason, C-file dependencies have to come before mslib32__.dev

$(GLOBJ)gp_mslib.$(OBJ): $(GLSRC)gp_mslib.c $(AK)
	$(GLCCWIN) $(GLO_)gp_mslib.$(OBJ) $(C_) $(GLSRC)gp_mslib.c

mslib32__=$(GLOBJ)gp_mslib.$(OBJ)

$(GLGEN)mslib32_.dev: $(mslib32__) $(ECHOGS_XE) $(GLGEN)mswin32_.dev
	$(SETMOD) $(GLGEN)mslib32_ $(mslib32__)
	$(ADDMOD) $(GLGEN)mslib32_ -include $(GLGEN)mswin32_.dev

# ----------------------------- Main program ------------------------------ #

# The library tester EXE
$(GS_XE):  $(GS_ALL) $(DEVS_ALL) $(LIB_ONLY) $(LIBCTR)
	copy $(ld_tr) $(GLGENDIR)\gslib32.tr
	echo $(GLOBJ)gsnogc.obj >> $(GLGENDIR)\gslib32.tr
	echo $(GLOBJ)gconfig.obj >> $(GLGENDIR)\gslib32.tr
	echo $(GLOBJ)gscdefs.obj >> $(GLGENDIR)\gslib32.tr
	echo  /SUBSYSTEM:CONSOLE > $(GLGENDIR)\gslib32.rsp
	echo  /OUT:$(GS_XE) >> $(GLGENDIR)\gslib32.rsp
	$(LINK) $(LCT) @$(GLGENDIR)\gslib32.rsp $(GLOBJ)gslib @$(GLGENDIR)\gslib32.tr @$(LIBCTR) $(INTASM)
	-del $(GLGENDIR)\gslib32.rsp
	-del $(GLGENDIR)\gslib32.tr
