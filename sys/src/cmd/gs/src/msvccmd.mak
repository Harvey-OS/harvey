#    Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: msvccmd.mak,v 1.27 2005/07/12 17:54:55 igor Exp $
# Command definition section for Microsoft Visual C++ 4.x/5.x,
# Windows NT or Windows 95 platform.
# Created 1997-05-22 by L. Peter Deutsch from msvc4/5 makefiles.
# edited 1997-06-xx by JD to factor out interpreter-specific sections
# edited 2000-03-30 by lpd to make /FPi87 conditional on MSVC version
# edited 2000-06-05 by lpd to treat empty MSINCDIR and LIBDIR specially.

# Set up linker differently for MSVC 4 vs. later versions

!if $(MSVC_VERSION) == 4

# MSVC version 4.x doesn't recognize the /QI0f switch, which works around
# an unspecified bug in the Pentium decoding of certain 0F instructions.
QI0f=

!else
#else $(MSVC_VERSION) != 4

# MSVC 5.x does recognize /QI0f.
QI0f=/QI0f

# Define separate CCAUX command-line switch that must be at END of line.

CCAUX_TAIL= /link

!endif
#endif #$(MSVC_VERSION) == 4


# Define the current directory prefix and shell invocations.

D=\#

SH=

# Define switches for the compilers.

C_=
O_=-Fo
RO_=$(O_)

# Define the arguments for genconf.

CONFILES=-p %%s
CONFLDTR=-ol

# Define the generic compilation flags.

PLATOPT=

INTASM=
PCFBASM=

# Make sure we get the right default target for make.

dosdefault: default
	@attrib -H dosdefault > nul:
	@echo Done. > dosdefault
	@attrib +H dosdefault

# Define the compilation flags.

# MSVC 8 (2005) warns about deprecated unsafe common functions like strcpy.
!if ($(MSVC_VERSION) == 8) || defined(WIN64)
VC8WARN=/wd4996 /wd4224
!else
VC8WARN=
!endif

!if ($(MSVC_VERSION) < 8)
CDCC=/Gi /ZI
!else
CDCC=/ZI
!endif

!if "$(CPU_FAMILY)"=="i386"

!if ($(MSVC_VERSION) >= 8) || defined(WIN64)
# MSVC 8 (2005) attempts to produce code good for all processors.
# and doesn't used /G5 or /GB.
# MSVC 8 (2005) avoids buggy 0F instructions.
CPFLAGS=
!else
!if $(CPU_TYPE)>500
CPFLAGS=/G5 $(QI0f)
!else if $(CPU_TYPE)>400
CPFLAGS=/GB $(QI0f)
!else
CPFLAGS=/GB $(QI0f)
!endif
!endif

!if $(FPU_TYPE)>0 && $(MSVC_VERSION)<5
FPFLAGS=/FPi87
!else
FPFLAGS=
!endif

!endif

!if "$(CPU_FAMILY)"=="ppc"

!if $(CPU_TYPE)>=620
CPFLAGS=/QP620
!else if $(CPU_TYPE)>=604
CPFLAGS=/QP604
!else
CPFLAGS=/QP601
!endif

FPFLAGS=

!endif

!if "$(CPU_FAMILY)"=="alpha"

# *** alpha *** This needs fixing
CPFLAGS=
FPFLAGS=

!endif

!if $(NOPRIVATE)!=0
CP=/DNOPRIVATE
!else
CP=
!endif

!if $(DEBUG)!=0
CD=/DDEBUG
!else
CD=
!endif

# Precompiled headers
!if $(MSVC_VERSION) >= 8
CPCH=/Fp$(GLOBJDIR)\gs.pch
!else
CPCH=/YX /Fp$(GLOBJDIR)\gs.pch
!endif

!if $(TDEBUG)!=0
# /Fd designates the directory for the .pdb file.
# Note that it must be followed by a space.
CT=/Od /Fd$(GLOBJDIR) $(NULL) $(CDCC) $(CPCH)
LCT=/DEBUG /INCREMENTAL:YES
COMPILE_FULL_OPTIMIZED=    # no optimization when debugging
COMPILE_WITH_FRAMES=    # no optimization when debugging
COMPILE_WITHOUT_FRAMES=    # no optimization when debugging
CMT=/MTd
!else
!if $(DEBUGSYM)==0
CT=
LCT=
CMT=/MT
!else
CT=/Zi /Fd$(GLOBJDIR) $(NULL)
LCT=/DEBUG
CMT=/MTd
!endif
!if $(MSVC_VERSION) == 5
# NOTE: With MSVC++ 5.0, /O2 produces a non-working executable.
# We believe the following list of optimizations works around this bug.
COMPILE_FULL_OPTIMIZED=/GF /Ot /Oi /Ob2 /Oy /Oa- /Ow-
!else
COMPILE_FULL_OPTIMIZED=/GF /O2 /Ob2
!endif
COMPILE_WITH_FRAMES=
COMPILE_WITHOUT_FRAMES=/Oy
!endif

!if $(MSVC_VERSION) >= 8
# MSVC 8 (2005) always does stack probes and checking.
CS=
!else
!if $(DEBUG)!=0 || $(TDEBUG)!=0
CS=/Ge
!else
CS=/Gs
!endif
!endif

!if ($(MSVC_VERSION) == 7) && defined(WIN64)
# Need to specify DDK include directories before .NET 2003 directories.
MSINCFLAGS=-I"$(INCDIR64A)" -I"$(INCDIR64B)"
!else
MSINCFLAGS=
!endif

# Specify output object name
CCOBJNAME=-Fo

# Specify function prolog type
COMPILE_FOR_DLL=
COMPILE_FOR_EXE=
COMPILE_FOR_CONSOLE_EXE=

# The /MT is for multi-threading.  We would like to make this an option,
# but it's too much work right now.
GENOPT=$(CP) $(CD) $(CT) $(CS) $(WARNOPT) $(VC8WARN) /nologo $(CMT)

CCFLAGS=$(PLATOPT) $(FPFLAGS) $(CPFLAGS) $(CFLAGS) $(XCFLAGS) $(MSINCFLAGS)
CC=$(COMP) /c $(CCFLAGS) @$(GLGENDIR)\ccf32.tr
CPP=$(COMPCPP) /c $(CCFLAGS) @$(GLGENDIR)\ccf32.tr
!if $(MAKEDLL)
WX=$(COMPILE_FOR_DLL)
!else
WX=$(COMPILE_FOR_EXE)
!endif

!if $(COMPILE_INITS)
ZM=/Zm600
!else
ZM=
!endif


# /Za disables the MS-specific extensions & enables ANSI mode.
CC_WX=$(CC) $(WX)
CC_=$(CC_WX) $(COMPILE_FULL_OPTIMIZED) /Za $(ZM)
CC_D=$(CC_WX) $(COMPILE_WITH_FRAMES)
CC_INT=$(CC_)
CC_NO_WARN=$(CC_)

# Compiler for auxiliary programs

CCAUX=$(COMPAUX) $(VC8WARN)

# Compiler for Windows programs.
CCWINFLAGS=$(COMPILE_FULL_OPTIMIZED)

#end msvccmd.mak
