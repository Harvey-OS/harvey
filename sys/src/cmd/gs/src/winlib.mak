#    Copyright (C) 1991-1999 Aladdin Enterprises.  All rights reserved.
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

# $Id: winlib.mak,v 1.1 2000/03/09 08:40:44 lpd Exp $
# Common makefile section for 32-bit MS Windows.

# This makefile must be acceptable to Microsoft Visual C++, Watcom C++,
# and Borland C++.  For this reason, the only conditional directives
# allowed are !if[n]def, !else, and !endif.


# Note that built-in third-party libraries aren't available.

SHARE_JPEG=0
SHARE_LIBPNG=0
SHARE_ZLIB=0

# Define the platform name.

!ifndef PLATFORM
PLATFORM=mswin32_
!endif

# Define the ANSI-to-K&R dependency.  Borland C, Microsoft C and
# Watcom C all accept ANSI syntax, but we need to preconstruct ccf32.tr 
# to get around the limit on the maximum length of a command line.

AK=$(GLGENDIR)\ccf32.tr

# Define the syntax for command, object, and executable files.

NULL=

CMD=.bat
D_=-D
_D_=$(NULL)=
_D=
I_=-I
II=-I
_I=
NO_OP=@rem
# O_ and XE_ are defined separately for each compiler.
OBJ=obj
Q=
XE=.exe
XEAUX=.exe

# Define generic commands.

# We have to use a batch file for the equivalent of cp,
# because the DOS COPY command copies the file write time, like cp -p.
CP_=call $(GLSRCDIR)\cp.bat
RM_=erase
RMN_=call $(GLSRCDIR)\rm.bat

# Define the generic compilation flags.

PLATOPT=

INTASM=
PCFBASM=

# Define the files to be removed by `make clean'.
# nmake expands macros when encountered, not when used,
# so this must precede the !include statements.

# ****** WRONG ****** NEED GLOBJ PREFIX ******
BEGINFILES=gs*.res gs*.ico $(GLGENDIR)\ccf32.tr\
   $(GSDLL).dll $(GSCONSOLE).exe\
   $(BEGINFILES2)

# Include the generic makefiles.
#!include $(COMMONDIR)/pcdefs.mak
#!include $(COMMONDIR)/generic.mak
!include $(GLSRCDIR)\gs.mak
!include $(GLSRCDIR)\lib.mak
!include $(GLSRCDIR)\jpeg.mak
# zlib.mak must precede libpng.mak
!include $(GLSRCDIR)\zlib.mak
!include $(GLSRCDIR)\libpng.mak
!include $(GLSRCDIR)\devs.mak
!include $(GLSRCDIR)\contrib.mak

# Define the compilation rule for Windows devices.
# This requires GL*_ to be defined, so it has to come after lib.mak.
GLCCWIN=$(CC_WX) $(CCWINFLAGS) $(I_)$(GLI_)$(_I) $(GLF_)

!include $(GLSRCDIR)\winplat.mak
!include $(GLSRCDIR)\pcwin.mak

# Define abbreviations for the executable and DLL files.
GS_OBJ=$(GLOBJ)$(GS)
GSDLL_SRC=$(GLSRC)$(GSDLL)
GSDLL_OBJ=$(GLOBJ)$(GSDLL)

# -------------------------- Auxiliary files --------------------------- #

# No special gconfig_.h is needed.
# Assume `make' supports output redirection.
$(gconfig__h): $(TOP_MAKEFILES)
	echo /* This file deliberately left blank. */ >$(gconfig__h)

$(gconfigv_h): $(TOP_MAKEFILES) $(ECHOGS_XE)
	$(ECHOGS_XE) -w $(gconfigv_h) -x 23 define USE_ASM -x 2028 -q $(USE_ASM)-0 -x 29
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define USE_FPU -x 2028 -q $(FPU_TYPE)-0 -x 29
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define EXTEND_NAMES 0$(EXTEND_NAMES)
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define SYSTEM_CONSTANTS_ARE_WRITABLE 0$(SYSTEM_CONSTANTS_ARE_WRITABLE)

# -------------------------------- Library -------------------------------- #

# The Windows Win32 platform

mswin32__=$(GLOBJ)gp_msio.$(OBJ)
$(GLGEN)mswin32_.dev: $(mswin32__) $(ECHOGS_XE) $(GLGEN)msw32nc_.dev
	$(SETMOD) $(GLGEN)mswin32_ $(mswin32__)
	$(ADDMOD) $(GLGEN)mswin32_ -include $(GLGEN)msw32nc_.dev
	$(ADDMOD) $(GLGEN)mswin32_ -iodev wstdio

$(GLOBJ)gp_msio.$(OBJ): $(GLSRC)gp_msio.c $(AK) $(gp_mswin_h) \
 $(gsdll_h) $(stdio__h) $(gxiodev_h) $(stream_h) $(gx_h) $(gp_h) $(windows__h)
	$(GLCCWIN) $(GLO_)gp_msio.$(OBJ) $(C_) $(GLSRC)gp_msio.c

# Hack: we need a version of the platform code that doesn't include the
# console I/O module gp_msio.c, because this incorrectly refers to gsdll.c,
# which in turn incorrectly refers to PostScript interpreter code.

msw32nc__=$(GLOBJ)gp_mswin.$(OBJ) $(GLOBJ)gp_nofb.$(OBJ) $(GLOBJ)gp_wgetv.$(OBJ)
msw32nc_inc=$(GLD)nosync.dev $(GLD)winplat.dev
$(GLGEN)msw32nc_.dev: $(msw32nc__) $(ECHOGS_XE) $(msw32nc_inc)
	$(SETMOD) $(GLGEN)msw32nc_ $(msw32nc__)
	$(ADDMOD) $(GLGEN)msw32nc_ -include $(msw32nc_inc)

$(GLOBJ)gp_mswin.$(OBJ): $(GLSRC)gp_mswin.c $(AK) $(gp_mswin_h) \
 $(ctype__h) $(dos__h) $(malloc__h) $(memory__h) $(string__h) $(windows__h) \
 $(gx_h) $(gp_h) $(gpcheck_h) $(gserrors_h) $(gsexit_h)
	$(GLCCWIN) $(GLO_)gp_mswin.$(OBJ) $(C_) $(GLSRC)gp_mswin.c

$(GLOBJ)gp_wgetv.$(OBJ): $(GLSRC)gp_wgetv.c $(AK) $(gscdefs_h)
	$(GLCCWIN) $(GLO_)gp_wgetv.$(OBJ) $(C_) $(GLSRC)gp_wgetv.c

# Define MS-Windows handles (file system) as a separable feature.

mshandle_=$(GLOBJ)gp_mshdl.$(OBJ)
$(GLD)mshandle.dev: $(ECHOGS_XE) $(mshandle_)
	$(SETMOD) $(GLD)mshandle $(mshandle_)
	$(ADDMOD) $(GLD)mshandle -iodev handle

$(GLOBJ)gp_mshdl.$(OBJ): $(GLSRC)gp_mshdl.c $(AK)\
 $(ctype__h) $(errno__h) $(stdio__h) $(string__h)\
 $(gserror_h) $(gsmemory_h) $(gstypes_h) $(gxiodev_h)
	$(GLCC) $(GLO_)gp_mshdl.$(OBJ) $(C_) $(GLSRC)gp_mshdl.c

# end of winlib.mak
