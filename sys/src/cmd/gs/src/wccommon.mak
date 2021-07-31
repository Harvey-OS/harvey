#    Copyright (C) 1991, 2000 Aladdin Enterprises.  All rights reserved.
# 
# This file is part of AFPL Ghostscript.
# 
# AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
# distributor accepts any responsibility for the consequences of using it, or
# for whether it serves any particular purpose or works at all, unless he or
# she says so in writing.  Refer to the Aladdin Free Public License (the
# "License") for full details.
# 
# Every copy of AFPL Ghostscript must include a copy of the License, normally
# in a plain ASCII text file named PUBLIC.  The License grants you the right
# to copy, modify and redistribute AFPL Ghostscript, but only under certain
# conditions described in the License.  Among other things, the License
# requires that the copyright notice and this notice be preserved on all
# copies.

# $Id: wccommon.mak,v 1.3 2000/09/19 19:00:52 lpd Exp $
# wccommon.mak
# Section of Watcom C/C++ makefile common to MS-DOS and MS Windows.
# We strongly recommend that you read the Watcom section of Make.htm
# before attempting to build Ghostscript with the Watcom compiler.

# This file is used by watc.mak, watcwin.mak, and watclib.mak.
# Those files supply the following parameters:
#   Configuration, public:
#	GS_LIB_DEFAULT, SEARCH_HERE_FIRST, GS_INIT, FEATURE_DEVS,
#	DEVICE_DEVS*, COMPILE_INITS, BAND_LIST_*
#   Configuration, internal, generic:
#	PLATFORM, MAKEFILE, AK, CC*, DEBUG, NOPRIVATE, CP_, RM_, RMN_
#   Configuration, internal, specific to DOS/Windows:
#	TDEBUG, USE_ASM, ASM,
#	COMPDIR, LIBPATHS,
#	CPU_TYPE, FPU_TYPE

# We want Unix-compatible behavior.  This is part of it.

.NOCHECK

# Define additional extensions to keep `make' happy

.EXTENSIONS: .be .z

# Define the ANSI-to-K&R dependency.  Watcom C accepts ANSI syntax.

AK=

# Note that built-in libpng and zlib aren't available.

SHARE_JPEG=0
SHARE_LIBPNG=0
SHARE_ZLIB=0

# Define the extensions for command, object, and executable files.

NULL=

C_=
CMD=.bat
D_=-D
_D_=$(NULL)=
_D=
I_=-i=
II=-i=
_I=
NO_OP=%null
O_=-fo=
OBJ=obj
Q=
RO_=$(O_)
XE=.exe
XEAUX=.exe

# Define the executable and shell invocations.

D=\\

EXP=
SH=

# Define generic commands.

CP_=call $(GLSRCDIR)\cp.bat
RM_=call $(GLSRCDIR)\rm.bat
RMN_=call $(GLSRCDIR)\rm.bat

# Define the arguments for genconf.

# wmake interprets & as calling for background execution, and ^ fails on
# Windows NT.
CONFILES=-e ~ -p FILE~s~ps
CONFLDTR=-ol

# Define the names of the Watcom C files.
# See the comments in watc.mak and watcwin.mak regarding WCVERSION.

!ifeq WCVERSION 11.0
# 11.0 is currently the same as 10.5.
COMP=$(%WATCOM)\binw\wcc386
LINK=$(%WATCOM)\binw\wlink
STUB=$(%WATCOM)\binw\wstub.exe
WRC=$(%WATCOM)\binw\wrc.exe
!endif

!ifeq WCVERSION 10.5
COMP=$(%WATCOM)\binw\wcc386
LINK=$(%WATCOM)\binw\wlink
STUB=$(%WATCOM)\binw\wstub.exe
WRC=$(%WATCOM)\binw\wrc.exe
!endif

!ifeq WCVERSION 10.0
COMP=$(%WATCOM)\binb\wcc386
LINK=$(%WATCOM)\bin\wlink
STUB=$(%WATCOM)\binb\wstub.exe
WRC=$(%WATCOM)\binb\wrc.exe
!endif

!ifeq WCVERSION 9.5
COMP=$(%WATCOM)\bin\wcc386
LINK=$(%WATCOM)\bin\wlinkp
STUB=$(%WATCOM)\binb\wstub.exe
WRC=$(%WATCOM)\binb\wrc.exe
!endif

# 95/NT Watcom compiler versions
# 10.695 is 10.6 under Windows 95 or NT (32 bit hosted tools)
!ifeq WCVERSION 10.695
COMP=$(%WATCOM)\binnt\wcc386
LINK=$(%WATCOM)\binnt\wlink
STUB=$(%WATCOM)\binw\wstub.exe
WRC=$(%WATCOM)\binnt\wrc.exe
WAT32=1
!endif

# Defaults
!ifndef COMP
COMP=$(%WATCOM)\bin\wcc386p
LINK=$(%WATCOM)\bin\wlinkp
STUB=$(%WATCOM)\binb\wstub.exe
WRC=$(%WATCOM)\binb\rc.exe
!endif
!ifndef WAT32
WAT32=0
!endif

!ifeq WAT32 0
INCDIRS=$(%WATCOM)\h
!else
INCDIRS=$(%WATCOM)\h;$(%WATCOM)\h\nt
!endif
WBIND=$(%WATCOM)\binb\wbind.exe

# Define the generic compilation flags.

!ifeq CPU_TYPE 586
!ifeq FPU_TYPE 0
FPU_TYPE=387
!endif
!else
!ifeq CPU_TYPE 486
!ifeq FPU_TYPE 0
FPU_TYPE=387
!endif
!endif
!endif

!ifeq FPU_TYPE 387
FPFLAGS=-fpi87
!else
!ifeq FPU_TYPE 287
FPFLAGS=-fpi287
!else
!ifeq FPU_TYPE -1
FPFLAGS=-fpc
!else
FPFLAGS=-fpi
!endif
!endif
!endif

INTASM=
PCFBASM=

# Define the generic compilation rules.

.asm.obj:
	$(ASM) $(ASMFLAGS) $<;

# Make sure we get the right default target for make.

dosdefault: default
	$(NO_OP)

# Define the compilation flags.

# Privacy
!ifneq NOPRIVATE 0
CP=-dNOPRIVATE
!else
CP=
!endif

# Run-time debugging and stack checking
!ifneq DEBUG 0
CD=-dDEBUG
CS=
!else
CD=
CS=-oeilnt -s
!endif

# Debugger symbols
!ifneq TDEBUG 0
CT=-d2
LCT=DEBUG ALL
!else
CT=-d1
LCT=DEBUG LINES
!endif

GENOPT=$(CP) $(CD) $(CT) $(CS)

CCOPT=-d+ -i=$(INCDIRS) -zq -zp8 -ei
CCFLAGS=$(CCOPT) $(GENOPT) $(PLATOPT) $(FPFLAGS) $(CFLAGS) $(XCFLAGS)
CC=$(COMP) -oi $(CCFLAGS)
CCAUX=$(COMP) -oi $(CCOPT) $(FPFLAGS)
CC_=$(CC)
CC_D=$(CC)
CC_INT=$(COMP) -oit $(CCFLAGS)
CC_LEAF=$(CC_) -s
CC_NO_WARN=$(CC_)
