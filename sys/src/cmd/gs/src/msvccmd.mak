#    Copyright (C) 1997, 1998 Aladdin Enterprises.  All rights reserved.
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

# $Id: msvccmd.mak,v 1.1 2000/03/09 08:40:44 lpd Exp $
# Command definition section for Microsoft Visual C++ 4.x/5.x,
# Windows NT or Windows 95 platform.
# Created 1997-05-22 by L. Peter Deutsch from msvc4/5 makefiles.
# edited 1997-06-xx by JD to factor out interpreter-specific sections

# Set up linker differently for MSVC 4 vs. later versions

!if $(MSVC_VERSION) == 4

# MSVC version 4.x doesn't recognize the /QI0f switch, which works around
# an unspecified bug in the Pentium decoding of certain 0F instructions.
QI0f=

# Set up LIB enviromnent variable to include LIBDIR. This is a hack for
# MSVC4.x, which doesn't have compiler switches to do the deed

!ifdef LIB
LIB=$(LIBDIR);$(LIB)
!else
LINK_SETUP=set LIB=$(LIBDIR)
CCAUX_SETUP=$(LINK_SETUP)
!endif

!else
#else $(MSVC_VERSION) != 4

# MSVC 5.x does recognize /QI0f.
QI0f=/QI0f

# Define linker switch that will select where MS libraries are.

LINK_LIB_SWITCH=/LIBPATH:$(LIBDIR)

# Define separate CCAUX command-line switch that must be at END of line.

CCAUX_TAIL= /link $(LINK_LIB_SWITCH)

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

CONFILES=-p %%s -l $(GLGENDIR)\lib.tr
CONFLDTR=-o

# Define the generic compilation flags.

PLATOPT=

INTASM=
PCFBASM=

# Make sure we get the right default target for make.

dosdefault: default

# Define the compilation flags.

!if "$(CPU_FAMILY)"=="i386"

!if $(CPU_TYPE)>500
CPFLAGS=/G5 $(QI0f)
!else if $(CPU_TYPE)>400
CPFLAGS=/GB $(QI0f)
!else
CPFLAGS=/GB $(QI0f)
!endif

!if $(FPU_TYPE)>0
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

!if $(TDEBUG)!=0
# /Fd designates the directory for the .pdb file.
# Note that it must be followed by a space.
CT=/Zi /Od /Fd$(GLOBJDIR) $(NULL)
LCT=/DEBUG $(LINK_LIB_SWITCH)
COMPILE_FULL_OPTIMIZED=    # no optimization when debugging
COMPILE_WITH_FRAMES=    # no optimization when debugging
COMPILE_WITHOUT_FRAMES=    # no optimization when debugging
!else
CT=
LCT=$(LINK_LIB_SWITCH)
# NOTE: With MSVC++ 5.0, /O2 produces a non-working executable.
# We believe the following list of optimizations works around this bug.
COMPILE_FULL_OPTIMIZED=/GF /Ot /Oi /Ob2 /Oy /Oa- /Ow-
COMPILE_WITH_FRAMES=
COMPILE_WITHOUT_FRAMES=/Oy
!endif

!if $(DEBUG)!=0 || $(TDEBUG)!=0
CS=/Ge
!else
CS=/Gs
!endif

# Specify output object name
CCOBJNAME=-Fo

# Specify function prolog type
COMPILE_FOR_DLL=
COMPILE_FOR_EXE=
COMPILE_FOR_CONSOLE_EXE=


# The /MT is for multi-threading.  We would like to make this an option,
# but it's too much work right now.
GENOPT=$(CP) $(CD) $(CT) $(CS) /W2 /nologo /MT

CCFLAGS=$(PLATOPT) $(FPFLAGS) $(CPFLAGS) $(CFLAGS) $(XCFLAGS)
CC=$(COMP) /c $(CCFLAGS) @$(GLGENDIR)\ccf32.tr
CPP=$(COMPCPP) /c $(CCFLAGS) @$(GLGENDIR)\ccf32.tr
!if $(MAKEDLL)
WX=$(COMPILE_FOR_DLL)
!else
WX=$(COMPILE_FOR_EXE)
!endif
# /Za disables the MS-specific extensions & enables ANSI mode.
CC_WX=$(CC) $(WX)
CC_=$(CC_WX) $(COMPILE_FULL_OPTIMIZED) /Za
CC_D=$(CC_WX) $(COMPILE_WITH_FRAMES)
CC_INT=$(CC_)
CC_LEAF=$(CC_) $(COMPILE_WITHOUT_FRAMES)
CC_NO_WARN=$(CC_)

# Compiler for auxiliary programs

CCAUX=$(COMPAUX) /I$(INCDIR) /O

# Compiler for Windows programs.
# /Ze enables MS-specific extensions (this is also the default).

CCWINFLAGS=$(COMPILE_FULL_OPTIMIZED) /Ze

#end msvccmd.mak
