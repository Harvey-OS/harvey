#    Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: winint.mak,v 1.3 2000/03/17 03:01:58 lpd Exp $
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
WINZIPSE_XE="C:\Program Files\WinZip Self-Extractor\WZIPSE32.EXE"
!endif

# Define the name and location of the zip archive maker.
!ifndef ZIP_XE
ZIP_XE="zip.exe"
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

# ----------------------------- Main program ------------------------------ #

ICONS=$(GLGEN)gsgraph.ico $(GLGEN)gstext.ico

GS_ALL=$(INT_ALL) $(INTASM)\
  $(LIB_ALL) $(LIBCTR) $(GLGEN)lib.tr $(ld_tr) $(GSDLL_OBJ).res $(GLSRC)$(GSDLL).def $(ICONS)

dwdll_h=$(GLSRC)dwdll.h $(gsdllwin_h)
dwimg_h=$(GLSRC)dwimg.h
dwmain_h=$(GLSRC)dwmain.h
dwtext_h=$(GLSRC)dwtext.h

# Make the icons from their text form.

$(GLGEN)gsgraph.ico: $(GLSRC)gsgraph.icx $(ECHOGS_XE) $(WININT_MAK)
	$(ECHOGS_XE) -wb $(GLGEN)gsgraph.ico -n -X -r $(GLSRC)gsgraph.icx

$(GLGEN)gstext.ico: $(GLSRC)gstext.icx $(ECHOGS_XE) $(WININT_MAK)
	$(ECHOGS_XE) -wb $(GLGEN)gstext.ico -n -X -r $(GLSRC)gstext.icx

# resources for short EXE loader (no dialogs)
$(GS_OBJ).res: $(GLSRC)dwmain.rc $(dwmain_h) $(ICONS) $(WININT_MAK)
	$(ECHOGS_XE) -w $(GLGEN)_exe.rc -x 23 define -s gstext_ico $(GLGENDIR)\gstext.ico
	$(ECHOGS_XE) -a $(GLGEN)_exe.rc -x 23 define -s gsgraph_ico $(GLGENDIR)\gsgraph.ico
	$(ECHOGS_XE) -a $(GLGEN)_exe.rc -R $(GLSRC)dwmain.rc
	$(RCOMP) -I$(GLSRCDIR) -i$(INCDIR)$(_I) -r $(RO_)$(GS_OBJ).res $(GLGEN)_exe.rc
	del $(GLGEN)_exe.rc

# resources for main program (includes dialogs)
$(GSDLL_OBJ).res: $(GLSRC)gsdll32.rc $(gp_mswin_h) $(ICONS) $(WININT_MAK)
	$(ECHOGS_XE) -w $(GLGEN)_dll.rc -x 23 define -s gstext_ico $(GLGENDIR)\gstext.ico
	$(ECHOGS_XE) -a $(GLGEN)_dll.rc -x 23 define -s gsgraph_ico $(GLGENDIR)\gsgraph.ico
	$(ECHOGS_XE) -a $(GLGEN)_dll.rc -R $(GLSRC)gsdll32.rc
	$(RCOMP) -I$(GLSRCDIR) -i$(INCDIR)$(_I) -r $(RO_)$(GSDLL_OBJ).res $(GLGEN)_dll.rc
	del $(GLGEN)_dll.rc


# Modules for small EXE loader.

DWOBJ=$(GLOBJ)dwdll.obj $(GLOBJ)dwimg.obj $(GLOBJ)dwmain.obj $(GLOBJ)dwtext.obj $(GLOBJ)gscdefs.obj $(GLOBJ)gp_wgetv.obj

$(GLOBJ)dwdll.obj: $(GLSRC)dwdll.cpp $(AK)\
 $(dwdll_h) $(gsdll_h) $(gsdllwin_h)
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwdll.obj $(C_) $(GLSRC)dwdll.cpp

$(GLOBJ)dwimg.obj: $(GLSRC)dwimg.cpp $(AK)\
 $(dwmain_h) $(dwdll_h) $(dwtext_h) $(dwimg_h)\
 $(gscdefs_h) $(gsdll_h)
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwimg.obj $(C_) $(GLSRC)dwimg.cpp

$(GLOBJ)dwmain.obj: $(GLSRC)dwmain.cpp $(AK)\
 $(dwdll_h) $(gscdefs_h) $(gsdll_h)
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwmain.obj $(C_) $(GLSRC)dwmain.cpp

$(GLOBJ)dwtext.obj: $(GLSRC)dwtext.cpp $(AK) $(dwtext_h)
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwtext.obj $(C_) $(GLSRC)dwtext.cpp

# Modules for big EXE

DWOBJNO = $(GLOBJ)dwnodll.obj $(GLOBJ)dwimg.obj $(GLOBJ)dwmain.obj $(GLOBJ)dwtext.obj

$(GLOBJ)dwnodll.obj: $(GLSRC)dwnodll.cpp $(AK)\
 $(dwdll_h) $(gsdll_h) $(gsdllwin_h)
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwnodll.obj $(C_) $(GLSRC)dwnodll.cpp

# Compile gsdll.c, the main program of the DLL.

$(GLOBJ)gsdll.obj: $(GLSRC)gsdll.c $(AK) $(gsdll_h) $(ghost_h)
	$(PSCCWIN) $(COMPILE_FOR_DLL) $(GLO_)gsdll.$(OBJ) $(C_) $(GLSRC)gsdll.c

# Modules for console mode EXEs

OBJC=$(GLOBJ)dwmainc.obj $(GLOBJ)dwdllc.obj $(GLOBJ)gscdefs.obj $(GLOBJ)gp_wgetv.obj
OBJCNO=$(GLOBJ)dwmainc.obj $(GLOBJ)dwnodllc.obj

$(GLOBJ)dwmainc.obj: $(GLSRC)dwmainc.cpp $(AK) $(dwmain_h) $(dwdll_h) $(gscdefs_h) $(gsdll_h)
	$(GLCPP) $(COMPILE_FOR_CONSOLE_EXE) $(GLO_)dwmainc.obj $(C_) $(GLSRC)dwmainc.cpp

$(GLOBJ)dwdllc.obj: $(GLSRC)dwdll.cpp $(AK) $(dwdll_h) $(gsdll_h)
	$(GLCPP) $(COMPILE_FOR_CONSOLE_EXE) $(GLO_)dwdllc.obj $(C_) $(GLSRC)dwdll.cpp

$(GLOBJ)dwnodllc.obj: $(GLSRC)dwnodll.cpp $(AK) $(dwdll_h) $(gsdll_h)
	$(GLCPP) $(COMPILE_FOR_CONSOLE_EXE) $(GLO_)dwnodllc.obj $(C_) $(GLSRC)dwnodll.cpp


# ---------------------- Setup and uninstall program ---------------------- #


# Modules for setup program
# These modules shouldn't be referenced if MAKEDDLL==0,but dependencies here
# don't hurt.

$(GLOBJ)dwsetup.res: $(GLSRC)dwsetup.rc $(GLSRC)dwsetup.h $(GLGEN)gstext.ico
	$(RCOMP) -I$(GLSRCDIR) -i$(GLOBJDIR) -i$(INCDIR)$(_I) -r $(RO_)$(GLOBJ)dwsetup.res $(GLSRC)dwsetup.rc

$(GLOBJ)dwsetup.obj: $(GLSRC)dwsetup.cpp $(GLSRC)dwsetup.h $(GLSRC)dwinst.h
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwsetup.obj $(C_) $(GLSRC)dwsetup.cpp

$(GLOBJ)dwinst.obj: $(GLSRC)dwinst.cpp $(GLSRC)dwinst.h
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwinst.obj $(C_) $(GLSRC)dwinst.cpp

# Modules for uninstall program

$(GLOBJ)dwuninst.res: $(GLSRC)dwuninst.rc $(GLSRC)dwuninst.h $(GLGEN)gstext.ico
	$(RCOMP) -I$(GLSRCDIR) -i$(GLOBJDIR) -i$(INCDIR)$(_I) -r $(RO_)$(GLOBJ)dwuninst.res $(GLSRC)dwuninst.rc

$(GLOBJ)dwuninst.obj: $(GLSRC)dwuninst.cpp $(GLSRC)dwuninst.h
	$(GLCPP) $(COMPILE_FOR_EXE) $(GLO_)dwuninst.obj $(C_) $(GLSRC)dwuninst.cpp


# ------------------------- Distribution archive -------------------------- #

# ****** Aladdin Enterprises assumes no responsibility whatsoever for the
# ****** following section of this makefile.  If you have questions, please
# ****** contact bug-gswin@artifex.com or gsview@ghostgum.com.au.

# Create a self-extracting archive with setup program.
# This assumes that the current directory is named gs#.## relative to its
# parent, where #.## is the Ghostscript version, and that the files and
# directories listed in ZIPTEMPFILE and ZIPFONTFILES are the complete list
# of needed files and directories relative to the current directory's parent.

ZIPTEMPFILE=gs$(GS_DOT_VERSION)\obj\dwfiles.rsp
ZIPPROGFILE1=gs$(GS_DOT_VERSION)\bin\gsdll32.dll
ZIPPROGFILE2=gs$(GS_DOT_VERSION)\bin\gswin32.exe
ZIPPROGFILE3=gs$(GS_DOT_VERSION)\bin\gswin32c.exe
ZIPPROGFILE4=gs$(GS_DOT_VERSION)\bin\gs16spl.exe
ZIPPROGFILE5=gs$(GS_DOT_VERSION)\doc
ZIPPROGFILE6=gs$(GS_DOT_VERSION)\examples
ZIPPROGFILE7=gs$(GS_DOT_VERSION)\lib
ZIPFONTDIR=fonts
ZIPFONTFILES=$(ZIPFONTDIR)\*.*

# Make the zip archive.
FILELIST_TXT=filelist.txt
FONTLIST_TXT=fontlist.txt
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
	$(SETUP_XE_NAME) -title "Aladdin Ghostscript $(GS_DOT_VERSION)" -dir "gs$(GS_DOT_VERSION)" -list "$(FILELIST_TXT)" @$(ZIPTEMPFILE)
	$(SETUP_XE_NAME) -title "Aladdin Ghostscript Fonts" -dir "fonts" -list "$(FONTLIST_TXT)" $(ZIPFONTFILES)
	-del gs$(GS_VERSION)w32.zip
	$(ZIP_XE) -9 -r gs$(GS_VERSION)w32.zip $(SETUP_XE_NAME) $(UNINSTALL_XE_NAME) $(FILELIST_TXT) $(FONTLIST_TXT)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)w32.zip $(ZIPFONTDIR)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)w32.zip $(ZIPPROGFILE1)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)w32.zip $(ZIPPROGFILE2)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)w32.zip $(ZIPPROGFILE3)
	rem
	rem	Don't flag error if Win32s spooler file is missing.
	rem	This occurs when using MSVC++.
	rem
	-$(ZIP_XE) -9 -r gs$(GS_VERSION)w32.zip $(ZIPPROGFILE4)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)w32.zip $(ZIPPROGFILE5)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)w32.zip $(ZIPPROGFILE6)
	$(ZIP_XE) -9 -r gs$(GS_VERSION)w32.zip $(ZIPPROGFILE7)
	-del $(ZIPTEMPFILE)
	-del $(SETUP_XE_NAME)
	-del $(UNINSTALL_XE_NAME)
	-del $(FILELIST_TXT)
	-del $(FONTLIST_TXT)
	cd gs$(GS_DOT_VERSION)

# Now convert to a self extracting archive.
# This involves making a few temporary files.
ZIP_RSP = $(GLOBJ)setupgs.rsp
# Note that we use ECHOGS_XE rather than echo for the .txt files
# to avoid ANSI/OEM character mapping.
archive: zip $(GLOBJ)gstext.ico $(ECHOGS_XE)
	$(ECHOGS_XE) -w $(ZIP_RSP) -q "-win32 -setup"
	$(ECHOGS_XE) -a $(ZIP_RSP) -q -st -x 22 Aladdin Ghostscript $(GS_DOT_VERSION) for Win32 -x 22
	$(ECHOGS_XE) -a $(ZIP_RSP) -q -i -s $(GLOBJ)gstext.ico
	$(ECHOGS_XE) -a $(ZIP_RSP) -q -a -s $(GLOBJ)about.txt
	$(ECHOGS_XE) -a $(ZIP_RSP) -q -t -s $(GLOBJ)dialog.txt
	$(ECHOGS_XE) -a $(ZIP_RSP) -q -c -s $(SETUP_XE_NAME)
	$(ECHOGS_XE) -w $(GLOBJ)about.txt "Aladdin Ghostscript is Copyright " -x A9 " 1999 Aladdin Enterprises."
	$(ECHOGS_XE) -a $(GLOBJ)about.txt See license in gs$(GS_DOT_VERSION)\doc\PUBLIC.
	$(ECHOGS_XE) -a $(GLOBJ)about.txt See gs$(GS_DOT_VERSION)\doc\Commprod.htm regarding commercial distribution.
	$(ECHOGS_XE) -w $(GLOBJ)dialog.txt This installs Aladdin Ghostscript $(GS_DOT_VERSION).
	$(ECHOGS_XE) -a $(GLOBJ)dialog.txt Aladdin Ghostscript displays, prints and converts PostScript and PDF files.
	$(WINZIPSE_XE) ..\gs$(GS_VERSION)w32 @$(GLOBJ)setupgs.rsp
# Don't delete temporary files, because make continues
# before these files are used.
#	-del $(ZIP_RSP)
#	-del $(GLOBJ)about.txt
#	-del $(GLOBJ)dialog.txt


# end of winint.mak
