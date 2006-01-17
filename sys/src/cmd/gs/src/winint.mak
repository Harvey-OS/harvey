#    Copyright (C) 1997, 2003 Aladdin Enterprises.  All rights reserved.
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

# $Id: winint.mak,v 1.26 2005/04/12 22:05:40 giles Exp $
# Common interpreter makefile section for 32-bit MS Windows.

# This makefile must be acceptable to Microsoft Visual C++, Watcom C++,
# and Borland C++.  For this reason, the only conditional directives
# allowed are !if[n]def, !else, and !endif.


# Include the generic makefile.
!include $(PSSRCDIR)\int.mak
!include $(PSSRCDIR)\cfonts.mak

# Define the C++ compiler invocation for library modules.
GLCPP=$(CPP) $(CO) $(I_)$(GLI_)$(_I)

# Define the compilation rule for Windows interpreter code.
# This requires PS*_ to be defined, so it has to come after int.mak.
PSCCWIN=$(CC_WX) $(CCWINFLAGS) $(I_)$(PSI_)$(_I) $(PSF_)

# Define the name of this makefile.
WININT_MAK=$(PSSRC)winint.mak

# Define the location of the WinZip self-extracting-archive-maker.
!ifndef WINZIPSE_XE
!ifdef WIN64
WINZIPSE_XE="C:\Program Files (x86)\WinZip Self-Extractor\WZIPSE32.EXE"
!else
WINZIPSE_XE="C:\Program Files\WinZip Self-Extractor\WZIPSE32.EXE"
!endif
!endif

# Define the name and location of the zip archive maker.
!ifndef ZIP_XE
ZIP_XE="zip.exe" -X
!endif

# Define the setup and install programs, which are only suitable
# for the DLL build.
# If MAKEDLL==0, these names are never referenced.
!ifndef SETUP_XE_NAME
SETUP_XE_NAME=setupgs.exe
!endif
!ifndef SETUP_XE
SETUP_XE=$(BINDIR)\$(SETUP_XE_NAME)
!endif
!ifndef UNINSTALL_XE_NAME
UNINSTALL_XE_NAME=uninstgs.exe
!endif
!ifndef UNINSTALL_XE
UNINSTALL_XE=$(BINDIR)\$(UNINSTALL_XE_NAME)
!endif

# Define the RCOMP switch for including INCDIR.
!if "$(INCDIR)"==""
i_INCDIR=
!else
i_INCDIR=-i$(INCDIR)
!endif


# ----------------------------- Main program ------------------------------ #

ICONS=$(GLGEN)gswin.ico $(GLGEN)gswin16.ico

GS_ALL=$(INT_ALL) $(INTASM)\
  $(LIB_ALL) $(LIBCTR) $(ld_tr) $(GSDLL_OBJ).res $(PSSRC)$(GSDLL).def $(ICONS)

dwdll_h=$(PSSRC)dwdll.h
dwimg_h=$(GLSRC)dwimg.h
dwtrace_h=$(GLSRC)dwtrace.h
dwmain_h=$(PSSRC)dwmain.h
dwtext_h=$(GLSRC)dwtext.h
dwreg_h=$(GLSRC)dwreg.h

# Make the icons from their text form.

$(GLGEN)gswin.ico: $(GLSRC)gswin.icx $(ECHOGS_XE) $(WININT_MAK)
	$(ECHOGS_XE) -wb $(GLGEN)gswin.ico -n -X -r $(GLSRC)gswin.icx

$(GLGEN)gswin16.ico: $(GLSRC)gswin16.icx $(ECHOGS_XE) $(WININT_MAK)
	$(ECHOGS_XE) -wb $(GLGEN)gswin16.ico -n -X -r $(GLSRC)gswin16.icx

# resources for short EXE loader (no dialogs)
$(GS_OBJ).res: $(PSSRC)dwmain.rc $(dwmain_h) $(ICONS) $(WININT_MAK)
	$(ECHOGS_XE) -w $(PSGEN)_exe.rc -x 23 define -s gstext_ico $(GLGENDIR)\gswin.ico
	$(ECHOGS_XE) -a $(PSGEN)_exe.rc -x 23 define -s gsgraph_ico $(GLGENDIR)\gswin.ico
	$(ECHOGS_XE) -a $(PSGEN)_exe.rc -R $(PSSRC)dwmain.rc
	$(RCOMP) -i$(PSSRCDIR) -i$(PSGENDIR) $(i_INCDIR) -r $(RO_)$(GS_OBJ).res $(PSGEN)_exe.rc
	del $(PSGEN)_exe.rc

# resources for main program (includes dialogs)
$(GSDLL_OBJ).res: $(PSSRC)gsdll32.rc $(gp_mswin_h) $(ICONS) $(WININT_MAK)
	$(ECHOGS_XE) -w $(PSGEN)_dll.rc -x 23 define -s gstext_ico $(GLGENDIR)\gswin.ico
	$(ECHOGS_XE) -a $(PSGEN)_dll.rc -x 23 define -s gsgraph_ico $(GLGENDIR)\gswin.ico
	$(ECHOGS_XE) -a $(PSGEN)_dll.rc -R $(PSSRC)gsdll32.rc
	$(RCOMP) -i$(PSSRCDIR) -i$(PSGENDIR) -i$(GLSRCDIR) $(i_INCDIR) -r $(RO_)$(GSDLL_OBJ).res $(PSGEN)_dll.rc
	del $(PSGEN)_dll.rc


# Modules for big EXE

!if $(DEBUG)
DWTRACE=$(GLOBJ)dwtrace.obj
!else
DWTRACE=
!endif


DWOBJNO = $(PSOBJ)dwnodll.obj $(GLOBJ)dwimg.obj $(DWTRACE) $(PSOBJ)dwmain.obj \
$(GLOBJ)dwtext.obj $(GLOBJ)dwreg.obj

$(PSOBJ)dwnodll.obj: $(PSSRC)dwnodll.c $(AK)\
 $(dwdll_h) $(iapi_h)
	$(PSCCWIN) $(COMPILE_FOR_EXE) $(PSO_)dwnodll.obj $(C_) $(PSSRC)dwnodll.c

# Compile gsdll.c, the main program of the DLL.

$(PSOBJ)gsdll.obj: $(PSSRC)gsdll.c $(AK) $(iapi_h) $(ghost_h)
	$(PSCCWIN) $(COMPILE_FOR_DLL) $(PSO_)gsdll.$(OBJ) $(C_) $(PSSRC)gsdll.c

$(GLOBJ)gp_msdll.obj: $(GLSRC)gp_msdll.c $(AK) $(iapi_h)
	$(PSCCWIN) $(COMPILE_FOR_DLL) $(GLO_)gp_msdll.$(OBJ) $(C_) $(GLSRC)gp_msdll.c

# Modules for console mode EXEs

OBJC=$(PSOBJ)dwmainc.obj $(PSOBJ)dwdllc.obj $(GLOBJ)gscdefs.obj $(GLOBJ)gp_wgetv.obj \
$(GLOBJ)dwimg.obj $(DWTRACE) $(GLOBJ)dwreg.obj

OBJCNO=$(PSOBJ)dwmainc.obj $(PSOBJ)dwnodllc.obj $(GLOBJ)dwimg.obj $(DWTRACE) $(GLOBJ)dwreg.obj

$(PSOBJ)dwmainc.obj: $(PSSRC)dwmainc.c $(AK) $(windows__h) $(fcntl__h) $(unistd__h) \
  $(iapi_h) $(vdtrace_h) $(gdevdsp_h) $(dwdll_h) $(dwimg_h) $(dwtrace_h)
	$(PSCCWIN) $(COMPILE_FOR_CONSOLE_EXE) $(PSO_)dwmainc.obj $(C_) $(PSSRC)dwmainc.c

$(PSOBJ)dwdllc.obj: $(PSSRC)dwdll.c $(AK) $(dwdll_h) $(iapi_h)
	$(PSCCWIN) $(COMPILE_FOR_CONSOLE_EXE) $(PSO_)dwdllc.obj $(C_) $(PSSRC)dwdll.c

$(PSOBJ)dwnodllc.obj: $(PSSRC)dwnodll.c $(AK) $(dwdll_h) $(iapi_h)
	$(PSCCWIN) $(COMPILE_FOR_CONSOLE_EXE) $(PSO_)dwnodllc.obj $(C_) $(PSSRC)dwnodll.c


# Modules for small EXE loader.

DWOBJ=$(PSOBJ)dwdll.obj $(GLOBJ)dwimg.obj $(DWTRACE) $(PSOBJ)dwmain.obj \
$(GLOBJ)dwtext.obj $(GLOBJ)gscdefs.obj $(GLOBJ)gp_wgetv.obj $(GLOBJ)dwreg.obj

$(PSOBJ)dwdll.obj: $(PSSRC)dwdll.c $(AK)\
 $(dwdll_h) $(iapi_h)
	$(PSCCWIN) $(COMPILE_FOR_EXE) $(PSO_)dwdll.obj $(C_) $(PSSRC)dwdll.c

$(GLOBJ)dwimg.obj: $(GLSRC)dwimg.c $(AK)\
 $(dwmain_h) $(dwdll_h) $(dwtext_h) $(dwimg_h) $(gdevdsp_h) $(stdio__h) \
 $(gscdefs_h) $(iapi_h) $(dwreg_h)
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwimg.obj $(C_) $(GLSRC)dwimg.c

$(GLOBJ)dwtrace.obj: $(GLSRC)dwtrace.c $(AK)\
 $(dwimg_h) $(dwtrace_h)\
 $(gscdefs_h) $(stdpre_h) $(gsdll_h) $(vdtrace_h)
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwtrace.obj $(C_) $(GLSRC)dwtrace.c

$(PSOBJ)dwmain.obj: $(PSSRC)dwmain.c $(AK)  $(windows__h) \
 $(iapi_h) $(vdtrace_h) $(dwmain_h) $(dwdll_h) $(dwtext_h) $(dwimg_h) $(dwtrace_h) \
 $(dwreg_h) $(gdevdsp_h)
	$(PSCCWIN) $(COMPILE_FOR_EXE) $(PSO_)dwmain.obj $(C_) $(PSSRC)dwmain.c

$(GLOBJ)dwtext.obj: $(GLSRC)dwtext.c $(AK) $(dwtext_h)
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwtext.obj $(C_) $(GLSRC)dwtext.c

$(GLOBJ)dwreg.obj: $(PSSRC)dwreg.c $(AK) $(dwreg_h)
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwreg.obj $(C_) $(GLSRC)dwreg.c


# ---------------------- Setup and uninstall program ---------------------- #


# Modules for setup program
# These modules shouldn't be referenced if MAKEDDLL==0,but dependencies here
# don't hurt.

$(PSOBJ)dwsetup.res: $(PSSRC)dwsetup.rc $(PSSRC)dwsetup.h $(GLGEN)gswin.ico
	$(RCOMP) -i$(PSSRCDIR) -i$(PSGENDIR) -i$(PSOBJDIR) $(i_INCDIR) -r $(RO_)$(PSOBJ)dwsetup.res $(PSSRC)dwsetup.rc

$(PSOBJ)dwsetup.obj: $(PSSRC)dwsetup.cpp $(PSSRC)dwsetup.h $(PSSRC)dwinst.h
	$(PSCCWIN) $(COMPILE_FOR_EXE) $(PSO_)dwsetup.obj $(C_) $(PSSRC)dwsetup.cpp

$(PSOBJ)dwinst.obj: $(PSSRC)dwinst.cpp $(PSSRC)dwinst.h
	$(PSCCWIN) $(COMPILE_FOR_EXE) $(PSO_)dwinst.obj $(C_) $(PSSRC)dwinst.cpp

# Modules for uninstall program

$(PSOBJ)dwuninst.res: $(PSSRC)dwuninst.rc $(PSSRC)dwuninst.h $(GLGEN)gswin.ico
	$(RCOMP) -i$(PSSRCDIR) -i$(PSGENDIR) -i$(PSOBJDIR) $(i_INCDIR) -r $(RO_)$(PSOBJ)dwuninst.res $(PSSRC)dwuninst.rc

$(PSOBJ)dwuninst.obj: $(PSSRC)dwuninst.cpp $(PSSRC)dwuninst.h
	$(PSCCWIN) $(COMPILE_FOR_EXE) $(PSO_)dwuninst.obj $(C_) $(PSSRC)dwuninst.cpp


# ------------------------- Distribution archive -------------------------- #

# The following section of this makefile was developed by, and is maintained
# by, the developer of GSview.  If you have any questions about it, please
# contact bug-gswin@ghostscript.com.

# Create a self-extracting archive with setup program.
# This assumes that the current directory is named gs#.## relative to its
# parent, where #.## is the Ghostscript version, and that the files and
# directories listed in ZIPTEMPFILE and ZIPFONTFILES are the complete list
# of needed files and directories relative to the current directory's parent.

ZIPTEMPFILE=gs$(GS_DOT_VERSION)\obj\dwfiles.rsp
ZIPPROGFILE1=gs$(GS_DOT_VERSION)\bin\gsdll32.dll
ZIPPROGFILE2=gs$(GS_DOT_VERSION)\bin\gsdll32.lib
ZIPPROGFILE3=gs$(GS_DOT_VERSION)\bin\gswin32.exe
ZIPPROGFILE4=gs$(GS_DOT_VERSION)\bin\gswin32c.exe
ZIPPROGFILE5=gs$(GS_DOT_VERSION)\bin\gs16spl.exe
ZIPPROGFILE6=gs$(GS_DOT_VERSION)\doc
ZIPPROGFILE7=gs$(GS_DOT_VERSION)\examples
ZIPPROGFILE8=gs$(GS_DOT_VERSION)\lib
ZIPPROGFILE9=gs$(GS_DOT_VERSION)\Resource
ZIPFONTDIR=fonts
ZIPFONTFILES=$(ZIPFONTDIR)\*.*

# Make the zip archive.
FILELIST_TXT=filelist.txt
FONTLIST_TXT=fontlist.txt
!ifdef WIN64
ZIPTARGET=gs$(GS_VERSION)w64
!else
ZIPTARGET=gs$(GS_VERSION)w32
!endif
zip: $(SETUP_XE) $(UNINSTALL_XE)
	cd ..
	copy gs$(GS_DOT_VERSION)\$(SETUP_XE) .
	copy gs$(GS_DOT_VERSION)\$(UNINSTALL_XE) .
	echo $(ZIPPROGFILE1) >  $(ZIPTEMPFILE)
	echo $(ZIPPROGFILE2) >> $(ZIPTEMPFILE)
	echo $(ZIPPROGFILE3) >> $(ZIPTEMPFILE)
	echo $(ZIPPROGFILE4) >> $(ZIPTEMPFILE)
	echo $(ZIPPROGFILE5) >> $(ZIPTEMPFILE)
	echo $(ZIPPROGFILE6) >> $(ZIPTEMPFILE)
	echo $(ZIPPROGFILE7) >> $(ZIPTEMPFILE)
	echo $(ZIPPROGFILE8) >> $(ZIPTEMPFILE)
	echo $(ZIPPROGFILE9) >> $(ZIPTEMPFILE)
	$(SETUP_XE_NAME) -title "AFPL Ghostscript $(GS_DOT_VERSION)" -dir "gs$(GS_DOT_VERSION)" -list "$(FILELIST_TXT)" @$(ZIPTEMPFILE)
	$(SETUP_XE_NAME) -title "AFPL Ghostscript Fonts" -dir "fonts" -list "$(FONTLIST_TXT)" $(ZIPFONTFILES)
	-del $(ZIPTARGET).zip
	$(ZIP_XE) -9 $(ZIPTARGET).zip $(SETUP_XE_NAME) $(UNINSTALL_XE_NAME) $(FILELIST_TXT) $(FONTLIST_TXT)
	$(ZIP_XE) -9 -r $(ZIPTARGET).zip $(ZIPFONTDIR)
	$(ZIP_XE) -9 -r $(ZIPTARGET).zip $(ZIPPROGFILE1)
	$(ZIP_XE) -9 -r $(ZIPTARGET).zip $(ZIPPROGFILE2)
	$(ZIP_XE) -9 -r $(ZIPTARGET).zip $(ZIPPROGFILE3)
	$(ZIP_XE) -9 -r $(ZIPTARGET).zip $(ZIPPROGFILE4)
	rem
	rem	Don't flag error if Win32s spooler file is missing.
	rem	This occurs when using MSVC++.
	rem
	-$(ZIP_XE) -9 -r $(ZIPTARGET).zip $(ZIPPROGFILE5)
	$(ZIP_XE) -9 -r $(ZIPTARGET).zip $(ZIPPROGFILE6)
	$(ZIP_XE) -9 -r $(ZIPTARGET).zip $(ZIPPROGFILE7)
	$(ZIP_XE) -9 -r $(ZIPTARGET).zip $(ZIPPROGFILE8)
	$(ZIP_XE) -9 -r $(ZIPTARGET).zip $(ZIPPROGFILE9)
	-del $(ZIPTEMPFILE)
	-del $(SETUP_XE_NAME)
	-del $(UNINSTALL_XE_NAME)
	-del $(FILELIST_TXT)
	-del $(FONTLIST_TXT)
	cd gs$(GS_DOT_VERSION)

# Now convert to a self extracting archive.
# This involves making a few temporary files.
ZIP_RSP = $(PSOBJ)setupgs.rsp
# Note that we use ECHOGS_XE rather than echo for the .txt files
# to avoid ANSI/OEM character mapping.
# Use a special icon WinZip SE can't handle 48 pixel 32-bit icons 
# as used by Windows XP.
archive: zip $(PSOBJ)gswin16.ico $(ECHOGS_XE)
	$(ECHOGS_XE) -w $(ZIP_RSP) -q "-win32 -setup"
	$(ECHOGS_XE) -a $(ZIP_RSP) -q -st -x 22 AFPL Ghostscript $(GS_DOT_VERSION) for Win32 -x 22
	$(ECHOGS_XE) -a $(ZIP_RSP) -q -i -s $(PSOBJ)gswin16.ico
	$(ECHOGS_XE) -a $(ZIP_RSP) -q -a -s $(PSOBJ)about.txt
	$(ECHOGS_XE) -a $(ZIP_RSP) -q -t -s $(PSOBJ)dialog.txt
	$(ECHOGS_XE) -a $(ZIP_RSP) -q -c -s $(SETUP_XE_NAME)
	$(ECHOGS_XE) -w $(PSOBJ)about.txt "AFPL Ghostscript is Copyright " -x A9 " 2005 artofcode LLC."
	$(ECHOGS_XE) -a $(PSOBJ)about.txt See license in gs$(GS_DOT_VERSION)\doc\Public.htm.
	$(ECHOGS_XE) -a $(PSOBJ)about.txt See gs$(GS_DOT_VERSION)\doc\Commprod.htm regarding commercial distribution.
	$(ECHOGS_XE) -w $(PSOBJ)dialog.txt This installs AFPL Ghostscript $(GS_DOT_VERSION).
	$(ECHOGS_XE) -a $(PSOBJ)dialog.txt AFPL Ghostscript displays, prints and converts PostScript and PDF files.
	$(WINZIPSE_XE) ..\$(ZIPTARGET) @$(PSOBJ)setupgs.rsp
# Don't delete temporary files, because make continues
# before these files are used.
#	-del $(ZIP_RSP)
#	-del $(PSOBJ)about.txt
#	-del $(PSOBJ)dialog.txt


# end of winint.mak
