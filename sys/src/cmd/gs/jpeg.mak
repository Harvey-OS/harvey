#    Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
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

# makefile for Independent JPEG Group code.

# NOTE: this makefile is only known to work with version 5 of the IJG code.
# You can get the current version by Internet anonymous FTP from
# ftp.uu.net:/graphics/jpeg.  As of September 26, 1994, version 5 is
# the current version, and the IJG files are named:
#	jpegsrc.v5.tar.gz		Standard distribution
#	jpeg-5.zip			DOS-style archive
# You may find more recent files on the FTP server.  If the version number,
# and hence the subdirectory name, has changed, you will probably want to
# change the definition of JSRCDIR (in the platform-specific makefile,
# not here) to reflect this, since that way you can use the IJG archive
# without change.

# JSRCDIR is defined in the platform-specific makefile, not here.
#JSRCDIR=jpeg-5

JSRC=$(JSRCDIR)$(D)
RMJ=rm -f
CCCJ=$(CCC) -I. -I$(JSRCDIR)

# We keep all of the IJG code in a separate directory so as not to
# inadvertently mix it up with Aladdin Enterprises' own code.
# However, we need our own version of jconfig.h, and our own "wrapper" for
# jmorecfg.h.  We also need a substitute for jerror.c, in order to
# keep the error strings out of the automatic data segment in
# 16-bit environments.

jconfig_h=jconfig.h $(std_h)
jerror_h=jerror.h
jmorecfg_h=jmorecfg.h jmcorig.h

jconfig.h: gsjconf.h
	cp gsjconf.h jconfig.h

jmorecfg.h: gsjmorec.h
	cp gsjmorec.h jmorecfg.h

jmcorig.h: $(JSRC)jmorecfg.h
	cp $(JSRC)jmorecfg.h jmcorig.h

# To ensure that the compiler finds our versions of jconfig.h and jmorecfg.h,
# regardless of the compiler's search rule, we must copy up all .c files,
# and all .h files that include either of these files, directly or
# indirectly.  The only such .h files currently are jinclude.h and jpeglib.h.
# Also, to avoid including the JSRCDIR directory name in our source files,
# we must also copy up any other .h files that our own code references.
# Currently, the only such .h files are jerror.h and jversion.h.

JHCOPY=jinclude.h jpeglib.h jerror.h jversion.h

jinclude.h: $(JSRC)jinclude.h
	cp $(JSRC)jinclude.h jinclude.h

jpeglib.h: $(JSRC)jpeglib.h
	cp $(JSRC)jpeglib.h jpeglib.h

jerror.h: $(JSRC)jerror.h
	cp $(JSRC)jerror.h jerror.h

jversion.h: $(JSRC)jversion.h
	cp $(JSRC)jversion.h jversion.h

# In order to avoid having to keep the dependency lists for the IJG code
# accurate, we simply make all of them depend on the only files that
# we are ever going to change, and on all the .h files that must be copied up.
# This is too conservative, but only hurts us if we are changing our own
# j*.h files, which happens only rarely during development.

JDEP=$(AK) $(jconfig_h) $(jerror_h) $(jmorecfg_h) $(JHCOPY)

# Code common to compression and decompression.

jpegc_=jcomapi.$(OBJ) jutils.$(OBJ) sjpegerr.$(OBJ) jmemmgr.$(OBJ)
jpegc.dev: jpeg.mak $(jpegc_)
	$(SETMOD) jpegc $(jpegc_)

jcomapi.$(OBJ): $(JSRC)jcomapi.c $(JDEP)
	cp $(JSRC)jcomapi.c .
	$(CCCJ) jcomapi.c
	$(RMJ) jcomapi.c

jutils.$(OBJ): $(JSRC)jutils.c $(JDEP)
	cp $(JSRC)jutils.c .
	$(CCCJ) jutils.c
	$(RMJ) jutils.c

# Note that sjpegerr replaces jerror.
sjpegerr.$(OBJ): sjpegerr.c $(JDEP)
	$(CCCF) sjpegerr.c

jmemmgr.$(OBJ): $(JSRC)jmemmgr.c $(JDEP)
	cp $(JSRC)jmemmgr.c .
	$(CCCJ) jmemmgr.c
	$(RMJ) jmemmgr.c

# Encoding (compression) code.

jpege_1=jcapi.$(OBJ) jccoefct.$(OBJ) jccolor.$(OBJ) jcdctmgr.$(OBJ) 
jpege_2=jchuff.$(OBJ) jcmainct.$(OBJ) jcmarker.$(OBJ) jcmaster.$(OBJ)
jpege_3=jcparam.$(OBJ) jcprepct.$(OBJ) jcsample.$(OBJ) jfdctint.$(OBJ)
jpege.dev: jpeg.mak jpegc.dev $(jpege_1) $(jpege_2) $(jpege_3)
	$(SETMOD) jpege
	$(ADDMOD) jpege -include jpegc
	$(ADDMOD) jpege -obj $(jpege_1)
	$(ADDMOD) jpege -obj $(jpege_2)
	$(ADDMOD) jpege -obj $(jpege_3)

jcapi.$(OBJ): $(JSRC)jcapi.c $(JDEP)
	cp $(JSRC)jcapi.c .
	$(CCCJ) jcapi.c
	$(RMJ) jcapi.c

jccoefct.$(OBJ): $(JSRC)jccoefct.c $(JDEP)
	cp $(JSRC)jccoefct.c .
	$(CCCJ) jccoefct.c
	$(RMJ) jccoefct.c

jccolor.$(OBJ): $(JSRC)jccolor.c $(JDEP)
	cp $(JSRC)jccolor.c .
	$(CCCJ) jccolor.c
	$(RMJ) jccolor.c

jcdctmgr.$(OBJ): $(JSRC)jcdctmgr.c $(JDEP)
	cp $(JSRC)jcdctmgr.c .
	$(CCCJ) jcdctmgr.c
	$(RMJ) jcdctmgr.c

jchuff.$(OBJ): $(JSRC)jchuff.c $(JDEP)
	cp $(JSRC)jchuff.c .
	$(CCCJ) jchuff.c
	$(RMJ) jchuff.c

jcmainct.$(OBJ): $(JSRC)jcmainct.c $(JDEP)
	cp $(JSRC)jcmainct.c .
	$(CCCJ) jcmainct.c
	$(RMJ) jcmainct.c

jcmarker.$(OBJ): $(JSRC)jcmarker.c $(JDEP)
	cp $(JSRC)jcmarker.c .
	$(CCCJ) jcmarker.c
	$(RMJ) jcmarker.c

jcmaster.$(OBJ): $(JSRC)jcmaster.c $(JDEP)
	cp $(JSRC)jcmaster.c .
	$(CCCJ) jcmaster.c
	$(RMJ) jcmaster.c

jcparam.$(OBJ): $(JSRC)jcparam.c $(JDEP)
	cp $(JSRC)jcparam.c .
	$(CCCJ) jcparam.c
	$(RMJ) jcparam.c

jcprepct.$(OBJ): $(JSRC)jcprepct.c $(JDEP)
	cp $(JSRC)jcprepct.c .
	$(CCCJ) jcprepct.c
	$(RMJ) jcprepct.c

jcsample.$(OBJ): $(JSRC)jcsample.c $(JDEP)
	cp $(JSRC)jcsample.c .
	$(CCCJ) jcsample.c
	$(RMJ) jcsample.c

jfdctint.$(OBJ): $(JSRC)jfdctint.c $(JDEP)
	cp $(JSRC)jfdctint.c .
	$(CCCJ) jfdctint.c
	$(RMJ) jfdctint.c

# Decompression code

jpegd_1=jdapi.$(OBJ) jdcoefct.$(OBJ) jdcolor.$(OBJ)
jpegd_2=jddctmgr.$(OBJ) jdhuff.$(OBJ) jdmainct.$(OBJ) jdmarker.$(OBJ)
jpegd_3=jdmaster.$(OBJ) jdpostct.$(OBJ) jdsample.$(OBJ) jidctint.$(OBJ)
jpegd.dev: jpeg.mak jpegc.dev $(jpegd_1) $(jpegd_2) $(jpegd_3)
	$(SETMOD) jpegd
	$(ADDMOD) jpegd -include jpegc
	$(ADDMOD) jpegd -obj $(jpegd_1)
	$(ADDMOD) jpegd -obj $(jpegd_2)
	$(ADDMOD) jpegd -obj $(jpegd_3)

jdapi.$(OBJ): $(JSRC)jdapi.c $(JDEP)
	cp $(JSRC)jdapi.c .
	$(CCCJ) jdapi.c
	$(RMJ) jdapi.c

jdcoefct.$(OBJ): $(JSRC)jdcoefct.c $(JDEP)
	cp $(JSRC)jdcoefct.c .
	$(CCCJ) jdcoefct.c
	$(RMJ) jdcoefct.c

jdcolor.$(OBJ): $(JSRC)jdcolor.c $(JDEP)
	cp $(JSRC)jdcolor.c .
	$(CCCJ) jdcolor.c
	$(RMJ) jdcolor.c

jddctmgr.$(OBJ): $(JSRC)jddctmgr.c $(JDEP)
	cp $(JSRC)jddctmgr.c .
	$(CCCJ) jddctmgr.c
	$(RMJ) jddctmgr.c

jdhuff.$(OBJ): $(JSRC)jdhuff.c $(JDEP)
	cp $(JSRC)jdhuff.c .
	$(CCCJ) jdhuff.c
	$(RMJ) jdhuff.c

jdmainct.$(OBJ): $(JSRC)jdmainct.c $(JDEP)
	cp $(JSRC)jdmainct.c .
	$(CCCJ) jdmainct.c
	$(RMJ) jdmainct.c

jdmarker.$(OBJ): $(JSRC)jdmarker.c $(JDEP)
	cp $(JSRC)jdmarker.c .
	$(CCCJ) jdmarker.c
	$(RMJ) jdmarker.c

jdmaster.$(OBJ): $(JSRC)jdmaster.c $(JDEP)
	cp $(JSRC)jdmaster.c .
	$(CCCJ) jdmaster.c
	$(RMJ) jdmaster.c

jdpostct.$(OBJ): $(JSRC)jdpostct.c $(JDEP)
	cp $(JSRC)jdpostct.c .
	$(CCCJ) jdpostct.c
	$(RMJ) jdpostct.c

jdsample.$(OBJ): $(JSRC)jdsample.c $(JDEP)
	cp $(JSRC)jdsample.c .
	$(CCCJ) jdsample.c
	$(RMJ) jdsample.c

jidctint.$(OBJ): $(JSRC)jidctint.c $(JDEP)
	cp $(JSRC)jidctint.c .
	$(CCCJ) jidctint.c
	$(RMJ) jidctint.c
