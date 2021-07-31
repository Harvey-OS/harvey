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

# wccommon.mak
# Section of Watcom C/C++ makefile common to MS-DOS and MS Windows.
# We strongly recommend that you read the Watcom section of make.doc
# before attempting to build Ghostscript with the Watcom compiler.

# This file is used by watc.mak and watcwin.mak.
# Those files supply the following parameters:
#   Configuration, public:
#	GS_LIB_DEFAULT, GS_INIT, FEATURE_DEVS, DEVICE_DEVS*
#   Configuration, internal, generic:
#	PLATFORM, MAKEFILE, AK, CC*, DEBUG, NOPRIVATE
#   Configuration, internal, specific to DOS/Windows:
#	TDEBUG, USE_ASM, ASM,
#	COMPDIR, INCDIR, LIBDIR,
#	CPU_TYPE, FPU_TYPE

# We want Unix-compatible behavior.  This is part of it.

.NOCHECK

# Define additional extensions to keep `make' happy

.EXTENSIONS: .be .z

# Define the ANSI-to-K&R dependency.  Watcom C accepts ANSI syntax.

AK=

# Define the extensions for command, object, and executable files.

CMD=.bat
O=-fo=
OBJ=obj
XE=.exe

# Define the current directory prefix and shell invocations.

D=\\

EXPP=dos4gw
SH=
# The following is needed to work around a problem in wmake.
SHP=command /c

# Define the arguments for genconf.

CONFILES=-p FILE&s%s -ol objw.tr

# Define the generic compilation flags.

!ifeq CPU_TYPE 586
FPFLAGS=-fpi87
!else
!ifeq CPU_TYPE 486
FPFLAGS=-fpi87
!else
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
!endif
!endif

INTASM=
PCFBASM=

# Define the generic compilation rules.

.asm.obj:
	$(ASM) $(ASMFLAGS) $<;

# Make sure we get the right default target for make.

dosdefault: $(GS)$(XE)
	%null

# -------------------------- Auxiliary programs --------------------------- #

echogs$(XE): echogs.c
	echo OPTION STUB=$(STUB) >_temp_.tr
	$(CCL) $(CCFLAGS) -i=$(LIBDIR) @_temp_.tr echogs.c

genarch$(XE): genarch.c
	$(CCL) $(CCFLAGS) -i=$(LIBDIR) genarch.c

genconf$(XE): genconf.c
	echo OPTION STUB=$(STUB) >_temp_.tr
	echo OPTION STACK=8k >>_temp_.tr
	$(CCL) $(CCFLAGS) -i=$(LIBDIR) @_temp_.tr genconf.c

# No special gconfig_.h is needed.
# Watcom `make' supports output redirection.
gconfig_.h:
	echo /* This file deliberately left blank. */ >gconfig_.h

# Define the compilation flags.

!ifneq NOPRIVATE 0
CP=-dNOPRIVATE
!else
CP=
!endif

!ifneq DEBUG 0
CD=-dDEBUG
!else
CD=
!endif

!ifneq TDEBUG 0
CT=-d2
LCT=DEBUG ALL
!else
CT=-d1
LCT=DEBUG LINES
!endif

!ifneq DEBUG 0
CS=
!else
CS=-s
!endif

GENOPT=$(CP) $(CD) $(CT) $(CS)

CCFLAGS=$(GENOPT) $(PLATOPT) $(FPFLAGS) $(CFLAGS) $(XCFLAGS)
CC=$(COMP) -oi -i=$(INCDIR) $(CCFLAGS) -zq
CCL=$(CLINK) -p -oi -i=$(INCDIR) -l=dos4g
CCC=$(CC)
CCD=$(CC)
CCCF=$(CC)
CCINT=$(COMP) -oit -i=$(INCDIR) $(CCFLAGS)

.c.obj:
	$(CCC) $<
