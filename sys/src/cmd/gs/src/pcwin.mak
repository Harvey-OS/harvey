#    Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
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

# $Id: pcwin.mak,v 1.5 2002/10/29 09:22:29 ghostgum Exp $
# makefile for PC window system (MS Windows and OS/2) -specific device
# drivers.

# Define the name of this makefile.
PCWIN_MAK=$(GLSRC)pcwin.mak

# We have to isolate these in their own file because the MS Windows code
# requires special compilation switches, different from all other files
# and platforms.

### -------------------- The MS-Windows 3.n DLL ------------------------- ###

gp_mswin_h=$(GLSRC)gp_mswin.h
gsdll_h=$(GLSRC)gsdll.h
gsdllwin_h=$(GLSRC)gsdllwin.h

gdevmswn_h=$(GLSRC)gdevmswn.h $(GDEVH)\
 $(dos__h) $(memory__h) $(string__h) $(windows__h)\
 $(gp_mswin_h)

# This is deprecated and requires the interpreter / PSSRCDIR.
$(GLOBJ)gdevmswn.$(OBJ): $(GLSRC)gdevmswn.c $(gdevmswn_h) $(gp_h) $(gpcheck_h)\
 $(gsdll_h) $(gsdllwin_h) $(gsparam_h) $(gdevpccm_h)
	$(GLCCWIN) -I$(PSSRCDIR) $(GLO_)gdevmswn.$(OBJ) $(C_) $(GLSRC)gdevmswn.c

$(GLOBJ)gdevmsxf.$(OBJ): $(GLSRC)gdevmsxf.c $(ctype__h) $(math__h) $(memory__h) $(string__h)\
 $(gdevmswn_h) $(gsstruct_h) $(gsutil_h) $(gxxfont_h)
	$(GLCCWIN) $(GLO_)gdevmsxf.$(OBJ) $(C_) $(GLSRC)gdevmsxf.c

# An implementation using a DIB filled by an image device.
# This is deprecated and requires the interpreter / PSSRCDIR.
$(GLOBJ)gdevwdib.$(OBJ): $(GLSRC)gdevwdib.c\
 $(gdevmswn_h) $(gsdll_h) $(gsdllwin_h) $(gxdevmem_h)
	$(GLCCWIN) -I$(PSSRCDIR) $(GLO_)gdevwdib.$(OBJ) $(C_) $(GLSRC)gdevwdib.c

mswindll1_=$(GLOBJ)gdevmswn.$(OBJ) $(GLOBJ)gdevmsxf.$(OBJ) $(GLOBJ)gdevwdib.$(OBJ)
mswindll2_=$(GLOBJ)gdevemap.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ)
mswindll_=$(mswindll1_) $(mswindll2_)
$(GLGEN)mswindll.dev: $(mswindll_)
	$(SETDEV) $(GLGEN)mswindll $(mswindll1_)
	$(ADDMOD) $(GLGEN)mswindll $(mswindll2_)

### -------------------- The MS-Windows DDB 3.n printer ----------------- ###

mswinprn_=$(GLOBJ)gdevwprn.$(OBJ) $(GLOBJ)gdevmsxf.$(OBJ)
$(DD)mswinprn.dev: $(mswinprn_)
	$(SETDEV) $(DD)mswinprn $(mswinprn_)

$(GLOBJ)gdevwprn.$(OBJ): $(GLSRC)gdevwprn.c $(gdevmswn_h) $(gp_h)
	$(GLCCWIN) $(GLO_)gdevwprn.$(OBJ) $(C_) $(GLSRC)gdevwprn.c

### -------------------- The MS-Windows DIB 3.n printer ----------------- ###

mswinpr2_=$(GLOBJ)gdevwpr2.$(OBJ)
$(DD)mswinpr2.dev: $(mswinpr2_) $(GLD)page.dev
	$(SETPDEV) $(DD)mswinpr2 $(mswinpr2_)

$(GLOBJ)gdevwpr2.$(OBJ): $(GLSRC)gdevwpr2.c $(PDEVH) $(windows__h)\
 $(gdevpccm_h) $(gp_h) $(gp_mswin_h)
	$(GLCCWIN) $(GLO_)gdevwpr2.$(OBJ) $(C_) $(GLSRC)gdevwpr2.c

### ------------------ OS/2 Presentation Manager device ----------------- ###

os2pm_=$(GLOBJ)gdevpm.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ)
$(DD)os2pm.dev: $(os2pm_)
	$(SETDEV) $(DD)os2pm $(os2pm_)

os2dll_=$(GLOBJ)gdevpm.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ)
$(GLGEN)os2dll.dev: $(os2dll_)
	$(SETDEV) $(GLGEN)os2dll $(os2dll_)

$(GLOBJ)gdevpm.$(OBJ): $(GLSRC)gdevpm.c $(string__h)\
 $(gp_h) $(gpcheck_h)\
 $(gsdll_h) $(gsdllwin_h) $(gserrors_h) $(gsexit_h) $(gsparam_h)\
 $(gx_h) $(gxdevice_h) $(gxdevmem_h)\
 $(gdevpccm_h) $(GLSRC)gdevpm.h
	$(GLCC) $(GLO_)gdevpm.$(OBJ) $(C_) $(GLSRC)gdevpm.c

### --------------------------- The OS/2 printer ------------------------ ###

os2prn_=$(GLOBJ)gdevos2p.$(OBJ)
$(DD)os2prn.dev: $(os2prn_) $(GLD)page.dev
	$(SETPDEV) $(DD)os2prn $(os2prn_)

$(GLOBJ)gdevos2p.$(OBJ): $(GLSRC)gdevos2p.c $(gp_h) $(gdevpccm_h) $(gdevprn_h) $(gscdefs_h)
	$(GLCC) $(GLO_)gdevos2p.$(OBJ) $(C_) $(GLSRC)gdevos2p.c
