#    Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
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

# $Id: winplat.mak,v 1.1 2000/03/09 08:40:44 lpd Exp $
# Common makefile section for builds on 32-bit MS Windows, including the
# Watcom MS-DOS build.

# Define the name of this makefile.
WINPLAT_MAK=$(GLSRC)winplat.mak

# Define generic Windows-specific modules.

winplat_=$(GLOBJ)gp_ntfs.$(OBJ) $(GLOBJ)gp_win32.$(OBJ)
$(GLD)winplat.dev : $(WINPLAT_MAK) $(ECHOGS_XE) $(winplat_)
	$(SETMOD) $(GLD)winplat $(winplat_)

$(GLOBJ)gp_ntfs.$(OBJ): $(GLSRC)gp_ntfs.c $(AK)\
 $(dos__h) $(memory__h) $(stdio__h) $(string__h) $(windows__h)\
 $(gp_h) $(gsmemory_h) $(gsstruct_h) $(gstypes_h) $(gsutil_h)
	$(GLCCWIN) $(GLO_)gp_ntfs.$(OBJ) $(C_) $(GLSRC)gp_ntfs.c

$(GLOBJ)gp_win32.$(OBJ): $(GLSRC)gp_win32.c $(AK)\
 $(dos__h) $(malloc__h) $(stdio__h) $(string__h) $(windows__h)\
 $(gp_h) $(gsmemory_h) $(gstypes_h)
	$(GLCCWIN) $(GLO_)gp_win32.$(OBJ) $(C_) $(GLSRC)gp_win32.c

# Define the Windows thread / synchronization module.

winsync_=$(GLOBJ)gp_wsync.$(OBJ)
$(GLD)winsync.dev : $(WINPLAT_MAK) $(ECHOGS_XE) $(winsync_)
	$(SETMOD) $(GLD)winsync $(winsync_)
	$(ADDMOD) $(GLD)winsync -replace $(GLD)nosync

$(GLOBJ)gp_wsync.$(OBJ): $(GLSRC)gp_wsync.c $(AK)\
 $(dos__h) $(malloc__h) $(stdio__h) $(string__h) $(windows__h)\
 $(gp_h) $(gsmemory_h) $(gstypes_h)
	$(GLCCWIN) $(GLO_)gp_wsync.$(OBJ) $(C_) $(GLSRC)gp_wsync.c
