#    Copyright (C) 1990, 1993, 1996, 1997, 1999 Aladdin Enterprises.  All rights reserved.
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

# $Id: unixhead.mak,v 1.4 2002/02/21 22:24:54 giles Exp $
# Partial makefile common to all Unix configurations.

# This part of the makefile gets inserted after the compiler-specific part
# (xxx-head.mak) and before gs.mak, devs.mak, and contrib.mak.

# ----------------------------- Generic stuff ----------------------------- #

# Define the platform name.  For a "stock" System V platform,
# use sysv_ instead of unix_.

PLATFORM=unix_

# Define the syntax for command, object, and executable files.

# Work around the fact that some `make' programs drop trailing spaces
# or interpret == as a special definition operator.
NULL=

CMD=
C_=-c
D_=-D
_D_=$(NULL)=
_D=
I_=-I
II=-I
_I=
NO_OP=@:
O_=-o $(NULL)
OBJ=o
Q=
XE=
XEAUX=

# Define the current directory prefix and command invocations.

CAT=cat
D=/
EXP=
SHELL=/bin/sh
SH=$(SHELL)

# Define generic commands.

CP_=cp
RM_=rm -f
RMN_=rm -f

# Define the arguments for genconf.

CONFILES=-p "%s&s&&" -pl "&-l%s&s&&" -pL "&-L%s&s&&"
CONFLDTR=-ol

# Define the compilation rules and flags.

CC_D=$(CC_)
CC_INT=$(CC_)

BEGINFILES=

# Patch a couple of PC-specific things that aren't relevant to Unix builds,
# but that cause `make' to produce warnings.

PCFBASM=

# Define the default build rule, so the object directories get created
# automatically.

std: STDDIRS default
	$(NO_OP)
