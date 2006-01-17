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

# $Id: msvctail.mak,v 1.11 2005/03/04 21:58:55 ghostgum Exp $
# Common tail section for Microsoft Visual C++ 4.x/5.x,
# Windows NT or Windows 95 platform.
# Created 1997-05-22 by L. Peter Deutsch from msvc4/5 makefiles.
# edited 1997-06-xx by JD to factor out interpreter-specific sections
# edited 2000-06-05 by lpd to handle empty MSINCDIR specially.


# -------------------------- Auxiliary programs --------------------------- #

# This also creates the subdirectories since this (hopefully) will be the
# first need. Too bad nmake doesn't have .BEFORE symbolic target.
$(GLGENDIR)\ccf32.tr: $(TOP_MAKEFILES)
	-mkdir $(PSOBJDIR)
	-mkdir $(PSGENDIR)
	-mkdir $(GLOBJDIR)
	-mkdir $(GLGENDIR)
	-mkdir $(BINDIR)
	echo $(GENOPT) -DCHECK_INTERRUPTS -D_Windows -D__WIN32__ > $(GLGENDIR)\ccf32.tr

$(ECHOGS_XE): $(GLSRC)echogs.c
	$(CCAUX) $(GLSRC)echogs.c /Fo$(GLOBJ)echogs.obj /Fe$(ECHOGS_XE) $(CCAUX_TAIL)

# Don't create genarch if it's not needed
!ifdef GENARCH_XE
!ifdef WIN64
$(GENARCH_XE): $(GLSRC)genarch.c $(GENARCH_DEPS) $(GLGENDIR)\ccf32.tr
	$(CC) @$(GLGENDIR)\ccf32.tr /Fo$(GLOBJ)genarch.obj $(GLSRC)genarch.c
	$(LINK) $(LCT) $(LINKLIBPATH) $(GLOBJ)genarch.obj /OUT:$(GENARCH_XE)
!else
$(GENARCH_XE): $(GLSRC)genarch.c $(GENARCH_DEPS) $(GLGENDIR)\ccf32.tr
	$(CCAUX) @$(GLGENDIR)\ccf32.tr /Fo$(GLOBJ)genarch.obj /Fe$(GENARCH_XE) $(GLSRC)genarch.c $(CCAUX_TAIL)
!endif
!endif

$(GENCONF_XE): $(GLSRC)genconf.c $(GENCONF_DEPS)
	$(CCAUX) $(GLSRC)genconf.c /Fo$(GLOBJ)genconf.obj /Fe$(GENCONF_XE) $(CCAUX_TAIL)

$(GENDEV_XE): $(GLSRC)gendev.c $(GENDEV_DEPS)
	$(CCAUX) $(GLSRC)gendev.c /Fo$(GLOBJ)gendev.obj /Fe$(GENDEV_XE) $(CCAUX_TAIL)

$(GENHT_XE): $(GLSRC)genht.c $(GENHT_DEPS)
	$(CCAUX) $(GENHT_CFLAGS) $(GLSRC)genht.c /Fo$(GLOBJ)genht.obj /Fe$(GENHT_XE) $(CCAUX_TAIL)

# PSSRC and PSOBJ aren't defined yet, so we spell out the definitions.
$(GENINIT_XE): $(PSSRCDIR)$(D)geninit.c $(GENINIT_DEPS)
	$(CCAUX) $(PSSRCDIR)$(D)geninit.c /Fo$(PSOBJDIR)$(D)geninit.obj /Fe$(GENINIT_XE) $(CCAUX_TAIL)

# -------------------------------- Library -------------------------------- #

# See winlib.mak

# ----------------------------- Main program ------------------------------ #

LIBCTR=$(GLGEN)libc32.tr

$(LIBCTR): $(TOP_MAKEFILES)
	echo shell32.lib >$(LIBCTR)
	echo comdlg32.lib >>$(LIBCTR)
	echo gdi32.lib >>$(LIBCTR)
	echo user32.lib >>$(LIBCTR)
	echo winspool.lib >>$(LIBCTR)
	echo advapi32.lib >>$(LIBCTR)

# end of msvctail.mak
