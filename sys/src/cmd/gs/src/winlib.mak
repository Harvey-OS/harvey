#    Copyright (C) 1991-1999, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: winlib.mak,v 1.24 2004/12/01 20:02:11 ray Exp $
# Common makefile section for 32-bit MS Windows.

# This makefile must be acceptable to Microsoft Visual C++, Watcom C++,
# and Borland C++.  For this reason, the only conditional directives
# allowed are !if[n]def, !else, and !endif.


# Note that built-in third-party libraries aren't available.

SHARE_JPEG=0
SHARE_LIBPNG=0
SHARE_ZLIB=0
SHARE_JBIG2=0
SHARE_JASPER=0

# Define the platform name.

!ifndef PLATFORM
PLATFORM=mswin32_
!endif

# Define the auxiliary program dependency. We use this to 
# preconstruct ccf32.tr to get around the limit on the maximum
# length of a command line.

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
# We also have to use a batch file for for the equivalent of rm -f,
# because the DOS ERASE command returns an error status if the file
# doesn't exist.
CP_=call $(GLSRCDIR)\cp.bat
RM_=call $(GLSRCDIR)\rm.bat
RMN_=call $(GLSRCDIR)\rm.bat

# Define the generic compilation flags.

PLATOPT=

INTASM=
PCFBASM=

# Define conditinal name for UFST bridge :
!ifdef UFST_ROOT
UFST_BRIDGE = 1
UFST_LIB_EXT=.lib
!endif

# Define conditinal name for FreeType bridge :
!ifdef FT_ROOT
FT_BRIDGE = 1
FT_LIB_EXT=.lib
!endif

# Define the files to be removed by `make clean'.
# nmake expands macros when encountered, not when used,
# so this must precede the !include statements.

BEGINFILES=$(GLGENDIR)\ccf32.tr\
 $(GLOBJDIR)\*.res $(GLOBJDIR)\*.ico\
 $(BINDIR)\$(GSDLL).dll $(BINDIR)\$(GSCONSOLE).exe\
 $(BINDIR)\setupgs.exe $(BINDIR)\uninstgs.exe\
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
!include $(GLSRCDIR)\jbig2.mak
!include $(GLSRCDIR)\jasper.mak
!include $(GLSRCDIR)\icclib.mak
!include $(GLSRCDIR)\ijs.mak
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

# No special gconfig_.h is needed.	/* This file deliberately left blank. */
$(gconfig__h): $(TOP_MAKEFILES)
	$(ECHOGS_XE) -w $(gconfig__h) -x 2f2a20 This file deliberately left blank. -x 2a2f

$(gconfigv_h): $(TOP_MAKEFILES) $(ECHOGS_XE)
	$(ECHOGS_XE) -w $(gconfigv_h) -x 23 define USE_ASM -x 2028 -q $(USE_ASM)-0 -x 29
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define USE_FPU -x 2028 -q $(FPU_TYPE)-0 -x 29
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define EXTEND_NAMES 0$(EXTEND_NAMES)
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define SYSTEM_CONSTANTS_ARE_WRITABLE 0$(SYSTEM_CONSTANTS_ARE_WRITABLE)

# -------------------------------- Library -------------------------------- #

# The Windows Win32 platform

mswin32__=$(GLOBJ)gp_mswin.$(OBJ) $(GLOBJ)gp_wgetv.$(OBJ) $(GLOBJ)gp_stdia.$(OBJ)
mswin32_inc=$(GLD)nosync.dev $(GLD)winplat.dev

$(GLGEN)mswin32_.dev:  $(mswin32__) $(ECHOGS_XE) $(mswin32_inc)
	$(SETMOD) $(GLGEN)mswin32_ $(mswin32__)
	$(ADDMOD) $(GLGEN)mswin32_ -include $(mswin32_inc)

$(GLOBJ)gp_mswin.$(OBJ): $(GLSRC)gp_mswin.c $(AK) $(gp_mswin_h) \
 $(ctype__h) $(dos__h) $(malloc__h) $(memory__h) $(pipe__h) \
 $(stdio__h) $(string__h) $(windows__h) \
 $(gx_h) $(gp_h) $(gpcheck_h) $(gpmisc_h) $(gserrors_h) $(gsexit_h)
	$(GLCCWIN) $(GLO_)gp_mswin.$(OBJ) $(C_) $(GLSRC)gp_mswin.c

$(GLOBJ)gp_wgetv.$(OBJ): $(GLSRC)gp_wgetv.c $(AK) $(gscdefs_h)
	$(GLCCWIN) $(GLO_)gp_wgetv.$(OBJ) $(C_) $(GLSRC)gp_wgetv.c

$(GLOBJ)gp_stdia.$(OBJ): $(GLSRC)gp_stdia.c $(AK)\
  $(stdio__h) $(time__h) $(unistd__h) $(gx_h) $(gp_h)
	$(GLCCWIN) $(GLO_)gp_stdia.$(OBJ) $(C_) $(GLSRC)gp_stdia.c

# Define MS-Windows handles (file system) as a separable feature.

mshandle_=$(GLOBJ)gp_mshdl.$(OBJ)
$(GLD)mshandle.dev: $(ECHOGS_XE) $(mshandle_)
	$(SETMOD) $(GLD)mshandle $(mshandle_)
	$(ADDMOD) $(GLD)mshandle -iodev handle

$(GLOBJ)gp_mshdl.$(OBJ): $(GLSRC)gp_mshdl.c $(AK)\
 $(ctype__h) $(errno__h) $(stdio__h) $(string__h)\
 $(gserror_h) $(gsmemory_h) $(gstypes_h) $(gxiodev_h)
	$(GLCC) $(GLO_)gp_mshdl.$(OBJ) $(C_) $(GLSRC)gp_mshdl.c

# Define MS-Windows printer (file system) as a separable feature.

msprinter_=$(GLOBJ)gp_msprn.$(OBJ)
$(GLD)msprinter.dev: $(ECHOGS_XE) $(msprinter_)
	$(SETMOD) $(GLD)msprinter $(msprinter_)
	$(ADDMOD) $(GLD)msprinter -iodev printer

$(GLOBJ)gp_msprn.$(OBJ): $(GLSRC)gp_msprn.c $(AK)\
 $(ctype__h) $(errno__h) $(stdio__h) $(string__h)\
 $(gserror_h) $(gsmemory_h) $(gstypes_h) $(gxiodev_h)
	$(GLCCWIN) $(GLO_)gp_msprn.$(OBJ) $(C_) $(GLSRC)gp_msprn.c

# Define MS-Windows polling as a separable feature
# because it is not needed by the gslib.
mspoll_=$(GLOBJ)gp_mspol.$(OBJ)
$(GLD)mspoll.dev: $(ECHOGS_XE) $(mspoll_)
	$(SETMOD) $(GLD)mspoll $(mspoll_)

$(GLOBJ)gp_mspol.$(OBJ): $(GLSRC)gp_mspol.c $(AK)\
 $(gx_h) $(gp_h) $(gpcheck_h) $(iapi_h) $(iref_h) $(iminst_h) $(imain_h)
	$(GLCCWIN) $(GLO_)gp_mspol.$(OBJ) $(C_) $(GLSRC)gp_mspol.c

# end of winlib.mak
