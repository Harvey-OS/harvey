#    Copyright (C) 2001 Aladdin Enterprises.  All rights reserved.
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

# $Id: icclib.mak,v 1.3.2.1 2001/10/26 00:15:30 giles Exp $
# makefile for icclib library code.
# Users of this makefile must define the following:
#	GLSRCDIR - the graphic library source directory
#	ICCSRCDIR - the icclib source directory
#	ICCGENDIR - the generated intermediate file directory
#	ICCOBJDIR - the object directory

# This partial makefile compiles Graeme W. Gill's icclibfor use in Ghostscript.
#
# The original source for the code in this directory may be accessed via
#   http://web.access.net.au/argyll/color.html
# For information on ICC color profiles in general, see the International
# Color Consortium's web site at
#   http://www.color.org
#
# This makefile has been tested with version 2.0 of the icclib code. If you
# are working with a later version, you may need to update the ICC profile
# version number macro ICCPROFVER.

ICCPROFVER=9809

ICCSRC=$(ICCSRCDIR)$(D)
ICCGEN=$(ICCGENDIR)$(D)
ICCOBJ=$(ICCOBJDIR)$(D)
ICCO_=$(O_)$(ICCOBJ)

# We need D_, _D_, and _D because the OpenVMS compiler uses different
# syntax from other compilers.
# ICCI_ and ICCF_ are defined in gs.mak.
ICC_INCL=$(I_)$(ICCI_) $(II)$(GLSRCDIR) $(II)$(GLGENDIR)$(_I)
ICC_CCFLAGS=$(ICC_INCL) $(ICCF_) 
ICC_CC=$(CC_) $(ICC_CCFLAGS)

# Define the name of this makefile.
ICCLIB_MAK=$(GLSRC)icclib.mak

icc.clean : icc.config-clean icc.clean-not-config-clean

### WRONG.  MUST DELETE OBJ AND GEN FILES SELECTIVELY.
icc.clean-not-config-clean :
#	echo $(ICCSRC) $(ICCGEN) $(ICCOBJ) $(ICCO_)
	$(EXP)$(ECHOGS_XE) $(ICCSRC) $(ICCGEN) $(ICCOBJ) $(ICCO_)
	$(RM_) $(ICCOBJ)*.$(OBJ)

icc.config-clean :
	$(RMN_) $(ICCGEN)icclib*.dev

ICCDEP=$(AK)

# Code common to compression and decompression.

icclib_=$(ICCOBJ)icc.$(OBJ)
$(ICCGEN)icclib.dev : $(ICCLIB_MAK) $(ECHOGS_XE) $(icclib_)
	$(SETMOD) $(ICCGEN)icclib $(icclib_)

icc_h=$(ICCSRC)$(D)icc.h $(ICCSRC)$(D)icc$(ICCPROFVER).h

$(ICCOBJ)icc.$(OBJ) : $(ICCSRC)icc.c $(ICCDEP) $(icc_h)
#	echo $(ICC_CCFLAGS)
	$(EXP)$(ECHOGS_XE) $(ICC_CCFLAGS)
	$(ICC_CC) $(ICCO_)icc.$(OBJ) $(C_) $(ICCSRC)icc.c
