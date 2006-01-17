# Copyright (C) 2004-2005 artofcode LLC.  All rights reserved.
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

# $Id: jasper.mak,v 1.2 2005/06/22 15:24:37 giles Exp $

# makefile for jasper JPEG 2000 support library
# Users of this makefile must define the following:
#	SHARE_JASPER - whether to compile in or link to the library
#	JASPERSRCDIR - the top-level library source directory
#
# gs.mak and friends define the following:
#	JASPEROBJDIR - the output obj directory
#	JASPERGENDIR - generated (.dev) file directory
#	JASI_ and JASCF_ - include dir and cflags
# and the usual gs portability stuff.

# This partial makefile compiles a portion of the jasper library for use in
# Ghostscript. You're better off just linking to the library's native build
# but this supports the library on all our platforms.

# Define the name of this makefile
JASPER_MAK=$(GLSRC)jasper.mak

JASSRC=$(JASPERSRCDIR)$(D)src$(D)libjasper$(D)
JASGEN=$(JASPERGENDIR)$(D)
JASOBJ=$(JASPEROBJDIR)$(D)

# our required files from jasper 1.701.x

libjasper_OBJS_base=\
	$(JASOBJ)jas_cm.$(OBJ) \
	$(JASOBJ)jas_debug.$(OBJ) \
	$(JASOBJ)jas_getopt.$(OBJ) \
	$(JASOBJ)jas_image.$(OBJ) \
	$(JASOBJ)jas_icc.$(OBJ) \
	$(JASOBJ)jas_iccdata.$(OBJ) \
	$(JASOBJ)jas_init.$(OBJ) \
	$(JASOBJ)jas_malloc.$(OBJ) \
	$(JASOBJ)jas_seq.$(OBJ) \
	$(JASOBJ)jas_stream.$(OBJ) \
	$(JASOBJ)jas_string.$(OBJ) \
	$(JASOBJ)jas_tvp.$(OBJ) \
	$(JASOBJ)jas_version.$(OBJ)

libjasper_OBJS_jpc=\
	$(JASOBJ)jpc_bs.$(OBJ) \
	$(JASOBJ)jpc_cs.$(OBJ) \
	$(JASOBJ)jpc_dec.$(OBJ) \
	$(JASOBJ)jpc_enc.$(OBJ) \
	$(JASOBJ)jpc_math.$(OBJ) \
	$(JASOBJ)jpc_mct.$(OBJ) \
	$(JASOBJ)jpc_mqcod.$(OBJ) \
	$(JASOBJ)jpc_mqdec.$(OBJ) \
	$(JASOBJ)jpc_mqenc.$(OBJ) \
	$(JASOBJ)jpc_qmfb.$(OBJ) \
	$(JASOBJ)jpc_tagtree.$(OBJ) \
	$(JASOBJ)jpc_t1cod.$(OBJ) \
	$(JASOBJ)jpc_t1dec.$(OBJ) \
	$(JASOBJ)jpc_t1enc.$(OBJ) \
	$(JASOBJ)jpc_tsfb.$(OBJ) \
	$(JASOBJ)jpc_t2cod.$(OBJ) \
	$(JASOBJ)jpc_t2dec.$(OBJ) \
	$(JASOBJ)jpc_t2enc.$(OBJ) \
	$(JASOBJ)jpc_util.$(OBJ)

libjasper_OBJS_jp2=\
	$(JASOBJ)jp2_cod.$(OBJ) \
	$(JASOBJ)jp2_dec.$(OBJ) \
	$(JASOBJ)jp2_enc.$(OBJ)

libjasper_OBJS=$(libjasper_OBJS_base) $(libjasper_OBJS_jpc) $(libjasper_OBJS_jp2)

libjasper_HDRS_base=\
	$(JASSRC)include$(D)jasper$(D)jasper.h \
	$(JASSRC)include$(D)jasper$(D)jas_config.h \
	$(JASSRC)include$(D)jasper$(D)jas_config_win32.h \
	$(JASSRC)include$(D)jasper$(D)jas_cm.h \
	$(JASSRC)include$(D)jasper$(D)jas_fix.h \
	$(JASSRC)include$(D)jasper$(D)jas_debug.h \
	$(JASSRC)include$(D)jasper$(D)jas_getopt.h \
	$(JASSRC)include$(D)jasper$(D)jas_icc.h \
	$(JASSRC)include$(D)jasper$(D)jas_image.h \
	$(JASSRC)include$(D)jasper$(D)jas_init.h \
	$(JASSRC)include$(D)jasper$(D)jas_malloc.h \
	$(JASSRC)include$(D)jasper$(D)jas_math.h \
	$(JASSRC)include$(D)jasper$(D)jas_seq.h \
	$(JASSRC)include$(D)jasper$(D)jas_stream.h \
	$(JASSRC)include$(D)jasper$(D)jas_string.h \
	$(JASSRC)include$(D)jasper$(D)jas_tvp.h \
	$(JASSRC)include$(D)jasper$(D)jas_types.h \
	$(JASSRC)include$(D)jasper$(D)jas_version.h

libjasper_HDRS_jpc=\
	$(JASSRC)jpc$(D)jpc_bs.h \
	$(JASSRC)jpc$(D)jpc_cod.h \
	$(JASSRC)jpc$(D)jpc_cs.h \
	$(JASSRC)jpc$(D)jpc_dec.h \
	$(JASSRC)jpc$(D)jpc_enc.h \
	$(JASSRC)jpc$(D)jpc_fix.h \
	$(JASSRC)jpc$(D)jpc_flt.h \
	$(JASSRC)jpc$(D)jpc_math.h \
	$(JASSRC)jpc$(D)jpc_mct.h \
	$(JASSRC)jpc$(D)jpc_mqcod.h \
	$(JASSRC)jpc$(D)jpc_mqdec.h \
	$(JASSRC)jpc$(D)jpc_mqenc.h \
	$(JASSRC)jpc$(D)jpc_qmfb.h \
	$(JASSRC)jpc$(D)jpc_tagtree.h \
	$(JASSRC)jpc$(D)jpc_t1cod.h \
	$(JASSRC)jpc$(D)jpc_t1dec.h \
	$(JASSRC)jpc$(D)jpc_t1enc.h \
	$(JASSRC)jpc$(D)jpc_tsfb.h \
	$(JASSRC)jpc$(D)jpc_t2cod.h \
	$(JASSRC)jpc$(D)jpc_t2dec.h \
	$(JASSRC)jpc$(D)jpc_t2enc.h \
	$(JASSRC)jpc$(D)jpc_util.h

libjasper_HDRS_jp2=\
	$(JASSRC)jp2$(D)jp2_cod.h \
	$(JASSRC)jp2$(D)jp2_dec.h

libjasper_HDRS=$(libjasper_HDRS_base) $(libjasper_HDRS_jpc) $(libjasper_HDRS_jp2)

jasper.clean : jasper.config-clean jasper.clean-not-config-clean

### WRONG.  MUST DELETE OBJ AND GEN FILES SELECTIVELY.
jasper.clean-not-config-clean :
	$(EXP)$(ECHOGS_XE) $(JASSRC) $(JASOBJ)
	$(RM_) $(JASOBJ)*.$(OBJ)

jasper.config-clean :
	$(RMN_) $(JASGEN)$(D)libjasper*.dev
	
JASDEP=$(AK) $(JASPER_MAK)

# hack: jasper uses EXCLUDE_fmt_SUPPORT defines to turn off unwanted
# format support. This keeps the noise down on the normal compiles
# where everything is enabled, but is inconvenient for us because
# we must disable everything that's implicitly included except those
# formats that we explicitly build. A better approach would be to
# patch jasper to invert the sense of these defines and to use
# config.h in its normal build to keep the noise down
JAS_EXCF_=\
	$(D_)EXCLUDE_BMP_SUPPORT$(_D_)1$(_D)\
	$(D_)EXCLUDE_JPG_SUPPORT$(_D_)1$(_D)\
	$(D_)EXCLUDE_MIF_SUPPORT$(_D_)1$(_D)\
	$(D_)EXCLUDE_PGX_SUPPORT$(_D_)1$(_D)\
        $(D_)EXCLUDE_PNM_SUPPORT$(_D_)1$(_D)\
        $(D_)EXCLUDE_RAS_SUPPORT$(_D_)1$(_D)\
        $(D_)EXCLUDE_PNG_SUPPORT$(_D_)1$(_D)\

JAS_CC=$(CC_) $(CFLAGS) $(I_)$(JASGEN) $(II)$(JASI_)$(_I) $(JASCF_) $(JAS_EXCF_)
JASO_=$(O_)$(JASOBJ)

# switch in the selected .dev
$(JASGEN)libjasper.dev : $(TOP_MAKEFILES) $(JASGEN)libjasper_$(SHARE_JASPER).dev
	$(CP_) $(JASGEN)libjasper_$(SHARE_JASPER).dev $(JASGEN)libjasper.dev

# external link .dev
$(GLOBJ)libjasper_1.dev : $(TOP_MAKEFILES) $(JASPER_MAK) $(ECHOGS_XE)
	$(SETMOD) $(GLOBJ)libjasper_1 -lib jasper

# compile in .dev
$(GLOBJ)libjasper_0.dev : $(TOP_MAKEFILES) $(JASPER_MAK) $(ECHOGS_XE) $(libjasper_OBJS)
	$(SETMOD) $(JASGEN)libjasper_0 $(libjasper_OBJS_base)
	$(ADDMOD) $(JASGEN)libjasper_0 $(libjasper_OBJS_jpc)
	$(ADDMOD) $(JASGEN)libjasper_0 $(libjasper_OBJS_jp2)

# explicit rules for building the source files
# for simplicity we have every source file depend on all headers

$(JASOBJ)jas_cm.$(OBJ) : $(JASSRC)base$(D)jas_cm.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_cm.$(OBJ) $(C_) $(JASSRC)base$(D)jas_cm.c

$(JASOBJ)jas_debug.$(OBJ) : $(JASSRC)base$(D)jas_debug.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_debug.$(OBJ) $(C_) $(JASSRC)base$(D)jas_debug.c

$(JASOBJ)jas_getopt.$(OBJ) : $(JASSRC)base$(D)jas_getopt.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_getopt.$(OBJ) $(C_) $(JASSRC)base$(D)jas_getopt.c

$(JASOBJ)jas_icc.$(OBJ) : $(JASSRC)base$(D)jas_icc.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_icc.$(OBJ) $(C_) $(JASSRC)base$(D)jas_icc.c

$(JASOBJ)jas_iccdata.$(OBJ) : $(JASSRC)base$(D)jas_iccdata.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_iccdata.$(OBJ) $(C_) $(JASSRC)base$(D)jas_iccdata.c

$(JASOBJ)jas_image.$(OBJ) : $(JASSRC)base$(D)jas_image.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_image.$(OBJ) $(C_) $(JASSRC)base$(D)jas_image.c

$(JASOBJ)jas_init.$(OBJ) : $(JASSRC)base$(D)jas_init.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_init.$(OBJ) $(C_) $(JASSRC)base$(D)jas_init.c

$(JASOBJ)jas_malloc.$(OBJ) : $(JASSRC)base$(D)jas_malloc.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_malloc.$(OBJ) $(C_) $(JASSRC)base$(D)jas_malloc.c

$(JASOBJ)jas_seq.$(OBJ) : $(JASSRC)base$(D)jas_seq.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_seq.$(OBJ) $(C_) $(JASSRC)base$(D)jas_seq.c

$(JASOBJ)jas_stream.$(OBJ) : $(JASSRC)base$(D)jas_stream.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_stream.$(OBJ) $(C_) $(JASSRC)base$(D)jas_stream.c

$(JASOBJ)jas_string.$(OBJ) : $(JASSRC)base$(D)jas_string.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_string.$(OBJ) $(C_) $(JASSRC)base$(D)jas_string.c

$(JASOBJ)jas_tvp.$(OBJ) : $(JASSRC)base$(D)jas_tvp.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_tvp.$(OBJ) $(C_) $(JASSRC)base$(D)jas_tvp.c

$(JASOBJ)jas_version.$(OBJ) : $(JASSRC)base$(D)jas_version.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jas_version.$(OBJ) $(C_) $(JASSRC)base$(D)jas_version.c


$(JASOBJ)jpc_bs.$(OBJ) : $(JASSRC)jpc$(D)jpc_bs.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_bs.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_bs.c

$(JASOBJ)jpc_cs.$(OBJ) : $(JASSRC)jpc$(D)jpc_cs.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_cs.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_cs.c

$(JASOBJ)jpc_dec.$(OBJ) : $(JASSRC)jpc$(D)jpc_dec.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_dec.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_dec.c

$(JASOBJ)jpc_enc.$(OBJ) : $(JASSRC)jpc$(D)jpc_enc.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_enc.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_enc.c

$(JASOBJ)jpc_math.$(OBJ) : $(JASSRC)jpc$(D)jpc_math.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_math.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_math.c

$(JASOBJ)jpc_mct.$(OBJ) : $(JASSRC)jpc$(D)jpc_mct.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_mct.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_mct.c

$(JASOBJ)jpc_mqcod.$(OBJ) : $(JASSRC)jpc$(D)jpc_mqcod.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_mqcod.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_mqcod.c

$(JASOBJ)jpc_mqdec.$(OBJ) : $(JASSRC)jpc$(D)jpc_mqdec.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_mqdec.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_mqdec.c

$(JASOBJ)jpc_mqenc.$(OBJ) : $(JASSRC)jpc$(D)jpc_mqenc.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_mqenc.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_mqenc.c

$(JASOBJ)jpc_qmfb.$(OBJ) : $(JASSRC)jpc$(D)jpc_qmfb.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_qmfb.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_qmfb.c

$(JASOBJ)jpc_tagtree.$(OBJ) : $(JASSRC)jpc$(D)jpc_tagtree.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_tagtree.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_tagtree.c

$(JASOBJ)jpc_t1cod.$(OBJ) : $(JASSRC)jpc$(D)jpc_t1cod.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_t1cod.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_t1cod.c

$(JASOBJ)jpc_t1dec.$(OBJ) : $(JASSRC)jpc$(D)jpc_t1dec.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_t1dec.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_t1dec.c

$(JASOBJ)jpc_t1enc.$(OBJ) : $(JASSRC)jpc$(D)jpc_t1enc.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_t1enc.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_t1enc.c

$(JASOBJ)jpc_tsfb.$(OBJ) : $(JASSRC)jpc$(D)jpc_tsfb.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_tsfb.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_tsfb.c

$(JASOBJ)jpc_t2cod.$(OBJ) : $(JASSRC)jpc$(D)jpc_t2cod.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_t2cod.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_t2cod.c

$(JASOBJ)jpc_t2dec.$(OBJ) : $(JASSRC)jpc$(D)jpc_t2dec.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_t2dec.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_t2dec.c

$(JASOBJ)jpc_t2enc.$(OBJ) : $(JASSRC)jpc$(D)jpc_t2enc.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_t2enc.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_t2enc.c

$(JASOBJ)jpc_util.$(OBJ) : $(JASSRC)jpc$(D)jpc_util.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jpc_util.$(OBJ) $(C_) $(JASSRC)jpc$(D)jpc_util.c


$(JASOBJ)jp2_cod.$(OBJ) : $(JASSRC)jp2$(D)jp2_cod.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jp2_cod.$(OBJ) $(C_) $(JASSRC)jp2$(D)jp2_cod.c

$(JASOBJ)jp2_dec.$(OBJ) : $(JASSRC)jp2$(D)jp2_dec.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jp2_dec.$(OBJ) $(C_) $(JASSRC)jp2$(D)jp2_dec.c

$(JASOBJ)jp2_enc.$(OBJ) : $(JASSRC)jp2$(D)jp2_enc.c $(JASDEP) $(libjasper_HDRS)
	$(JAS_CC) $(JASO_)jp2_enc.$(OBJ) $(C_) $(JASSRC)jp2$(D)jp2_enc.c

