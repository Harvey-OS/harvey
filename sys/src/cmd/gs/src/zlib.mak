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

# $Id: zlib.mak,v 1.1 2000/03/09 08:40:45 lpd Exp $
# makefile for zlib library code.
# Users of this makefile must define the following:
#	GSSRCDIR - the GS library source directory
#	ZSRCDIR - the source directory
#	ZGENDIR - the generated intermediate file directory
#	ZOBJDIR - the object directory
#	SHARE_ZLIB - 0 to compile zlib, 1 to share
#	ZLIB_NAME - if SHARE_ZLIB=1, the name of the shared library

# This partial makefile compiles the zlib library for use in Ghostscript.
# You can get the source code for this library from:
#   http://www.cdrom.com/infozip/zlib/
#   ftp://ftp.cdrom.com/pub/infozip/zlib/
#	zlib-#.#.#.tar.gz or zlib###.zip
# Please see Ghostscript's `Make.htm' file for instructions about how to
# unpack these archives.
#
# When each version of Ghostscript is released, we copy the associated
# version of the zlib library to
#	ftp://ftp.cs.wisc.edu/ghost/3rdparty/
# for more convenient access.
#
# This makefile is known to work with zlib versions through 1.1.3.
# Note that there are obscure bugs in zlib versions before 1.1.3 that
# may cause the FlateDecode filter to produce an occasional ioerror:
# we strongly recommend using version 1.1.3 or later.

ZSRC=$(ZSRCDIR)$(D)
ZGEN=$(ZGENDIR)$(D)
ZOBJ=$(ZOBJDIR)$(D)
ZO_=$(O_)$(ZOBJ)

# We need D_, _D_, and _D because the OpenVMS compiler uses different
# syntax from other compilers.
# ZI_ and ZF_ are defined in gs.mak.
ZCCFLAGS=$(I_)$(ZI_)$(_I) $(ZF_) $(D_)verbose$(_D_)-1$(_D)
ZCC=$(CC_) $(ZCCFLAGS)

# Define the name of this makefile.
ZLIB_MAK=$(GLSRC)zlib.mak

z.clean : z.config-clean z.clean-not-config-clean

### WRONG.  MUST DELETE OBJ AND GEN FILES SELECTIVELY.
z.clean-not-config-clean :
	$(RM_) $(ZOBJ)*.$(OBJ)

z.config-clean :
	$(RMN_) $(ZGEN)zlib*.dev $(ZGEN)crc32*.dev

ZDEP=$(AK)

# Code common to compression and decompression.

zlibc_=$(ZOBJ)zutil.$(OBJ)
$(ZGEN)zlibc.dev : $(ZLIB_MAK) $(ECHOGS_XE) $(zlibc_)
	$(SETMOD) $(ZGEN)zlibc $(zlibc_)

$(ZOBJ)zutil.$(OBJ) : $(ZSRC)zutil.c $(ZDEP)
	$(ZCC) $(ZO_)zutil.$(OBJ) $(C_) $(ZSRC)zutil.c

# Encoding (compression) code.

$(ZGEN)zlibe.dev : $(TOP_MAKEFILES) $(ZGEN)zlibe_$(SHARE_ZLIB).dev
	$(CP_) $(ZGEN)zlibe_$(SHARE_ZLIB).dev $(ZGEN)zlibe.dev

$(ZGEN)zlibe_1.dev : $(TOP_MAKEFILES) $(ZLIB_MAK) $(ECHOGS_XE)
	$(SETMOD) $(ZGEN)zlibe_1 -lib $(ZLIB_NAME)

zlibe_=$(ZOBJ)adler32.$(OBJ) $(ZOBJ)deflate.$(OBJ) $(ZOBJ)trees.$(OBJ)
$(ZGEN)zlibe_0.dev : $(ZLIB_MAK) $(ECHOGS_XE) $(ZGEN)zlibc.dev $(zlibe_)
	$(SETMOD) $(ZGEN)zlibe_0 $(zlibe_)
	$(ADDMOD) $(ZGEN)zlibe_0 -include $(ZGEN)zlibc.dev

$(ZOBJ)adler32.$(OBJ) : $(ZSRC)adler32.c $(ZDEP)
	$(ZCC) $(ZO_)adler32.$(OBJ) $(C_) $(ZSRC)adler32.c

$(ZOBJ)deflate.$(OBJ) : $(ZSRC)deflate.c $(ZDEP)
	$(ZCC) $(ZO_)deflate.$(OBJ) $(C_) $(ZSRC)deflate.c

$(ZOBJ)trees.$(OBJ) : $(ZSRC)trees.c $(ZDEP)
	$(ZCC) $(ZO_)trees.$(OBJ) $(C_) $(ZSRC)trees.c

# The zlib filters per se don't need crc32, but libpng versions starting
# with 0.90 do.

$(ZGEN)crc32.dev : $(TOP_MAKEFILES) $(ZGEN)crc32_$(SHARE_ZLIB).dev
	$(CP_) $(ZGEN)crc32_$(SHARE_ZLIB).dev $(ZGEN)crc32.dev

$(ZGEN)crc32_1.dev : $(TOP_MAKEFILES) $(ZLIB_MAK) $(ECHOGS_XE)
	$(SETMOD) $(ZGEN)crc32_1 -lib $(ZLIB_NAME)

$(ZGEN)crc32_0.dev : $(ZLIB_MAK) $(ECHOGS_XE) $(ZOBJ)crc32.$(OBJ)
	$(SETMOD) $(ZGEN)crc32_0 $(ZOBJ)crc32.$(OBJ)

# We have to compile crc32 without warnings, because it defines 32-bit
# constants that produces gcc warnings with -Wtraditional.
$(ZOBJ)crc32.$(OBJ) : $(ZSRC)crc32.c $(ZDEP)
	$(CC_NO_WARN) $(ZCCFLAGS) $(ZO_)crc32.$(OBJ) $(C_) $(ZSRC)crc32.c

# Decoding (decompression) code.

$(ZGEN)zlibd.dev : $(TOP_MAKEFILES) $(ZGEN)zlibd_$(SHARE_ZLIB).dev
	$(CP_) $(ZGEN)zlibd_$(SHARE_ZLIB).dev $(ZGEN)zlibd.dev

$(ZGEN)zlibd_1.dev : $(TOP_MAKEFILES) $(ZLIB_MAK) $(ECHOGS_XE)
	$(SETMOD) $(ZGEN)zlibd_1 -lib $(ZLIB_NAME)

zlibd1_=$(ZOBJ)infblock.$(OBJ) $(ZOBJ)infcodes.$(OBJ) $(ZOBJ)inffast.$(OBJ)
zlibd2_=$(ZOBJ)inflate.$(OBJ) $(ZOBJ)inftrees.$(OBJ) $(ZOBJ)infutil.$(OBJ)
zlibd_ = $(zlibd1_) $(zlibd2_)
$(ZGEN)zlibd_0.dev : $(ZLIB_MAK) $(ECHOGS_XE) $(ZGEN)zlibc.dev $(zlibd_)
	$(SETMOD) $(ZGEN)zlibd_0 $(zlibd1_)
	$(ADDMOD) $(ZGEN)zlibd_0 -obj $(zlibd2_)
	$(ADDMOD) $(ZGEN)zlibd_0 -include $(ZGEN)zlibc.dev

$(ZOBJ)infblock.$(OBJ) : $(ZSRC)infblock.c $(ZDEP)
	$(ZCC) $(ZO_)infblock.$(OBJ) $(C_) $(ZSRC)infblock.c

$(ZOBJ)infcodes.$(OBJ) : $(ZSRC)infcodes.c $(ZDEP)
	$(ZCC) $(ZO_)infcodes.$(OBJ) $(C_) $(ZSRC)infcodes.c

$(ZOBJ)inffast.$(OBJ) : $(ZSRC)inffast.c $(ZDEP)
	$(ZCC) $(ZO_)inffast.$(OBJ) $(C_) $(ZSRC)inffast.c

$(ZOBJ)inflate.$(OBJ) : $(ZSRC)inflate.c $(ZDEP)
	$(ZCC) $(ZO_)inflate.$(OBJ) $(C_) $(ZSRC)inflate.c

$(ZOBJ)inftrees.$(OBJ) : $(ZSRC)inftrees.c $(ZDEP)
	$(ZCC) $(ZO_)inftrees.$(OBJ) $(C_) $(ZSRC)inftrees.c

$(ZOBJ)infutil.$(OBJ) : $(ZSRC)infutil.c $(ZDEP)
	$(ZCC) $(ZO_)infutil.$(OBJ) $(C_) $(ZSRC)infutil.c
