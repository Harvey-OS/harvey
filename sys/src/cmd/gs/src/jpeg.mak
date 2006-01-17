#    Copyright (C) 1994, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: jpeg.mak,v 1.7 2002/06/16 09:11:47 lpd Exp $
# makefile for Independent JPEG Group library code.
# Users of this makefile must define the following:
#	GSSRCDIR - the GS library source directory
#	JSRCDIR - the source directory
#	JGENDIR - the generated intermediate file directory
#	JOBJDIR - the object directory
#	JVERSION - the library major version number (currently "6",
#	   "6b", "62" and "6a" are also supported)
#	SHARE_JPEG - 0 to compile the library, 1 to share
#	JPEG_NAME - if SHARE_JPEG = 1, the name of the shared library
#
# Note that if SHARE_JPEG = 1, you must still have the library header files
# available to compile Ghostscript: see doc/Make.htm for more information.

# NOTE: This makefile is only known to work with the following versions
# of the IJG library: 6, 6a, 6b.
# As of March 27, 1998, version 6b is the current version.
# The odds are good that other versions v6* will work.
#
# You can get the IJG library by Internet anonymous FTP from the following
# places:
#   Standard distribution (tar + gzip format, Unix end-of-line):
#	ftp://ftp.uu.net/graphics/jpeg/jpegsrc.v*.tar.gz
#   MS-DOS archive (PKZIP a.k.a. zip format, MS-DOS end-of-line):
#	ftp://ftp.simtel.net/pub/simtelnet/msdos/graphics/jpegsr*.zip
# Please see Ghostscript's `Make.htm' file for instructions about how to
# unpack these archives.
#
# When each version of Ghostscript is released, we copy the associated
# version of the IJG library to
#	ftp://ftp.cs.wisc.edu/ghost/3rdparty/
# for more convenient access.
#
# The platform-specific makefiles expect the jpeg source to be in a
# directory named 'jpeg' at the top level of the source tree, as per
# the instructions in Make.htm. If you'd prefer to use the versioned
# directory name of the native library source, you can override this
# by setting JSRCSDIR in the platform-specific makefile.
#
# NOTE: For some obscure reason (probably a bug in djtarx), if you are
# compiling on a DesqView/X system, you should use the zip version of the
# IJG library, not the tar.gz version.

JSRC=$(JSRCDIR)$(D)
JGEN=$(JGENDIR)$(D)
JOBJ=$(JOBJDIR)$(D)
JO_=$(O_)$(JOBJ)

# Define the name of this makefile.
JPEG_MAK=$(GLSRC)jpeg.mak

jpeg.clean : jpeg.config-clean jpeg.clean-not-config-clean

### WRONG.  MUST DELETE OBJ AND GEN FILES SELECTIVELY.
jpeg.clean-not-config-clean :
	$(RM_) $(JOBJ)*.$(OBJ)

jpeg.config-clean :
	$(RMN_) $(JGEN)jpeg*.dev

# JI_ and JF_ are defined in gs.mak.
# See below for why we need to include GLGENDIR here.
JCC=$(CC_) $(I_)$(GLGENDIR) $(II)$(JI_)$(_I) $(JF_)

# We need our own version of jconfig.h, and our own "wrapper" for
# jmorecfg.h.
# We also need to change D_MAX_BLOCKS_IN_MCU for Adobe compatibility;
# we do this in jmorecfg.h.

# Because this file is included after lib.mak, we can't use _h macros
# to express indirect dependencies; instead, we build the dependencies
# into the rules for copying the files.
# Note: jerror__h and jpeglib__h are defined in lib.mak.
jconfig__h=$(GLGEN)jconfig_.h $(GLGEN)arch.h
#jerror__h=$(GLSRC)jerror_.h
jmorecf__h=$(GLGEN)jmorecf_.h
#jpeglib__h=$(GLGEN)jpeglib_.h

# We use our own jconfig.h and jmorecfg.h iff we aren't sharing the library.
# The library itself may need copies of them.

jconfig_h=$(GLGEN)jconfig.h
jmorecfg_h=$(GLGEN)jmorecfg.h

$(GLGEN)jconfig_.h : $(GLGEN)jconfig$(SHARE_JPEG).h $(MAKEFILE)
	$(CP_) $(GLGEN)jconfig$(SHARE_JPEG).h $(GLGEN)jconfig_.h

$(GLGEN)jconfig0.h : $(ECHOGS_XE) $(GLSRC)gsjconf.h $(stdpre_h) $(MAKEFILE)
	$(EXP)$(ECHOGS_XE) -w $(GLGEN)jconfig0.h -+R $(GLSRC)stdpn.h -+R $(GLSRC)stdpre.h -+R $(GLSRC)gsjconf.h
	$(RM_) $(GLGEN)jconfig1.h

$(GLGEN)jconfig1.h : $(ECHOGS_XE) $(JPEG_MAK)
	$(EXP)$(ECHOGS_XE) -w $(GLGEN)jconfig1.h -x 23 include -x 203c jconfig.h -x 3e
	$(RMN_) $(GLGEN)jconfig0.h $(GLGEN)jconfig.h

$(GLGEN)jconfig.h : $(GLGEN)jconfig0.h
	$(CP_) $(GLGEN)jconfig0.h $(GLGEN)jconfig.h

$(GLGEN)jmorecf_.h : $(GLGEN)jmorecf$(SHARE_JPEG).h $(MAKEFILE)
	$(CP_) $(GLGEN)jmorecf$(SHARE_JPEG).h $(GLGEN)jmorecf_.h

$(GLGEN)jmorecf0.h : $(GLSRC)gsjmorec.h $(GLGEN)jmcorig.h
	$(CP_) $(GLSRC)gsjmorec.h $(GLGEN)jmorecf0.h
	$(RM_) $(GLGEN)jmorecf1.h

$(GLGEN)jmorecf1.h : $(ECHOGS_XE) $(JPEG_MAK)
	$(EXP)$(ECHOGS_XE) -w $(GLGEN)jmorecf1.h -x 23 include -x 203c jmorecfg.h -x 3e
	$(RMN_) $(GLGEN)jmorecf0.h $(GLGEN)jmorecfg.h

$(GLGEN)jmorecfg.h : $(GLGEN)jmorecf0.h
	$(CP_) $(GLGEN)jmorecf0.h $(GLGEN)jmorecfg.h

$(GLGEN)jmcorig.h : $(JSRC)jmorecfg.h
	$(CP_) $(JSRC)jmorecfg.h $(GLGEN)jmcorig.h

# Contrary to what some portability bigots assert as fact, C compilers are
# not consistent about where they start searching for #included files:
# some always start by looking in the same directory as the .c file being
# compiled, before using the search path specified with -I on the command
# line, while others do not do this.  For this reason, we must explicitly
# copy and then delete all the .c files, because they need to obtain our
# modified version of the header files.  We must similarly copy up all
# all .h files that include either of these files, directly or indirectly.
# (The only such .h files currently are jinclude.h and jpeglib.h.)
# And finally, we must include GLGENDIR in the -I list (see JCC= above).

JHCOPY=$(GLGEN)jinclude.h $(GLGEN)jpeglib.h

$(GLGEN)jinclude.h : $(JSRC)jinclude.h
	$(CP_) $(JSRC)jinclude.h $(GLGEN)jinclude.h

# jpeglib_.h doesn't really depend on jconfig.h or jmcorig.h,
# but we choose to put the dependencies here rather than in the
# definition of jpeglib__h.
$(GLGEN)jpeglib_.h : $(GLGEN)jpeglib$(SHARE_JPEG).h $(MAKEFILE)
	$(CP_) $(GLGEN)jpeglib$(SHARE_JPEG).h $(GLGEN)jpeglib_.h

$(GLGEN)jpeglib0.h : $(JSRC)jpeglib.h $(jconfig_h) $(jmorecfg_h)
	$(CP_) $(JSRC)jpeglib.h $(GLGEN)jpeglib0.h

$(GLGEN)jpeglib1.h : $(ECHOGS_XE) $(JPEG_MAK)
	$(EXP)$(ECHOGS_XE) -w $(GLGEN)jpeglib1.h -x 23 include -x 203c jpeglib.h -x 3e

# We also need jpeglib.h for #includes in the library itself.
$(GLGEN)jpeglib.h : $(JSRC)jpeglib.h
	$(CP_) $(JSRC)jpeglib.h $(GLGEN)jpeglib.h

# In order to avoid having to keep the dependency lists for the IJG code
# accurate, we simply make all of them depend on the only files that
# we are ever going to change, and on all the .h files that must be copied up.
# This is too conservative, but only hurts us if we are changing our own
# j*.h files, which happens only rarely during development.

JDEP=$(AK) $(jconfig_h) $(jmorecfg_h) $(JHCOPY)

# Code common to compression and decompression.

jpegc0_=$(JOBJ)jcomapi.$(OBJ) $(JOBJ)jutils.$(OBJ) $(JOBJ)jmemmgr.$(OBJ) $(JOBJ)jerror.$(OBJ)
$(JGEN)jpegc0.dev : $(JPEG_MAK) $(ECHOGS_XE) $(jpegc0_)
	$(SETMOD) $(JGEN)jpegc0 $(jpegc0_)

$(JOBJ)jcomapi.$(OBJ) : $(JSRC)jcomapi.c $(JDEP)
	$(CP_) $(JSRC)jcomapi.c $(GLGEN)jcomapi.c
	$(JCC) $(JO_)jcomapi.$(OBJ) $(C_) $(GLGEN)jcomapi.c
	$(RM_) $(GLGEN)jcomapi.c

$(JOBJ)jutils.$(OBJ) : $(JSRC)jutils.c $(JDEP)
	$(CP_) $(JSRC)jutils.c $(GLGEN)jutils.c
	$(JCC) $(JO_)jutils.$(OBJ) $(C_) $(GLGEN)jutils.c
	$(RM_) $(GLGEN)jutils.c

$(JOBJ)jmemmgr.$(OBJ) : $(JSRC)jmemmgr.c $(JDEP)
	$(CP_) $(JSRC)jmemmgr.c $(GLGEN)jmemmgr.c
	$(JCC) $(JO_)jmemmgr.$(OBJ) $(C_) $(GLGEN)jmemmgr.c
	$(RM_) $(GLGEN)jmemmgr.c

$(JOBJ)jerror.$(OBJ) : $(JSRC)jerror.c $(JDEP)
	$(CP_) $(JSRC)jerror.c $(GLGEN)jerror.c
	$(JCC) $(JO_)jerror.$(OBJ) $(C_) $(GLGEN)jerror.c
	$(RM_) $(GLGEN)jerror.c

# Encoding (compression) code.

$(JGEN)jpege.dev : $(TOP_MAKEFILES) $(JPEG_MAK) $(JGEN)jpege_$(SHARE_JPEG).dev
	$(CP_) $(JGEN)jpege_$(SHARE_JPEG).dev $(JGEN)jpege.dev

$(JGEN)jpege_1.dev : $(TOP_MAKEFILES) $(JPEG_MAK) $(ECHOGS_XE)
	$(SETMOD) $(JGEN)jpege_1 -lib $(JPEG_NAME)

# we actually ignore the setting of JVERSION here, which could be used
# to substitute for jpeg[ed]6.dev to handle differences. However, the
# build requirements for all supported versions are identical, and
# new versions are unlikely to be different. We therefore treat all
# subversions the same.
$(JGEN)jpege_0.dev : $(TOP_MAKEFILES) $(JPEG_MAK) $(JGEN)jpege6.dev
	$(CP_) $(JGEN)jpege6.dev $(JGEN)jpege_0.dev

jpege6=$(JOBJ)jcapimin.$(OBJ) $(JOBJ)jcapistd.$(OBJ) $(JOBJ)jcinit.$(OBJ)

jpege_1=$(JOBJ)jccoefct.$(OBJ) $(JOBJ)jccolor.$(OBJ) $(JOBJ)jcdctmgr.$(OBJ)
jpege_2=$(JOBJ)jchuff.$(OBJ) $(JOBJ)jcmainct.$(OBJ) $(JOBJ)jcmarker.$(OBJ) $(JOBJ)jcmaster.$(OBJ)
jpege_3=$(JOBJ)jcparam.$(OBJ) $(JOBJ)jcprepct.$(OBJ) $(JOBJ)jcsample.$(OBJ) $(JOBJ)jfdctint.$(OBJ)

$(JGEN)jpege6.dev : $(JPEG_MAK) $(ECHOGS_XE) $(JGEN)jpegc0.dev $(jpege6) $(jpege_1) $(jpege_2) $(jpege_3)
	$(SETMOD) $(JGEN)jpege6 $(jpege6)
	$(ADDMOD) $(JGEN)jpege6 -include $(JGEN)jpegc0.dev
	$(ADDMOD) $(JGEN)jpege6 -obj $(jpege_1)
	$(ADDMOD) $(JGEN)jpege6 -obj $(jpege_2)
	$(ADDMOD) $(JGEN)jpege6 -obj $(jpege_3)

$(JOBJ)jcapimin.$(OBJ) : $(JSRC)jcapimin.c $(JDEP)
	$(CP_) $(JSRC)jcapimin.c $(GLGEN)jcapimin.c
	$(JCC) $(JO_)jcapimin.$(OBJ) $(C_) $(GLGEN)jcapimin.c
	$(RM_) $(GLGEN)jcapimin.c

$(JOBJ)jcapistd.$(OBJ) : $(JSRC)jcapistd.c $(JDEP)
	$(CP_) $(JSRC)jcapistd.c $(GLGEN)jcapistd.c
	$(JCC) $(JO_)jcapistd.$(OBJ) $(C_) $(GLGEN)jcapistd.c
	$(RM_) $(GLGEN)jcapistd.c

$(JOBJ)jcinit.$(OBJ) : $(JSRC)jcinit.c $(JDEP)
	$(CP_) $(JSRC)jcinit.c $(GLGEN)jcinit.c
	$(JCC) $(JO_)jcinit.$(OBJ) $(C_) $(GLGEN)jcinit.c
	$(RM_) $(GLGEN)jcinit.c

$(JOBJ)jccoefct.$(OBJ) : $(JSRC)jccoefct.c $(JDEP)
	$(CP_) $(JSRC)jccoefct.c $(GLGEN)jccoefct.c
	$(JCC) $(JO_)jccoefct.$(OBJ) $(C_) $(GLGEN)jccoefct.c
	$(RM_) $(GLGEN)jccoefct.c

$(JOBJ)jccolor.$(OBJ) : $(JSRC)jccolor.c $(JDEP)
	$(CP_) $(JSRC)jccolor.c $(GLGEN)jccolor.c
	$(JCC) $(JO_)jccolor.$(OBJ) $(C_) $(GLGEN)jccolor.c
	$(RM_) $(GLGEN)jccolor.c

$(JOBJ)jcdctmgr.$(OBJ) : $(JSRC)jcdctmgr.c $(JDEP)
	$(CP_) $(JSRC)jcdctmgr.c $(GLGEN)jcdctmgr.c
	$(JCC) $(JO_)jcdctmgr.$(OBJ) $(C_) $(GLGEN)jcdctmgr.c
	$(RM_) $(GLGEN)jcdctmgr.c

$(JOBJ)jchuff.$(OBJ) : $(JSRC)jchuff.c $(JDEP)
	$(CP_) $(JSRC)jchuff.c $(GLGEN)jchuff.c
	$(JCC) $(JO_)jchuff.$(OBJ) $(C_) $(GLGEN)jchuff.c
	$(RM_) $(GLGEN)jchuff.c

$(JOBJ)jcmainct.$(OBJ) : $(JSRC)jcmainct.c $(JDEP)
	$(CP_) $(JSRC)jcmainct.c $(GLGEN)jcmainct.c
	$(JCC) $(JO_)jcmainct.$(OBJ) $(C_) $(GLGEN)jcmainct.c
	$(RM_) $(GLGEN)jcmainct.c

$(JOBJ)jcmarker.$(OBJ) : $(JSRC)jcmarker.c $(JDEP)
	$(CP_) $(JSRC)jcmarker.c $(GLGEN)jcmarker.c
	$(JCC) $(JO_)jcmarker.$(OBJ) $(C_) $(GLGEN)jcmarker.c
	$(RM_) $(GLGEN)jcmarker.c

$(JOBJ)jcmaster.$(OBJ) : $(JSRC)jcmaster.c $(JDEP)
	$(CP_) $(JSRC)jcmaster.c $(GLGEN)jcmaster.c
	$(JCC) $(JO_)jcmaster.$(OBJ) $(C_) $(GLGEN)jcmaster.c
	$(RM_) $(GLGEN)jcmaster.c

$(JOBJ)jcparam.$(OBJ) : $(JSRC)jcparam.c $(JDEP)
	$(CP_) $(JSRC)jcparam.c $(GLGEN)jcparam.c
	$(JCC) $(JO_)jcparam.$(OBJ) $(C_) $(GLGEN)jcparam.c
	$(RM_) $(GLGEN)jcparam.c

$(JOBJ)jcprepct.$(OBJ) : $(JSRC)jcprepct.c $(JDEP)
	$(CP_) $(JSRC)jcprepct.c $(GLGEN)jcprepct.c
	$(JCC) $(JO_)jcprepct.$(OBJ) $(C_) $(GLGEN)jcprepct.c
	$(RM_) $(GLGEN)jcprepct.c

$(JOBJ)jcsample.$(OBJ) : $(JSRC)jcsample.c $(JDEP)
	$(CP_) $(JSRC)jcsample.c $(GLGEN)jcsample.c
	$(JCC) $(JO_)jcsample.$(OBJ) $(C_) $(GLGEN)jcsample.c
	$(RM_) $(GLGEN)jcsample.c

$(JOBJ)jfdctint.$(OBJ) : $(JSRC)jfdctint.c $(JDEP)
	$(CP_) $(JSRC)jfdctint.c $(GLGEN)jfdctint.c
	$(JCC) $(JO_)jfdctint.$(OBJ) $(C_) $(GLGEN)jfdctint.c
	$(RM_) $(GLGEN)jfdctint.c

# Decompression code

$(JGEN)jpegd.dev : $(TOP_MAKEFILES) $(JPEG_MAK) $(JGEN)jpegd_$(SHARE_JPEG).dev
	$(CP_) $(JGEN)jpegd_$(SHARE_JPEG).dev $(JGEN)jpegd.dev

$(JGEN)jpegd_1.dev : $(TOP_MAKEFILES) $(JPEG_MAK) $(ECHOGS_XE)
	$(SETMOD) $(JGEN)jpegd_1 -lib $(JPEG_NAME)

$(JGEN)jpegd_0.dev : $(TOP_MAKEFILES) $(JPEG_MAK) $(JGEN)jpegd6.dev
	$(CP_) $(JGEN)jpegd6.dev $(JGEN)jpegd_0.dev

jpegd6=$(JOBJ)jdapimin.$(OBJ) $(JOBJ)jdapistd.$(OBJ) $(JOBJ)jdinput.$(OBJ) $(JOBJ)jdphuff.$(OBJ)

jpegd_1=$(JOBJ)jdcoefct.$(OBJ) $(JOBJ)jdcolor.$(OBJ)
jpegd_2=$(JOBJ)jddctmgr.$(OBJ) $(JOBJ)jdhuff.$(OBJ) $(JOBJ)jdmainct.$(OBJ) $(JOBJ)jdmarker.$(OBJ)
jpegd_3=$(JOBJ)jdmaster.$(OBJ) $(JOBJ)jdpostct.$(OBJ) $(JOBJ)jdsample.$(OBJ) $(JOBJ)jidctint.$(OBJ)

$(JGEN)jpegd6.dev : $(JPEG_MAK) $(ECHOGS_XE) $(JGEN)jpegc0.dev $(jpegd6) $(jpegd_1) $(jpegd_2) $(jpegd_3)
	$(SETMOD) $(JGEN)jpegd6 $(jpegd6)
	$(ADDMOD) $(JGEN)jpegd6 -include $(JGEN)jpegc0.dev
	$(ADDMOD) $(JGEN)jpegd6 -obj $(jpegd_1)
	$(ADDMOD) $(JGEN)jpegd6 -obj $(jpegd_2)
	$(ADDMOD) $(JGEN)jpegd6 -obj $(jpegd_3)

$(JOBJ)jdapimin.$(OBJ) : $(JSRC)jdapimin.c $(JDEP)
	$(CP_) $(JSRC)jdapimin.c $(GLGEN)jdapimin.c
	$(JCC) $(JO_)jdapimin.$(OBJ) $(C_) $(GLGEN)jdapimin.c
	$(RM_) $(GLGEN)jdapimin.c

$(JOBJ)jdapistd.$(OBJ) : $(JSRC)jdapistd.c $(JDEP)
	$(CP_) $(JSRC)jdapistd.c $(GLGEN)jdapistd.c
	$(JCC) $(JO_)jdapistd.$(OBJ) $(C_) $(GLGEN)jdapistd.c
	$(RM_) $(GLGEN)jdapistd.c

$(JOBJ)jdcoefct.$(OBJ) : $(JSRC)jdcoefct.c $(JDEP)
	$(CP_) $(JSRC)jdcoefct.c $(GLGEN)jdcoefct.c
	$(JCC) $(JO_)jdcoefct.$(OBJ) $(C_) $(GLGEN)jdcoefct.c
	$(RM_) $(GLGEN)jdcoefct.c

$(JOBJ)jdcolor.$(OBJ) : $(JSRC)jdcolor.c $(JDEP)
	$(CP_) $(JSRC)jdcolor.c $(GLGEN)jdcolor.c
	$(JCC) $(JO_)jdcolor.$(OBJ) $(C_) $(GLGEN)jdcolor.c
	$(RM_) $(GLGEN)jdcolor.c

$(JOBJ)jddctmgr.$(OBJ) : $(JSRC)jddctmgr.c $(JDEP)
	$(CP_) $(JSRC)jddctmgr.c $(GLGEN)jddctmgr.c
	$(JCC) $(JO_)jddctmgr.$(OBJ) $(C_) $(GLGEN)jddctmgr.c
	$(RM_) $(GLGEN)jddctmgr.c

$(JOBJ)jdhuff.$(OBJ) : $(JSRC)jdhuff.c $(JDEP)
	$(CP_) $(JSRC)jdhuff.c $(GLGEN)jdhuff.c
	$(JCC) $(JO_)jdhuff.$(OBJ) $(C_) $(GLGEN)jdhuff.c
	$(RM_) $(GLGEN)jdhuff.c

$(JOBJ)jdinput.$(OBJ) : $(JSRC)jdinput.c $(JDEP)
	$(CP_) $(JSRC)jdinput.c $(GLGEN)jdinput.c
	$(JCC) $(JO_)jdinput.$(OBJ) $(C_) $(GLGEN)jdinput.c
	$(RM_) $(GLGEN)jdinput.c

$(JOBJ)jdmainct.$(OBJ) : $(JSRC)jdmainct.c $(JDEP)
	$(CP_) $(JSRC)jdmainct.c $(GLGEN)jdmainct.c
	$(JCC) $(JO_)jdmainct.$(OBJ) $(C_) $(GLGEN)jdmainct.c
	$(RM_) $(GLGEN)jdmainct.c

$(JOBJ)jdmarker.$(OBJ) : $(JSRC)jdmarker.c $(JDEP)
	$(CP_) $(JSRC)jdmarker.c $(GLGEN)jdmarker.c
	$(JCC) $(JO_)jdmarker.$(OBJ) $(C_) $(GLGEN)jdmarker.c
	$(RM_) $(GLGEN)jdmarker.c

$(JOBJ)jdmaster.$(OBJ) : $(JSRC)jdmaster.c $(JDEP)
	$(CP_) $(JSRC)jdmaster.c $(GLGEN)jdmaster.c
	$(JCC) $(JO_)jdmaster.$(OBJ) $(C_) $(GLGEN)jdmaster.c
	$(RM_) $(GLGEN)jdmaster.c

$(JOBJ)jdphuff.$(OBJ) : $(JSRC)jdphuff.c $(JDEP)
	$(CP_) $(JSRC)jdphuff.c $(GLGEN)jdphuff.c
	$(JCC) $(JO_)jdphuff.$(OBJ) $(C_) $(GLGEN)jdphuff.c
	$(RM_) $(GLGEN)jdphuff.c

$(JOBJ)jdpostct.$(OBJ) : $(JSRC)jdpostct.c $(JDEP)
	$(CP_) $(JSRC)jdpostct.c $(GLGEN)jdpostct.c
	$(JCC) $(JO_)jdpostct.$(OBJ) $(C_) $(GLGEN)jdpostct.c
	$(RM_) $(GLGEN)jdpostct.c

$(JOBJ)jdsample.$(OBJ) : $(JSRC)jdsample.c $(JDEP)
	$(CP_) $(JSRC)jdsample.c $(GLGEN)jdsample.c
	$(JCC) $(JO_)jdsample.$(OBJ) $(C_) $(GLGEN)jdsample.c
	$(RM_) $(GLGEN)jdsample.c

$(JOBJ)jidctint.$(OBJ) : $(JSRC)jidctint.c $(JDEP)
	$(CP_) $(JSRC)jidctint.c $(GLGEN)jidctint.c
	$(JCC) $(JO_)jidctint.$(OBJ) $(C_) $(GLGEN)jidctint.c
	$(RM_) $(GLGEN)jidctint.c
