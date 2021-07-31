#    Copyright (C) 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
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

# $Id: libpng.mak,v 1.1 2000/03/09 08:40:44 lpd Exp $
# makefile for PNG (Portable Network Graphics) code.
# Users of this makefile must define the following:
#	ZSRCDIR - the zlib source directory
#	ZGENDIR - the zlib generated intermediate file directory
#	ZOBJDIR - the zlib object directory
#	PNGSRCDIR - the source directory
#	PNGGENDIR - the generated intermediate file directory
#	PNGOBJDIR - the object directory
#	PNGVERSION - the library version number ("89", "90", "95", "96",
#	  "10001", "10002", "10003", "10005").
#	  For historical reasons, "101" and "102" are also acceptable,
#	  even though they don't match libpng's numbering scheme
#	  (see png.h for more details).
#	SHARE_LIBPNG - 0 to compile libpng, 1 to share
#	LIBPNG_NAME - if SHARE_LIBPNG=1, the name of the shared library
# NOTE: currently users of this makefile define PSRCDIR and PVERSION,
# not PNGSRCDIR and PNGVERSION.  This is a bug.

# This partial makefile compiles the png library for use in the Ghostscript
# PNG drivers.  You can get the source code for this library from:
#   ftp://swrinde.nde.swri.edu/pub/png/src/
#   ftp://ftp.uu.net/graphics/png/src/
# Please see Ghostscript's `Make.htm' file for instructions about how to
# unpack these archives.
#
# When each version of Ghostscript is released, we copy the associated
# version of the png library to
#	ftp://ftp.cs.wisc.edu/ghost/3rdparty/
# for more convenient access.
#
# The makefile is known to work with the following library versions:
# 0.89, 0.90, 0.95, 0.96, 1.0.1, 1.0.2, 1.0.3, 1.0.5.
# NOTE: the archive for libpng 0.95 may be inconsistent: if you have
# compilation problems, use an older version.
# Please see Ghostscript's Make.htm file for instructions about how to
# unpack these archives.
#
# The specification for the PNG file format is available from:
#   http://www.group42.com/png.htm
#   http://www.w3.org/pub/WWW/TR/WD-png

# (Rename directories.)
PNGSRCDIR=$(PSRCDIR)
PNGVERSION=$(PVERSION)
PNGSRC=$(PNGSRCDIR)$(D)
PNGGEN=$(PNGGENDIR)$(D)
PNGOBJ=$(PNGOBJDIR)$(D)
PNGO_=$(O_)$(PNGOBJ)
PZGEN=$(ZGENDIR)$(D)

# PI_ and PF_ are defined in gs.mak.
PNGCC=$(CC_) $(I_)$(PI_)$(_I) $(PF_)

# Define the name of this makefile.
LIBPNG_MAK=$(GLSRC)libpng.mak

png.clean : png.config-clean png.clean-not-config-clean

### WRONG.  MUST DELETE OBJ AND GEN FILES SELECTIVELY.
png.clean-not-config-clean :
	$(RM_) $(PNGOBJ)*.$(OBJ)

png.config-clean :
	$(RM_) $(PNGGEN)lpg*.dev

PDEP=$(AK)

png_1=$(PNGOBJ)png.$(OBJ) $(PNGOBJ)pngmem.$(OBJ) $(PNGOBJ)pngerror.$(OBJ)
png_2=$(PNGOBJ)pngtrans.$(OBJ) $(PNGOBJ)pngwrite.$(OBJ) $(PNGOBJ)pngwtran.$(OBJ) $(PNGOBJ)pngwutil.$(OBJ)

# libpng modules

$(PNGOBJ)png.$(OBJ) : $(PNGSRC)png.c $(PDEP)
	$(PNGCC) $(PNGO_)png.$(OBJ) $(C_) $(PNGSRC)png.c

$(PNGOBJ)pngwio.$(OBJ) : $(PNGSRC)pngwio.c $(PDEP)
	$(PNGCC) $(PNGO_)pngwio.$(OBJ) $(C_) $(PNGSRC)pngwio.c

$(PNGOBJ)pngmem.$(OBJ) : $(PNGSRC)pngmem.c $(PDEP)
	$(PNGCC) $(PNGO_)pngmem.$(OBJ) $(C_) $(PNGSRC)pngmem.c

$(PNGOBJ)pngerror.$(OBJ) : $(PNGSRC)pngerror.c $(PDEP)
	$(PNGCC) $(PNGO_)pngerror.$(OBJ) $(C_) $(PNGSRC)pngerror.c

$(PNGOBJ)pngtrans.$(OBJ) : $(PNGSRC)pngtrans.c $(PDEP)
	$(PNGCC) $(PNGO_)pngtrans.$(OBJ) $(C_) $(PNGSRC)pngtrans.c

$(PNGOBJ)pngwrite.$(OBJ) : $(PNGSRC)pngwrite.c $(PDEP)
	$(PNGCC) $(PNGO_)pngwrite.$(OBJ) $(C_) $(PNGSRC)pngwrite.c

$(PNGOBJ)pngwtran.$(OBJ) : $(PNGSRC)pngwtran.c $(PDEP)
	$(PNGCC) $(PNGO_)pngwtran.$(OBJ) $(C_) $(PNGSRC)pngwtran.c

$(PNGOBJ)pngwutil.$(OBJ) : $(PNGSRC)pngwutil.c $(PDEP)
	$(PNGCC) $(PNGO_)pngwutil.$(OBJ) $(C_) $(PNGSRC)pngwutil.c

# Define the version of libpng.dev that we are actually using.
$(PNGGEN)libpng.dev : $(TOP_MAKEFILES) $(PNGGEN)libpng_$(SHARE_LIBPNG).dev
	$(CP_) $(PNGGEN)libpng_$(SHARE_LIBPNG).dev $(PNGGEN)libpng.dev

# Define the shared version of libpng.
# Note that it requires libz, which must be searched *after* libpng.
$(PNGGEN)libpng_1.dev : $(TOP_MAKEFILES) $(LIBPNG_MAK) $(ECHOGS_XE) $(PZGEN)zlibe.dev
	$(SETMOD) $(PNGGEN)libpng_1 -lib $(LIBPNG_NAME)
	$(ADDMOD) $(PNGGEN)libpng_1 -include $(PZGEN)zlibe.dev

# Define the non-shared version of libpng.
$(PNGGEN)libpng_0.dev : $(LIBPNG_MAK) $(ECHOGS_XE) $(png_1) $(png_2)\
 $(PZGEN)zlibe.dev $(PNGGEN)lpg$(PNGVERSION).dev
	$(SETMOD) $(PNGGEN)libpng_0 $(png_1)
	$(ADDMOD) $(PNGGEN)libpng_0 $(png_2)
	$(ADDMOD) $(PNGGEN)libpng_0 -include $(PZGEN)zlibe.dev $(PNGGEN)lpg$(PNGVERSION).dev

$(PNGGEN)lpg89.dev : $(LIBPNG_MAK) $(ECHOGS_XE) $(PNGOBJ)pngwio.$(OBJ)
	$(SETMOD) $(PNGGEN)lpg89 $(PNGOBJ)pngwio.$(OBJ)

$(PNGGEN)lpg90.dev : $(LIBPNG_MAK) $(ECHOGS_XE) $(PNGOBJ)pngwio.$(OBJ) $(PZGEN)crc32.dev
	$(SETMOD) $(PNGGEN)lpg90 $(PNGOBJ)pngwio.$(OBJ) -include $(PZGEN)crc32.dev

$(PNGGEN)lpg95.dev : $(LIBPNG_MAK) $(ECHOGS_XE) $(PNGOBJ)pngwio.$(OBJ) $(PZGEN)crc32.dev
	$(SETMOD) $(PNGGEN)lpg95 $(PNGOBJ)pngwio.$(OBJ) -include $(PZGEN)crc32.dev

$(PNGGEN)lpg96.dev : $(LIBPNG_MAK) $(ECHOGS_XE) $(PNGOBJ)pngwio.$(OBJ) $(PZGEN)crc32.dev
	$(SETMOD) $(PNGGEN)lpg96 $(PNGOBJ)pngwio.$(OBJ) -include $(PZGEN)crc32.dev

$(PNGGEN)lpg101.dev : $(LIBPNG_MAK) $(ECHOGS_XE) $(PNGOBJ)pngwio.$(OBJ) $(PZGEN)crc32.dev
	$(SETMOD) $(PNGGEN)lpg101 $(PNGOBJ)pngwio.$(OBJ) -include $(PZGEN)crc32.dev

$(PNGGEN)lpg102.dev : $(LIBPNG_MAK) $(ECHOGS_XE) $(PNGOBJ)pngwio.$(OBJ) $(PZGEN)crc32.dev
	$(SETMOD) $(PNGGEN)lpg102 $(PNGOBJ)pngwio.$(OBJ) -include $(PZGEN)crc32.dev

$(PNGGEN)lpg10001.dev : $(LIBPNG_MAK) $(ECHOGS_XE) $(PNGOBJ)pngwio.$(OBJ) $(PZGEN)crc32.dev
	$(SETMOD) $(PNGGEN)lpg10001 $(PNGOBJ)pngwio.$(OBJ) -include $(PZGEN)crc32.dev

$(PNGGEN)lpg10002.dev : $(LIBPNG_MAK) $(ECHOGS_XE) $(PNGOBJ)pngwio.$(OBJ) $(PZGEN)crc32.dev
	$(SETMOD) $(PNGGEN)lpg10002 $(PNGOBJ)pngwio.$(OBJ) -include $(PZGEN)crc32.dev

$(PNGGEN)lpg10003.dev : $(LIBPNG_MAK) $(ECHOGS_XE) $(PNGOBJ)pngwio.$(OBJ) $(PZGEN)crc32.dev
	$(SETMOD) $(PNGGEN)lpg10003 $(PNGOBJ)pngwio.$(OBJ) -include $(PZGEN)crc32.dev

$(PNGGEN)lpg10005.dev : $(LIBPNG_MAK) $(ECHOGS_XE) $(PNGOBJ)pngwio.$(OBJ) $(PZGEN)crc32.dev
	$(SETMOD) $(PNGGEN)lpg10005 $(PNGOBJ)pngwio.$(OBJ) -include $(PZGEN)crc32.dev
