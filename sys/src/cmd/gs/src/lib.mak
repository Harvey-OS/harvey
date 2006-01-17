#    Copyright (C) 1995-2004 artofcode LLC. All rights reserved.
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

# $Id: lib.mak,v 1.211 2005/08/24 19:02:22 giles Exp $
# (Platform-independent) makefile for Ghostscript graphics library
# and other support code.
# Users of this makefile must define the following:
#	GLSRCDIR - the source directory
#	GLGENDIR - the directory for source files generated during building
#	GLOBJDIR - the object code directory

GLSRC=$(GLSRCDIR)$(D)
GLGEN=$(GLGENDIR)$(D)
GLOBJ=$(GLOBJDIR)$(D)
GLO_=$(O_)$(GLOBJ)
GLI_=$(GLGENDIR) $(II)$(GLSRCDIR)
GLF_=
GLCCFLAGS=$(I_)$(GLI_)$(_I) $(GLF_)
GLCC=$(CC_) $(GLCCFLAGS)
GLICCCC=$(CC_) $(I_)$(GLI_) $(II)$(ICCI_)$(_I) $(ICCCF_) $(GLF_)
GLJCC=$(CC_) $(I_)$(GLI_) $(II)$(JI_)$(_I) $(JCF_) $(GLF_)
GLZCC=$(CC_) $(I_)$(GLI_) $(II)$(ZI_)$(_I) $(ZCF_) $(GLF_)
GLJBIG2CC=$(CC_) $(I_)$(GLI_) $(II)$(JB2I_)$(_I) $(JB2CF_) $(GLF_)
# All top-level makefiles define GLD.
#GLD=$(GLGEN)

# Define the name of this makefile.
LIB_MAK=$(GLSRC)lib.mak

# Define the inter-dependencies of the .h files.
# Since not all versions of `make' defer expansion of macros,
# we must list these in bottom-to-top order.

# Generic files

arch_h=$(GLGEN)arch.h
stdpn_h=$(GLSRC)stdpn.h
stdpre_h=$(GLSRC)stdpre.h $(stdpn_h)
std_h=$(GLSRC)std.h $(arch_h) $(stdpre_h)

$(GLGEN)arch.h : $(GENARCH_XE)
	$(EXP)$(GENARCH_XE) $(GLGEN)arch.h

# Platform interfaces

# gp.h requires gstypes.h and srdline.h.
gstypes_h=$(GLSRC)gstypes.h
srdline_h=$(GLSRC)srdline.h
gpgetenv_h=$(GLSRC)gpgetenv.h
gpmisc_h=$(GLSRC)gpmisc.h
gp_h=$(GLSRC)gp.h $(gpgetenv_h) $(gstypes_h) $(srdline_h)
gpcheck_h=$(GLSRC)gpcheck.h
gpsync_h=$(GLSRC)gpsync.h

# Configuration definitions

gconf_h=$(GLSRC)gconf.h $(gconfig_h)
# gconfig*.h are generated dynamically.
gconfig__h=$(GLGEN)gconfig_.h
gconfigv_h=$(GLGEN)gconfigv.h
gscdefs_h=$(GLSRC)gscdefs.h $(gconfigv_h)

# C library interfaces

# Because of variations in the "standard" header files between systems, and
# because we must include std.h before any file that includes sys/types.h,
# we define local include files named *_.h to substitute for <*.h>.

vmsmath_h=$(GLSRC)vmsmath.h

dos__h=$(GLSRC)dos_.h
ctype__h=$(GLSRC)ctype_.h $(std_h)
dirent__h=$(GLSRC)dirent_.h $(std_h) $(gconfig__h)
errno__h=$(GLSRC)errno_.h $(std_h)
fcntl__h=$(GLSRC)fcntl_.h $(std_h)
malloc__h=$(GLSRC)malloc_.h $(std_h)
math__h=$(GLSRC)math_.h $(std_h) $(vmsmath_h)
memory__h=$(GLSRC)memory_.h $(std_h)
stat__h=$(GLSRC)stat_.h $(std_h)
stdint__h=$(GLSRC)stdint_.h $(std_h)
stdio__h=$(GLSRC)stdio_.h $(std_h)
string__h=$(GLSRC)string_.h $(std_h)
time__h=$(GLSRC)time_.h $(std_h) $(gconfig__h)
unistd__h=$(GLSRC)unistd_.h $(std_h)
windows__h=$(GLSRC)windows_.h
# Out of order
pipe__h=$(GLSRC)pipe_.h $(stdio__h)

# Third-party library interfaces

jerror__h=$(GLSRC)jerror_.h $(MAKEFILE)
jpeglib__h=$(GLGEN)jpeglib_.h

# Miscellaneous

gsio_h=$(GLSRC)gsio.h
gxstdio_h=$(GLSRC)gxstdio.h $(gsio_h)
gs_dll_call_h=$(GLSRC)gs_dll_call.h
gslibctx_h=$(GLSRC)gslibctx.h $(std_h) $(stdio__h) $(gs_dll_call_h)
gdebug_h=$(GLSRC)gdebug.h
gsalloc_h=$(GLSRC)gsalloc.h
gsargs_h=$(GLSRC)gsargs.h
gserror_h=$(GLSRC)gserror.h
gserrors_h=$(GLSRC)gserrors.h
gsexit_h=$(GLSRC)gsexit.h
gsgc_h=$(GLSRC)gsgc.h
gsmalloc_h=$(GLSRC)gsmalloc.h
gsmdebug_h=$(GLSRC)gsmdebug.h
gsmemraw_h=$(GLSRC)gsmemraw.h
gsmemory_h=$(GLSRC)gsmemory.h $(gsmemraw_h) $(gstypes_h) $(gslibctx_h)
gsmemret_h=$(GLSRC)gsmemret.h $(gsmemory_h)
gsnogc_h=$(GLSRC)gsnogc.h $(gsgc_h)
gsrefct_h=$(GLSRC)gsrefct.h
gsserial_h=$(GLSRC)gsserial.h
gsstype_h=$(GLSRC)gsstype.h
gx_h=$(GLSRC)gx.h $(stdio__h) $(gdebug_h)\
 $(gserror_h) $(gsio_h) $(gsmemory_h) $(gstypes_h)
gxsync_h=$(GLSRC)gxsync.h $(gpsync_h) $(gsmemory_h)
# Out of order
gsmemlok_h=$(GLSRC)gsmemlok.h $(gsmemory_h) $(gxsync_h)
gsnotify_h=$(GLSRC)gsnotify.h $(gsstype_h)
gsstruct_h=$(GLSRC)gsstruct.h $(gsstype_h)
vdtrace_h=$(GLSRC)vdtrace.h

GX=$(AK) $(gx_h)
GXERR=$(GX) $(gserrors_h)

###### Support

### Include files

gsbitmap_h=$(GLSRC)gsbitmap.h $(gsstype_h)
gsbitops_h=$(GLSRC)gsbitops.h
gsbittab_h=$(GLSRC)gsbittab.h
gsflip_h=$(GLSRC)gsflip.h
gsuid_h=$(GLSRC)gsuid.h
gsutil_h=$(GLSRC)gsutil.h
gxarith_h=$(GLSRC)gxarith.h
gxbitmap_h=$(GLSRC)gxbitmap.h $(gsbitmap_h) $(gstypes_h)
gxfarith_h=$(GLSRC)gxfarith.h $(gconfigv_h) $(gxarith_h)
gxfixed_h=$(GLSRC)gxfixed.h
gxobj_h=$(GLSRC)gxobj.h $(gxbitmap_h)
gxrplane_h=$(GLSRC)gxrplane.h
# Out of order
gsrect_h=$(GLSRC)gsrect.h $(gxfixed_h)
gxalloc_h=$(GLSRC)gxalloc.h $(gsalloc_h) $(gxobj_h)
gxbitops_h=$(GLSRC)gxbitops.h $(gsbitops_h)
gxcindex_h=$(GLSRC)gxcindex.h $(gsbitops_h)

# Streams
scommon_h=$(GLSRC)scommon.h $(gsmemory_h) $(gstypes_h) $(gsstype_h)
stream_h=$(GLSRC)stream.h $(scommon_h) $(srdline_h)

### Memory manager

$(GLOBJ)gsalloc.$(OBJ) : $(GLSRC)gsalloc.c $(GXERR) $(memory__h) $(string__h)\
 $(gsexit_h) $(gsmdebug_h) $(gsstruct_h) $(gxalloc_h)\
 $(stream_h)
	$(GLCC) $(GLO_)gsalloc.$(OBJ) $(C_) $(GLSRC)gsalloc.c

$(GLOBJ)gsmalloc.$(OBJ) : $(GLSRC)gsmalloc.c $(malloc__h)\
 $(gdebug_h)\
 $(gserror_h) $(gserrors_h)\
 $(gsmalloc_h) $(gsmdebug_h) $(gsmemlok_h) $(gsmemret_h)\
 $(gsmemory_h) $(gsstruct_h) $(gstypes_h)
	$(GLCC) $(GLO_)gsmalloc.$(OBJ) $(C_) $(GLSRC)gsmalloc.c

$(GLOBJ)gsmemory.$(OBJ) : $(GLSRC)gsmemory.c $(memory__h)\
 $(gdebug_h)\
 $(gsmdebug_h) $(gsmemory_h) $(gsrefct_h) $(gsstruct_h) $(gstypes_h)
	$(GLCC) $(GLO_)gsmemory.$(OBJ) $(C_) $(GLSRC)gsmemory.c

$(GLOBJ)gsmemret.$(OBJ) : $(GLSRC)gsmemret.c $(GXERR) $(gsmemret_h)
	$(GLCC) $(GLO_)gsmemret.$(OBJ) $(C_) $(GLSRC)gsmemret.c

# gsnogc is not part of the base configuration.
# We make it available as a .dev so it can be used in configurations that
# don't include the garbage collector, as well as by the "async" logic.
gsnogc_=$(GLOBJ)gsnogc.$(OBJ)
$(GLD)gsnogc.dev : $(LIB_MAK) $(ECHOGS_XE) $(gsnogc_)
	$(SETMOD) $(GLD)gsnogc $(gsnogc_)

$(GLOBJ)gsnogc.$(OBJ) : $(GLSRC)gsnogc.c $(GX)\
 $(gsmdebug_h) $(gsnogc_h) $(gsstruct_h) $(gxalloc_h)
	$(GLCC) $(GLO_)gsnogc.$(OBJ) $(C_) $(GLSRC)gsnogc.c

### Bitmap processing

$(GLOBJ)gsbitcom.$(OBJ) : $(GLSRC)gsbitcom.c $(AK) $(std_h)\
 $(gdebug_h) $(gsbitops_h) $(gstypes_h)
	$(GLCC) $(GLO_)gsbitcom.$(OBJ) $(C_) $(GLSRC)gsbitcom.c

$(GLOBJ)gsbitops.$(OBJ) : $(GLSRC)gsbitops.c $(AK) $(memory__h) $(stdio__h)\
 $(gdebug_h) $(gsbittab_h) $(gserror_h) $(gserrors_h) $(gstypes_h)\
 $(gxbitops_h) $(gxcindex_h)
	$(GLCC) $(GLO_)gsbitops.$(OBJ) $(C_) $(GLSRC)gsbitops.c

$(GLOBJ)gsbittab.$(OBJ) : $(GLSRC)gsbittab.c $(AK) $(stdpre_h) $(gsbittab_h)
	$(GLCC) $(GLO_)gsbittab.$(OBJ) $(C_) $(GLSRC)gsbittab.c

# gsflip is not part of the standard configuration: it's rather large,
# and no standard facility requires it.
$(GLOBJ)gsflip.$(OBJ) : $(GLSRC)gsflip.c $(GXERR)\
 $(gsbitops_h) $(gsbittab_h) $(gsflip_h)
	$(GLCC) $(GLO_)gsflip.$(OBJ) $(C_) $(GLSRC)gsflip.c

### Multi-threading

# These are required in the standard configuration, because gsmalloc.c
# needs them even if the underlying primitives are dummies.

$(GLOBJ)gsmemlok.$(OBJ) : $(GLSRC)gsmemlok.c $(GXERR) $(gsmemlok_h)
	$(GLCC) $(GLO_)gsmemlok.$(OBJ) $(C_) $(GLSRC)gsmemlok.c

$(GLOBJ)gxsync.$(OBJ) : $(GLSRC)gxsync.c $(GXERR) $(memory__h)\
 $(gsmemory_h) $(gxsync_h)
	$(GLCC) $(GLO_)gxsync.$(OBJ) $(C_) $(GLSRC)gxsync.c

### Miscellaneous

# Support for platform code
$(GLOBJ)gpmisc.$(OBJ) : $(GLSRC)gpmisc.c\
 $(unistd__h) $(fcntl__h) $(stat__h) $(stdio__h)\
 $(memory__h) $(string__h) $(gp_h) $(gpgetenv_h) $(gpmisc_h)
	$(GLCC) $(GLO_)gpmisc.$(OBJ) $(C_) $(GLSRC)gpmisc.c

# Command line argument list management
$(GLOBJ)gsargs.$(OBJ) : $(GLSRC)gsargs.c\
 $(ctype__h) $(stdio__h) $(string__h)\
 $(gsargs_h) $(gsexit_h) $(gsmemory_h)\
 $(gserrors_h)
	$(GLCC) $(GLO_)gsargs.$(OBJ) $(C_) $(GLSRC)gsargs.c

# FPU emulation
# gsfemu is only used in FPU-less configurations, and currently only with gcc.
$(GLOBJ)gsfemu.$(OBJ) : $(GLSRC)gsfemu.c $(AK) $(std_h)
	$(GLCC) $(GLO_)gsfemu.$(OBJ) $(C_) $(GLSRC)gsfemu.c

$(GLOBJ)gsmisc.$(OBJ) : $(GLSRC)gsmisc.c $(GXERR)\
 $(gconfigv_h) $(std_h) $(vmsmath_h)\
 $(ctype__h) $(malloc__h) $(math__h) $(memory__h) $(string__h)\
 $(gpcheck_h) $(gserror_h) $(gxfarith_h) $(gxfixed_h)
	$(GLCC) $(GLO_)gsmisc.$(OBJ) $(C_) $(GLSRC)gsmisc.c

$(GLOBJ)gslibctx.$(OBJ) : $(GLSRC)gslibctx.c  $(GXERR)\
  $(gslibctx_h) $(stdio__h)
	$(GLCC) $(GLO_)gslibctx.$(OBJ) $(C_) $(GLSRC)gslibctx.c
  
$(GLOBJ)gsnotify.$(OBJ) : $(GLSRC)gsnotify.c $(GXERR)\
 $(gsnotify_h) $(gsstruct_h)
	$(GLCC) $(GLO_)gsnotify.$(OBJ) $(C_) $(GLSRC)gsnotify.c

$(GLOBJ)gsserial.$(OBJ) : $(GLSRC)gsserial.c $(stdpre_h) $(gstypes_h)\
 $(gsserial_h)
	$(GLCC) $(GLO_)gsserial.$(OBJ) $(C_) $(GLSRC)gsserial.c

$(GLOBJ)gsutil.$(OBJ) : $(GLSRC)gsutil.c $(AK) $(memory__h) $(string__h)\
 $(gconfigv_h)\
 $(gserror_h) $(gserrors_h) $(gsmemory_h) $(gsrect_h) $(gstypes_h)\
 $(gsuid_h) $(gsutil_h)
	$(GLCC) $(GLO_)gsutil.$(OBJ) $(C_) $(GLSRC)gsutil.c

# MD5 digest
md5_h=$(GLSRC)md5.h
# We have to use a slightly different compilation approach in order to
# get std.h included when compiling md5.c.
md5_=$(GLOBJ)md5.$(OBJ)
$(GLOBJ)md5.$(OBJ) : $(GLSRC)md5.c $(AK) $(memory__h) $(md5_h)
	$(EXP)$(ECHOGS_XE) -w $(GLGEN)md5.h -x 23 include -x 2022 memory_.h -x 22
	$(EXP)$(ECHOGS_XE) -a $(GLGEN)md5.h -+R $(GLSRC)md5.h
	$(CP_) $(GLSRC)md5.c $(GLGEN)md5.c
	$(GLCC) $(GLO_)md5.$(OBJ) $(C_) $(GLGEN)md5.c
	$(RM_) $(GLGEN)md5.c $(GLGEN)md5.h

# Visual Debugging
$(GLOBJ)vdtrace.$(OBJ) : $(GLSRC)vdtrace.c $(math__h)\
 $(gxfixed_h) $(vdtrace_h)
	$(GLCC) $(GLO_)vdtrace.$(OBJ) $(C_) $(GLSRC)vdtrace.c

###### Low-level facilities and utilities

### Include files

gsalpha_h=$(GLSRC)gsalpha.h
gsccode_h=$(GLSRC)gsccode.h
gsccolor_h=$(GLSRC)gsccolor.h $(gsstype_h)
# gscedata.[ch] are generated automatically by lib/encs2c.ps.
gscedata_h=$(GLSRC)gscedata.h
gscencs_h=$(GLSRC)gscencs.h $(stdpre_h) $(gstypes_h) $(gsccode_h)
gsclipsr_h=$(GLSRC)gsclipsr.h
gscsel_h=$(GLSRC)gscsel.h
gscolor1_h=$(GLSRC)gscolor1.h
gscompt_h=$(GLSRC)gscompt.h
gscoord_h=$(GLSRC)gscoord.h
gscpm_h=$(GLSRC)gscpm.h
gscsepnm_h=$(GLSRC)gscsepnm.h
gsdevice_h=$(GLSRC)gsdevice.h
gsdfilt_h=$(GLSRC)gsdfilt.h
gsfcmap_h=$(GLSRC)gsfcmap.h $(gsccode_h)
gsfname_h=$(GLSRC)gsfname.h
gsfont_h=$(GLSRC)gsfont.h
gsgdata_h=$(GLSRC)gsgdata.h $(gsstype_h)
gsgcache_h=$(GLSRC)gsgcache.h
gshsb_h=$(GLSRC)gshsb.h
gsht_h=$(GLSRC)gsht.h
gsht1_h=$(GLSRC)gsht1.h $(gsht_h)
gswts_h=$(GLSRC)gswts.h
gsjconf_h=$(GLSRC)gsjconf.h $(arch_h) $(stdpre_h)
gslib_h=$(GLSRC)gslib.h
gslparam_h=$(GLSRC)gslparam.h
gsmatrix_h=$(GLSRC)gsmatrix.h
# Out of order
gxbitfmt_h=$(GLSRC)gxbitfmt.h
gxcomp_h=$(GLSRC)gxcomp.h $(gscompt_h) $(gsrefct_h) $(gxbitfmt_h)
gsovrc_h=$(GLSRC)gsovrc.h $(gsstype_h) $(gxcomp_h)
gspaint_h=$(GLSRC)gspaint.h
gsparam_h=$(GLSRC)gsparam.h $(gsstype_h)
gsparams_h=$(GLSRC)gsparams.h $(gsparam_h) $(stream_h)
gsparamx_h=$(GLSRC)gsparamx.h
gspath2_h=$(GLSRC)gspath2.h
gspcolor_h=$(GLSRC)gspcolor.h $(gsccolor_h) $(gsrefct_h) $(gsuid_h)
gspenum_h=$(GLSRC)gspenum.h
gsptype1_h=$(GLSRC)gsptype1.h $(gspcolor_h) $(gxbitmap_h)
gsropt_h=$(GLSRC)gsropt.h
gstext_h=$(GLSRC)gstext.h $(gsccode_h) $(gscpm_h)
gsxfont_h=$(GLSRC)gsxfont.h
# Out of order
gschar_h=$(GLSRC)gschar.h $(gsccode_h) $(gscpm_h)
gsiparam_h=$(GLSRC)gsiparam.h $(gsccolor_h) $(gsmatrix_h) $(gsstype_h)
gsimage_h=$(GLSRC)gsimage.h $(gsiparam_h)
gsline_h=$(GLSRC)gsline.h $(gslparam_h)
gspath_h=$(GLSRC)gspath.h $(gspenum_h)
gsrop_h=$(GLSRC)gsrop.h $(gsropt_h)
gstparam_h=$(GLSRC)gstparam.h $(gsccolor_h) $(gsrefct_h)

gxalpha_h=$(GLSRC)gxalpha.h
gxbcache_h=$(GLSRC)gxbcache.h $(gxbitmap_h)
gxcvalue_h=$(GLSRC)gxcvalue.h
gxclio_h=$(GLSRC)gxclio.h $(gp_h)
gxclip_h=$(GLSRC)gxclip.h
gxclipsr_h=$(GLSRC)gxclipsr.h $(gsrefct_h)
gxcoord_h=$(GLSRC)gxcoord.h $(gscoord_h)
gxcpath_h=$(GLSRC)gxcpath.h
gxdda_h=$(GLSRC)gxdda.h
gxdevbuf_h=$(GLSRC)gxdevbuf.h $(gxrplane_h)
gxdevrop_h=$(GLSRC)gxdevrop.h
gxdevmem_h=$(GLSRC)gxdevmem.h $(gxrplane_h)
gxdhtres_h=$(GLSRC)gxdhtres.h $(stdpre_h)
gxfont0_h=$(GLSRC)gxfont0.h
gxfrac_h=$(GLSRC)gxfrac.h
gxftype_h=$(GLSRC)gxftype.h
gxgetbit_h=$(GLSRC)gxgetbit.h $(gxbitfmt_h)
gxhttile_h=$(GLSRC)gxhttile.h
gxhttype_h=$(GLSRC)gxhttype.h
gxiclass_h=$(GLSRC)gxiclass.h
gxiodev_h=$(GLSRC)gxiodev.h $(stat__h)
gxline_h=$(GLSRC)gxline.h $(gslparam_h) $(gsmatrix_h)
gxlum_h=$(GLSRC)gxlum.h
gxmatrix_h=$(GLSRC)gxmatrix.h $(gsmatrix_h)
gxmclip_h=$(GLSRC)gxmclip.h $(gxclip_h)
gxoprect_h=$(GLSRC)gxoprect.h
gxp1impl_h=$(GLSRC)gxp1impl.h
gxpaint_h=$(GLSRC)gxpaint.h
gxpath_h=$(GLSRC)gxpath.h $(gscpm_h) $(gslparam_h) $(gspenum_h) $(gsrect_h)
gxpcache_h=$(GLSRC)gxpcache.h
gxsample_h=$(GLSRC)gxsample.h
gxsamplp_h=$(GLSRC)gxsamplp.h
gxstate_h=$(GLSRC)gxstate.h
gxtext_h=$(GLSRC)gxtext.h $(gsrefct_h) $(gstext_h)
gxtmap_h=$(GLSRC)gxtmap.h
gxxfont_h=$(GLSRC)gxxfont.h $(gsccode_h) $(gsmatrix_h) $(gsuid_h) $(gsxfont_h)
# The following are out of order because they include other files.
gxband_h=$(GLSRC)gxband.h $(gxclio_h)
gxcdevn_h=$(GLSRC)gxcdevn.h $(gsrefct_h) $(gxcindex_h)
gxchar_h=$(GLSRC)gxchar.h $(gschar_h) $(gxtext_h)
gxchrout_h=$(GLSRC)gxchrout.h
gxwts_h=$(GLSRC)gxwts.h
gsdcolor_h=$(GLSRC)gsdcolor.h $(gsccolor_h)\
 $(gxarith_h) $(gxbitmap_h) $(gxcindex_h) $(gxhttile_h) $(gxwts_h)
gxdcolor_h=$(GLSRC)gxdcolor.h\
 $(gscsel_h) $(gsdcolor_h) $(gsropt_h) $(gsstruct_h)
gscspace_h=$(GLSRC)gscspace.h $(gsmemory_h)
gscssub_h=$(GLSRC)gscssub.h $(gscspace_h)
gxdevcli_h=$(GLSRC)gxdevcli.h $(std_h) $(stdint__h)\
 $(gscompt_h) $(gsdcolor_h) $(gsiparam_h) $(gsmatrix_h)\
 $(gsrefct_h) $(gsropt_h) $(gsstruct_h) $(gstparam_h) $(gsxfont_h)\
 $(gxbitmap_h) $(gxcindex_h) $(gxcvalue_h) $(gxfixed_h)\
 $(gxtext_h) $(gscspace_h)
gxdevice_h=$(GLSRC)gxdevice.h $(stdio__h)\
 $(gsfname_h) $(gsmalloc_h) $(gsparam_h) $(gxdevcli_h) $(gxstdio_h)
gxdht_h=$(GLSRC)gxdht.h\
 $(gsmatrix_h) $(gsrefct_h) $(gxarith_h) $(gxhttype_h) $(gscspace_h)\
 $(gxcindex_h) $(gxfrac_h)
gxdhtserial_h=$(GLSRC)gxdhtserial.h
gxdither_h=$(GLSRC)gxdither.h $(gxfrac_h)
gxclip2_h=$(GLSRC)gxclip2.h $(gxmclip_h)
gxclipm_h=$(GLSRC)gxclipm.h $(gxmclip_h)
gxctable_h=$(GLSRC)gxctable.h $(gxfixed_h) $(gxfrac_h)
gxfcache_h=$(GLSRC)gxfcache.h $(gsccode_h) $(gsuid_h) $(gsxfont_h)\
 $(gxbcache_h) $(gxfixed_h) $(gxftype_h)
gxfcopy_h=$(GLSRC)gxfcopy.h $(gsccode_h)
gxfont_h=$(GLSRC)gxfont.h\
 $(gsccode_h) $(gsfont_h) $(gsgdata_h) $(gsmatrix_h) $(gsnotify_h)\
 $(gsstype_h) $(gsuid_h)\
 $(gxftype_h)
gxiparam_h=$(GLSRC)gxiparam.h $(gsstype_h) $(gxdevcli_h)
gscie_h=$(GLSRC)gscie.h $(gconfigv_h) $(gsrefct_h) $(gsstype_h) $(gstypes_h) $(gxctable_h)
gsicc_h=$(GLSRC)gsicc.h $(gscie_h)
gscrd_h=$(GLSRC)gscrd.h $(gscie_h)
gscrdp_h=$(GLSRC)gscrdp.h $(gscie_h) $(gsparam_h)
gscdevn_h=$(GLSRC)gscdevn.h $(gscspace_h)
gxdevndi_h=$(GLSRC)gxdevndi.h $(gxfrac_h)
gscindex_h=$(GLSRC)gscindex.h $(gscspace_h)
gscolor2_h=$(GLSRC)gscolor2.h $(gscindex_h) $(gsptype1_h)
gscsepr_h=$(GLSRC)gscsepr.h $(gscspace_h)
gxdcconv_h=$(GLSRC)gxdcconv.h $(gxfrac_h)
gxfmap_h=$(GLSRC)gxfmap.h $(gsrefct_h) $(gsstype_h) $(gxfrac_h) $(gxtmap_h)
gxcmap_h=$(GLSRC)gxcmap.h $(gscsel_h) $(gxcindex_h) $(gxcvalue_h) $(gxfmap_h)
gxistate_h=$(GLSRC)gxistate.h\
 $(gscsel_h) $(gsrefct_h) $(gsropt_h) $(gstparam_h) $(gxcvalue_h) $(gxcmap_h)\
 $(gxfixed_h) $(gxline_h) $(gxmatrix_h) $(gxtmap_h) $(gscspace_h)
gxclist_h=$(GLSRC)gxclist.h $(gscspace_h)\
 $(gxband_h) $(gxbcache_h) $(gxclio_h) $(gxdevbuf_h) $(gxistate_h)\
 $(gxrplane_h)
gxcolor2_h=$(GLSRC)gxcolor2.h\
 $(gscolor2_h) $(gsmatrix_h) $(gsrefct_h) $(gxbitmap_h)
gxcspace_h=$(GLSRC)gxcspace.h\
 $(gscspace_h) $(gsccolor_h) $(gscsel_h) $(gxfrac_h)
gxht_h=$(GLSRC)gxht.h $(gsht1_h) $(gsrefct_h) $(gxhttype_h) $(gxtmap_h) $(gscspace_h)
gxcie_h=$(GLSRC)gxcie.h $(gscie_h)
gxpcolor_h=$(GLSRC)gxpcolor.h\
 $(gspcolor_h) $(gxcspace_h) $(gxdevice_h) $(gxdevmem_h) $(gxpcache_h)
gscolor_h=$(GLSRC)gscolor.h $(gxtmap_h)
gsstate_h=$(GLSRC)gsstate.h\
 $(gscolor_h) $(gscpm_h) $(gscsel_h) $(gsdevice_h) $(gsht_h) $(gsline_h)

gzacpath_h=$(GLSRC)gzacpath.h
gzcpath_h=$(GLSRC)gzcpath.h $(gxcpath_h)
gzht_h=$(GLSRC)gzht.h $(gscsel_h)\
 $(gxdht_h) $(gxfmap_h) $(gxht_h) $(gxhttile_h)
gzline_h=$(GLSRC)gzline.h $(gxline_h)
gzpath_h=$(GLSRC)gzpath.h $(gsmatrix_h) $(gsrefct_h) $(gsstype_h) $(gxpath_h)
gzstate_h=$(GLSRC)gzstate.h $(gscpm_h) $(gscspace_h) $(gsrefct_h) $(gsstate_h)\
 $(gxdcolor_h) $(gxistate_h) $(gxstate_h)

gdevbbox_h=$(GLSRC)gdevbbox.h
gdevmem_h=$(GLSRC)gdevmem.h $(gxbitops_h)
gdevmpla_h=$(GLSRC)gdevmpla.h
gdevmrop_h=$(GLSRC)gdevmrop.h
gdevmrun_h=$(GLSRC)gdevmrun.h $(gxdevmem_h)
gdevplnx_h=$(GLSRC)gdevplnx.h $(gxrplane_h)

sa85d_h=$(GLSRC)sa85d.h
sa85x_h=$(GLSRC)sa85x.h $(sa85d_h)
sbcp_h=$(GLSRC)sbcp.h
sbtx_h=$(GLSRC)sbtx.h
scanchar_h=$(GLSRC)scanchar.h
sfilter_h=$(GLSRC)sfilter.h $(gstypes_h)
sdct_h=$(GLSRC)sdct.h
shc_h=$(GLSRC)shc.h $(gsbittab_h) $(scommon_h)
sisparam_h=$(GLSRC)sisparam.h
sjpeg_h=$(GLSRC)sjpeg.h
slzwx_h=$(GLSRC)slzwx.h
smd5_h=$(GLSRC)smd5.h $(md5_h)
sarc4_h=$(GLSRC)sarc4.h $(scommon_h)
sjbig2_h=$(GLSRC)sjbig2.h $(stdint__h) $(scommon_h)
sjpx_h=$(GLSRC)sjpx.h $(scommon_h)
spdiffx_h=$(GLSRC)spdiffx.h
spngpx_h=$(GLSRC)spngpx.h
spprint_h=$(GLSRC)spprint.h
spsdf_h=$(GLSRC)spsdf.h $(gsparam_h)
srlx_h=$(GLSRC)srlx.h
sstring_h=$(GLSRC)sstring.h
strimpl_h=$(GLSRC)strimpl.h $(scommon_h) $(gstypes_h) $(gsstruct_h)
szlibx_h=$(GLSRC)szlibx.h
szlibxx_h=$(GLSRC)szlibxx.h $(szlibx_h)
# Out of order
scf_h=$(GLSRC)scf.h $(shc_h)
scfx_h=$(GLSRC)scfx.h $(shc_h)
siinterp_h=$(GLSRC)siinterp.h $(sisparam_h)
siscale_h=$(GLSRC)siscale.h $(sisparam_h)
gximage_h=$(GLSRC)gximage.h $(gsiparam_h)\
 $(gxcspace_h) $(gxdda_h) $(gxiclass_h) $(gxiparam_h) $(gxsample_h)\
 $(sisparam_h) $(strimpl_h)
gxhldevc_h=$(GLSRC)gxhldevc.h $(gsdcolor_h)
gsptype2_h=$(GLSRC)gsptype2.h $(gspcolor_h) $(gxhldevc_h) $(gxfixed_h)
gdevddrw_h=$(GLSRC)gdevddrw.h
gxfill_h=$(GLSRC)gxfill.h
gxfilltr_h=$(GLSRC)gxfilltr.h
gxfillsl_h=$(GLSRC)gxfillsl.h
gxfillts_h=$(GLSRC)gxfillts.h
gxfdrop_h=$(GLSRC)gxfdrop.h
gxdtfill_h=$(GLSRC)gxdtfill.h

ttfoutl_h=$(GLSRC)ttfoutl.h
gxttfb_h = $(GLSRC)gxttfb.h $(ttfoutl_h)
gzspotan_h = $(GLSRC)gzspotan.h $(gxdevcli_h)

### Executable code

# gconfig and gscdefs are handled specially.  Currently they go in psbase
# rather than in libcore, which is clearly wrong.
$(GLOBJ)gconfig.$(OBJ) : $(gconfig_h) $(GLSRC)gconf.c $(GX)\
 $(gscdefs_h) $(gconf_h)\
 $(gxdevice_h) $(gxiclass_h) $(gxiodev_h) $(gxiparam_h) $(TOP_MAKEFILES)
	$(RM_) $(GLGEN)gconfig.c
	$(RM_) $(GLGEN)gconfig.h
	$(CP_) $(gconfig_h) $(GLGEN)gconfig.h
	$(CP_) $(GLSRC)gconf.c $(GLGEN)gconfig.c
	$(GLCC) $(GLO_)gconfig.$(OBJ) $(C_) $(GLGEN)gconfig.c

$(GLOBJ)gscdefs.$(OBJ) : $(GLSRC)gscdef.c\
 $(std_h) $(gscdefs_h) $(gconfigd_h) $(TOP_MAKEFILES)
	$(RM_) $(GLGEN)gscdefs.c
	$(CP_) $(GLSRC)gscdef.c $(GLGEN)gscdefs.c
	$(GLCC) $(GLO_)gscdefs.$(OBJ) $(C_) $(GLGEN)gscdefs.c

$(GLOBJ)gxacpath.$(OBJ) : $(GLSRC)gxacpath.c $(GXERR)\
 $(gsdcolor_h) $(gsrop_h) $(gsstruct_h) $(gsutil_h)\
 $(gxdevice_h) $(gxfixed_h) $(gxistate_h) $(gxpaint_h)\
 $(gzacpath_h) $(gzcpath_h) $(gzpath_h)
	$(GLCC) $(GLO_)gxacpath.$(OBJ) $(C_) $(GLSRC)gxacpath.c

$(GLOBJ)gxbcache.$(OBJ) : $(GLSRC)gxbcache.c $(GX) $(memory__h)\
 $(gsmdebug_h) $(gxbcache_h)
	$(GLCC) $(GLO_)gxbcache.$(OBJ) $(C_) $(GLSRC)gxbcache.c

$(GLOBJ)gxccache.$(OBJ) : $(GLSRC)gxccache.c $(GXERR) $(memory__h)\
 $(gpcheck_h)\
 $(gscencs_h) $(gxfixed_h) $(gxmatrix_h)\
 $(gzstate_h) $(gzpath_h) $(gxdevice_h) $(gxdevmem_h)\
 $(gzcpath_h) $(gxchar_h) $(gxfont_h) $(gxfcache_h)\
 $(gxxfont_h) $(gscspace_h) $(gximage_h) $(gxhttile_h)
	$(GLCC) $(GLO_)gxccache.$(OBJ) $(C_) $(GLSRC)gxccache.c

$(GLOBJ)gxccman.$(OBJ) : $(GLSRC)gxccman.c $(GXERR) $(memory__h) $(gpcheck_h)\
 $(gsbitops_h) $(gsstruct_h) $(gsutil_h) $(gxfixed_h) $(gxmatrix_h)\
 $(gxdevice_h) $(gxdevmem_h) $(gxfont_h) $(gxfcache_h) $(gxchar_h)\
 $(gxpath_h) $(gxxfont_h) $(gzstate_h) $(gxttfb_h) $(gxttfb_h) $(gxfont42_h)
	$(GLCC) $(GLO_)gxccman.$(OBJ) $(C_) $(GLSRC)gxccman.c

$(GLOBJ)gxchar.$(OBJ) : $(GLSRC)gxchar.c $(GXERR) $(memory__h) $(string__h)\
 $(gspath_h) $(gsstruct_h)\
 $(gxfixed_h) $(gxarith_h) $(gxmatrix_h) $(gxcoord_h) $(gxdevice_h) $(gxdevmem_h)\
 $(gxfont_h) $(gxfont0_h) $(gxchar_h) $(gxfcache_h) $(gzpath_h) $(gzstate_h)
	$(GLCC) $(GLO_)gxchar.$(OBJ) $(C_) $(GLSRC)gxchar.c

$(GLOBJ)gxchrout.$(OBJ) : $(GLSRC)gxchrout.c $(GX) $(math__h)\
 $(gxchrout_h) $(gxfarith_h) $(gxistate_h)
	$(GLCC) $(GLO_)gxchrout.$(OBJ) $(C_) $(GLSRC)gxchrout.c

$(GLOBJ)gxcht.$(OBJ) : $(GLSRC)gxcht.c $(GXERR) $(memory__h)\
 $(gsutil_h)\
 $(gxarith_h) $(gxcmap_h) $(gxdcolor_h) $(gxdevice_h) $(gxfixed_h)\
 $(gxistate_h) $(gxmatrix_h) $(gzht_h) $(gsserial_h)
	$(GLCC) $(GLO_)gxcht.$(OBJ) $(C_) $(GLSRC)gxcht.c

$(GLOBJ)gxclip.$(OBJ) : $(GLSRC)gxclip.c $(GX)\
 $(gxclip_h) $(gxcpath_h) $(gxdevice_h) $(gxpath_h)
	$(GLCC) $(GLO_)gxclip.$(OBJ) $(C_) $(GLSRC)gxclip.c

$(GLOBJ)gxcmap.$(OBJ) : $(GLSRC)gxcmap.c $(GXERR)\
 $(gsccolor_h)\
 $(gxalpha_h) $(gxcspace_h) $(gxfarith_h) $(gxfrac_h)\
 $(gxdcconv_h) $(gxdevice_h) $(gxcmap_h) $(gxlum_h)\
 $(gzstate_h) $(gxdither_h) $(gxcdevn_h) $(string__h)
	$(GLCC) $(GLO_)gxcmap.$(OBJ) $(C_) $(GLSRC)gxcmap.c

$(GLOBJ)gxcpath.$(OBJ) : $(GLSRC)gxcpath.c $(GXERR)\
 $(gscoord_h) $(gsline_h) $(gsstruct_h) $(gsutil_h)\
 $(gxdevice_h) $(gxfixed_h) $(gxistate_h) $(gzpath_h) $(gzcpath_h) $(gzacpath_h)
	$(GLCC) $(GLO_)gxcpath.$(OBJ) $(C_) $(GLSRC)gxcpath.c

$(GLOBJ)gxdcconv.$(OBJ) : $(GLSRC)gxdcconv.c $(GX)\
 $(gsdcolor_h) $(gxcmap_h) $(gxdcconv_h) $(gxdevice_h)\
 $(gxfarith_h) $(gxistate_h) $(gxlum_h)
	$(GLCC) $(GLO_)gxdcconv.$(OBJ) $(C_) $(GLSRC)gxdcconv.c

$(GLOBJ)gxdcolor.$(OBJ) : $(GLSRC)gxdcolor.c $(GX)\
 $(memory__h) $(gsbittab_h) $(gserrors_h) $(gxdcolor_h) $(gxdevice_h)
	$(GLCC) $(GLO_)gxdcolor.$(OBJ) $(C_) $(GLSRC)gxdcolor.c

$(GLOBJ)gxhldevc.$(OBJ) : $(GLSRC)gxhldevc.c $(GX)\
 $(gzstate_h) $(gscspace_h) $(gxcspace_h) $(gxhldevc_h) $(memory_h)\
 $(gxpcolor_h) $(gsptype2_h)
	$(GLCC) $(GLO_)gxhldevc.$(OBJ) $(C_) $(GLSRC)gxhldevc.c

$(GLOBJ)gxfill.$(OBJ) : $(GLSRC)gxfill.c $(GXERR)\
 $(gsstruct_h)\
 $(gxdcolor_h) $(gxdevice_h) $(gxfixed_h) $(gxhttile_h)\
 $(gxistate_h) $(gxpaint_h) $(gxfill_h) $(gxfdrop_h) $(gsptype2_h)\
 $(gzcpath_h) $(gzpath_h) $(gzspotan_h) $(gdevddrw_h) $(memory__h) $(stdint_) $(vdtrace_h)\
 $(gxfilltr_h) $(gxfillsl_h) $(gxfillts_h)
	$(GLCC) $(GLO_)gxfill.$(OBJ) $(C_) $(GLSRC)gxfill.c

$(GLOBJ)gxfdrop.$(OBJ) : $(GLSRC)gxfdrop.c $(GXERR)\
 $(gsstruct_h) $(gzpath_h) $(gxfixed_h) $(gxdevice_h) $(gxdcolor_h) $(gxfill_h) $(gxfdrop_h)
	$(GLCC) $(GLO_)gxfdrop.$(OBJ) $(C_) $(GLSRC)gxfdrop.c

$(GLOBJ)gxht.$(OBJ) : $(GLSRC)gxht.c $(GXERR) $(memory__h)\
 $(gsbitops_h) $(gsstruct_h) $(gsutil_h)\
 $(gxdcolor_h) $(gxdevice_h) $(gxfixed_h) $(gxistate_h) $(gzht_h)\
 $(gsserial_h)
	$(GLCC) $(GLO_)gxht.$(OBJ) $(C_) $(GLSRC)gxht.c

$(GLOBJ)gxhtbit.$(OBJ) : $(GLSRC)gxhtbit.c $(GXERR) $(memory__h)\
 $(gsbitops_h) $(gscdefs_h)\
 $(gxbitmap_h) $(gxdht_h) $(gxdhtres_h) $(gxhttile_h) $(gxtmap_h)
	$(GLCC) $(GLO_)gxhtbit.$(OBJ) $(C_) $(GLSRC)gxhtbit.c

$(GLOBJ)gxwts.$(OBJ) : $(GLSRC)gxwts.c $(GXERR) $(gxwts_h)\
 $(stdpre_h) $(memory__h) $(gxstate_h) $(gxht_h) $(math__h) $(gxdevcli_h)\
 $(gxdht_h)
	$(GLCC) $(GLO_)gxwts.$(OBJ) $(C_) $(GLSRC)gxwts.c

$(GLOBJ)gswts.$(OBJ) : $(GLSRC)gswts.c $(GXERR) $(gxwts_h) $(gswts_h)\
 $(stdpre_h) $(gx_h) $(gxstate_h) $(gsht_h) $(math__h) $(gxfrac_h)
	$(GLCC) $(GLO_)gswts.$(OBJ) $(C_) $(GLSRC)gswts.c

$(GLOBJ)gxidata.$(OBJ) : $(GLSRC)gxidata.c $(GXERR) $(memory__h)\
 $(gxcpath_h) $(gxdevice_h) $(gximage_h)
	$(GLCC) $(GLO_)gxidata.$(OBJ) $(C_) $(GLSRC)gxidata.c

$(GLOBJ)gxifast.$(OBJ) : $(GLSRC)gxifast.c $(GXERR) $(memory__h) $(gpcheck_h)\
 $(gdevmem_h) $(gsbittab_h) $(gsccolor_h) $(gspaint_h) $(gsutil_h)\
 $(gxarith_h) $(gxcmap_h) $(gxcpath_h) $(gxdcolor_h) $(gxdevice_h)\
 $(gxdevmem_h) $(gxfixed_h) $(gximage_h) $(gxistate_h) $(gxmatrix_h)\
 $(gzht_h)
	$(GLCC) $(GLO_)gxifast.$(OBJ) $(C_) $(GLSRC)gxifast.c

$(GLOBJ)gximage.$(OBJ) : $(GLSRC)gximage.c $(GXERR) $(memory__h)\
 $(gscspace_h) $(gsmatrix_h) $(gsutil_h)\
 $(gxcolor2_h) $(gxiparam_h)\
 $(stream_h)
	$(GLCC) $(GLO_)gximage.$(OBJ) $(C_) $(GLSRC)gximage.c

$(GLOBJ)gximage1.$(OBJ) : $(GLSRC)gximage1.c $(GXERR)\
 $(gximage_h) $(gxiparam_h) $(stream_h)
	$(GLCC) $(GLO_)gximage1.$(OBJ) $(C_) $(GLSRC)gximage1.c

$(GLOBJ)gximono.$(OBJ) : $(GLSRC)gximono.c $(GXERR) $(memory__h) $(gpcheck_h)\
 $(gdevmem_h) $(gsccolor_h) $(gspaint_h) $(gsutil_h)\
 $(gxarith_h) $(gxcmap_h) $(gxcpath_h) $(gxdcolor_h) $(gxdevice_h)\
 $(gxdevmem_h) $(gxfixed_h) $(gximage_h) $(gxistate_h) $(gxmatrix_h)\
 $(gzht_h)
	$(GLCC) $(GLO_)gximono.$(OBJ) $(C_) $(GLSRC)gximono.c

$(GLOBJ)gxipixel.$(OBJ) : $(GLSRC)gxipixel.c $(GXERR) $(math__h) $(memory__h)\
 $(gpcheck_h)\
 $(gsccolor_h) $(gscdefs_h) $(gspaint_h) $(gsstruct_h) $(gsutil_h)\
 $(gxfixed_h) $(gxfrac_h) $(gxarith_h) $(gxiparam_h) $(gxmatrix_h)\
 $(gxdevice_h) $(gzpath_h) $(gzstate_h)\
 $(gzcpath_h) $(gxdevmem_h) $(gximage_h) $(gdevmrop_h)
	$(GLCC) $(GLO_)gxipixel.$(OBJ) $(C_) $(GLSRC)gxipixel.c

# gxmclip is used for Patterns and ImageType 3 images:
# it isn't included in the base library.
$(GLOBJ)gxmclip.$(OBJ) : $(GLSRC)gxmclip.c $(GX)\
 $(gxdevice_h) $(gxdevmem_h) $(gxmclip_h)
	$(GLCC) $(GLO_)gxmclip.$(OBJ) $(C_) $(GLSRC)gxmclip.c

$(GLOBJ)gxpaint.$(OBJ) : $(GLSRC)gxpaint.c $(GX)\
 $(gxdevice_h) $(gxhttile_h) $(gxpaint_h) $(gxpath_h) $(gzstate_h) $(gxfont_h)
	$(GLCC) $(GLO_)gxpaint.$(OBJ) $(C_) $(GLSRC)gxpaint.c

$(GLOBJ)gxpath.$(OBJ) : $(GLSRC)gxpath.c $(GXERR)\
 $(gsstruct_h) $(gxfixed_h) $(gzpath_h) $(vdtrace_h)
	$(GLCC) $(GLO_)gxpath.$(OBJ) $(C_) $(GLSRC)gxpath.c

$(GLOBJ)gxpath2.$(OBJ) : $(GLSRC)gxpath2.c $(GXERR) $(math__h)\
 $(gspath_h) $(gsstruct_h) $(gxfixed_h) $(gxarith_h) $(gzpath_h)
	$(GLCC) $(GLO_)gxpath2.$(OBJ) $(C_) $(GLSRC)gxpath2.c

$(GLOBJ)gxpcopy.$(OBJ) : $(GLSRC)gxpcopy.c $(GXERR) $(math__h) $(gconfigv_h)\
 $(gxfarith_h) $(gxfixed_h) $(gxistate_h) $(gzpath_h) $(vdtrace_h)
	$(GLCC) $(GLO_)gxpcopy.$(OBJ) $(C_) $(GLSRC)gxpcopy.c

$(GLOBJ)gxpdash.$(OBJ) : $(GLSRC)gxpdash.c $(GX) $(math__h)\
 $(gscoord_h) $(gsline_h) $(gsmatrix_h)\
 $(gxfixed_h) $(gzline_h) $(gzpath_h)
	$(GLCC) $(GLO_)gxpdash.$(OBJ) $(C_) $(GLSRC)gxpdash.c

$(GLOBJ)gxpflat.$(OBJ) : $(GLSRC)gxpflat.c $(GX)\
 $(gxarith_h) $(gxfixed_h) $(gzpath_h) $(memory__h) $(string__h)\
 $(vdtrace_h)
	$(GLCC) $(GLO_)gxpflat.$(OBJ) $(C_) $(GLSRC)gxpflat.c

$(GLOBJ)gxsample.$(OBJ) : $(GLSRC)gxsample.c $(GX)\
 $(gxsample_h) $(gxfixed_h) $(gximage_h) $(gxsamplp_h) 
	$(GLCC) $(GLO_)gxsample.$(OBJ) $(C_) $(GLSRC)gxsample.c

$(GLOBJ)gxstroke.$(OBJ) : $(GLSRC)gxstroke.c $(GXERR) $(math__h) $(gpcheck_h)\
 $(gscoord_h) $(gsdcolor_h) $(gsdevice_h)\
 $(gxdevice_h) $(gxfarith_h) $(gxfixed_h)\
 $(gxhttile_h) $(gxistate_h) $(gxmatrix_h) $(gxpaint_h)\
 $(gzcpath_h) $(gzline_h) $(gzpath_h) $(vdtrace_h)
	$(GLCC) $(GLO_)gxstroke.$(OBJ) $(C_) $(GLSRC)gxstroke.c

###### Higher-level facilities

$(GLOBJ)gsalpha.$(OBJ) : $(GLSRC)gsalpha.c $(GX)\
 $(gsalpha_h) $(gxdcolor_h) $(gzstate_h)
	$(GLCC) $(GLO_)gsalpha.$(OBJ) $(C_) $(GLSRC)gsalpha.c

# gscedata.[ch] are generated automatically by lib/encs2c.ps.
$(GLOBJ)gscedata.$(OBJ) : $(GLSRC)gscedata.c\
 $(stdpre_h) $(gstypes_h) $(gscedata_h)
	$(GLCC) $(GLO_)gscedata.$(OBJ) $(C_) $(GLSRC)gscedata.c

$(GLOBJ)gscencs.$(OBJ) : $(GLSRC)gscencs.c\
 $(memory__h) $(gscedata_h) $(gscencs_h) $(gserror_h) $(gserrors_h)
	$(GLCC) $(GLO_)gscencs.$(OBJ) $(C_) $(GLSRC)gscencs.c

$(GLOBJ)gschar.$(OBJ) : $(GLSRC)gschar.c $(GXERR)\
 $(gscoord_h) $(gsmatrix_h) $(gsstruct_h)\
 $(gxdevice_h) $(gxdevmem_h) $(gxchar_h) $(gxfont_h) $(gzstate_h)
	$(GLCC) $(GLO_)gschar.$(OBJ) $(C_) $(GLSRC)gschar.c

$(GLOBJ)gscolor.$(OBJ) : $(GLSRC)gscolor.c $(GXERR)\
 $(gsccolor_h) $(gsstruct_h) $(gsutil_h)\
 $(gxcmap_h) $(gxcspace_h) $(gxdcconv_h) $(gxdevice_h) $(gzstate_h)
	$(GLCC) $(GLO_)gscolor.$(OBJ) $(C_) $(GLSRC)gscolor.c

$(GLOBJ)gscoord.$(OBJ) : $(GLSRC)gscoord.c $(GXERR) $(math__h)\
 $(gsccode_h) $(gxcoord_h) $(gxdevice_h) $(gxfarith_h) $(gxfixed_h) $(gxfont_h)\
 $(gxmatrix_h) $(gxpath_h) $(gzstate_h)
	$(GLCC) $(GLO_)gscoord.$(OBJ) $(C_) $(GLSRC)gscoord.c

$(GLOBJ)gscparam.$(OBJ) : $(GLSRC)gscparam.c $(GXERR) $(memory__h) $(string__h)\
 $(gsparam_h) $(gsstruct_h)
	$(GLCC) $(GLO_)gscparam.$(OBJ) $(C_) $(GLSRC)gscparam.c

$(GLOBJ)gscspace.$(OBJ) : $(GLSRC)gscspace.c $(GXERR) $(memory__h)\
 $(gsstruct_h) $(gsccolor_h) $(gsutil_h)\
 $(gxcmap_h) $(gxcspace_h) $(gxistate_h) $(gsovrc_h) $(gsstate_h)\
 $(gsdevice_h) $(gxdevcli_h) $(gzstate_h) $(stream_h)
	$(GLCC) $(GLO_)gscspace.$(OBJ) $(C_) $(GLSRC)gscspace.c

$(GLOBJ)gsovrc.$(OBJ) : $(GLSRC)gsovrc.c $(GXERR)\
 $(memory__h) $(gsutil_h) $(gxcomp_h) $(gxdevice_h) $(gsdevice_h) $(gxgetbit_h)\
 $(gsovrc_h) $(gxdcolor_h) $(gxoprect_h) $(gsbitops_h) $(gxistate_h)
	$(GLCC) $(GLO_)gsovrc.$(OBJ) $(C_) $(GLSRC)gsovrc.c

$(GLOBJ)gxoprect.$(OBJ) : $(GLSRC)gxoprect.c $(GXERR)\
 $(memory__h) $(gsutil_h) $(gxdevice_h) $(gsdevice_h) $(gxgetbit_h)\
 $(gxoprect_h) $(gsbitops_h)
	$(GLCC) $(GLO_)gxoprect.$(OBJ) $(C_) $(GLSRC)gxoprect.c

$(GLOBJ)gsdevice.$(OBJ) : $(GLSRC)gsdevice.c $(GXERR)\
 $(ctype__h) $(memory__h) $(string__h) $(gp_h)\
 $(gscdefs_h) $(gsfname_h) $(gsstruct_h) $(gspath_h)\
 $(gspaint_h) $(gsmatrix_h) $(gscoord_h) $(gzstate_h)\
 $(gxcmap_h) $(gxdevice_h) $(gxdevmem_h) $(gxiodev_h) $(gxcspace_h)
	$(GLCC) $(GLO_)gsdevice.$(OBJ) $(C_) $(GLSRC)gsdevice.c

$(GLOBJ)gsdfilt.$(OBJ) : $(GLSRC)gsdfilt.c $(GXERR)\
 $(ctype__h) $(memory__h) $(string__h) $(gp_h)\
 $(gscdefs_h) $(gscoord_h) $(gsdfilt_h) $(gsfname_h) $(gsmatrix_h)\
 $(gspaint_h) $(gspath_h) $(gsstruct_h)\
 $(gxcmap_h) $(gxdevice_h) $(gxdevmem_h) $(gxiodev_h) $(gzstate_h)
	$(GLCC) $(GLO_)gsdfilt.$(OBJ) $(C_) $(GLSRC)gsdfilt.c

$(GLOBJ)gsdevmem.$(OBJ) : $(GLSRC)gsdevmem.c $(GXERR) $(math__h) $(memory__h)\
 $(gsdevice_h) $(gxarith_h) $(gxdevice_h) $(gxdevmem_h)
	$(GLCC) $(GLO_)gsdevmem.$(OBJ) $(C_) $(GLSRC)gsdevmem.c

$(GLOBJ)gsdparam.$(OBJ) : $(GLSRC)gsdparam.c $(GXERR)\
 $(memory__h) $(string__h)\
 $(gsdevice_h) $(gsparam_h) $(gxdevice_h) $(gxfixed_h)
	$(GLCC) $(GLO_)gsdparam.$(OBJ) $(C_) $(GLSRC)gsdparam.c

$(GLOBJ)gsfname.$(OBJ) : $(GLSRC)gsfname.c $(memory__h)\
 $(gserror_h) $(gserrors_h) $(gsfname_h) $(gsmemory_h) $(gstypes_h)\
 $(gxiodev_h)
	$(GLCC) $(GLO_)gsfname.$(OBJ) $(C_) $(GLSRC)gsfname.c

$(GLOBJ)gsfont.$(OBJ) : $(GLSRC)gsfont.c $(GXERR) $(memory__h)\
 $(gsstruct_h) $(gsutil_h)\
 $(gxdevice_h) $(gxfixed_h) $(gxmatrix_h) $(gxfont_h) $(gxfcache_h)\
 $(gzpath_h)\
 $(gzstate_h)
	$(GLCC) $(GLO_)gsfont.$(OBJ) $(C_) $(GLSRC)gsfont.c

$(GLOBJ)gsgdata.$(OBJ) : $(GLSRC)gsgdata.c $(GXERR) $(memory__h)\
 $(gsgdata_h) $(gsmatrix_h) $(gsstruct_h) $(gxfont_h)
	$(GLCC) $(GLO_)gsgdata.$(OBJ) $(C_) $(GLSRC)gsgdata.c

$(GLOBJ)gsgcache.$(OBJ) : $(GLSRC)gsgcache.c $(GXERR)\
 $(memory__h) $(gsstruct_h) $(gsgdata_h) $(gsgcache_h) $(gxfont_h) $(gxfont42_h)
	$(GLCC) $(GLO_)gsgcache.$(OBJ) $(C_) $(GLSRC)gsgcache.c

$(GLOBJ)gsht.$(OBJ) : $(GLSRC)gsht.c $(GXERR) $(memory__h) $(string__h)\
 $(gsstruct_h) $(gsutil_h) $(gxarith_h) $(gxdevice_h) $(gzht_h) $(gzstate_h)\
 $(gswts_h)
	$(GLCC) $(GLO_)gsht.$(OBJ) $(C_) $(GLSRC)gsht.c

$(GLOBJ)gshtscr.$(OBJ) : $(GLSRC)gshtscr.c $(GXERR) $(math__h)\
 $(gsstruct_h) $(gxarith_h) $(gxdevice_h) $(gzht_h) $(gzstate_h) $(gswts_h)
	$(GLCC) $(GLO_)gshtscr.$(OBJ) $(C_) $(GLSRC)gshtscr.c

$(GLOBJ)gsimage.$(OBJ) : $(GLSRC)gsimage.c $(GXERR) $(memory__h)\
 $(gscspace_h) $(gsimage_h) $(gsmatrix_h) $(gsstruct_h)\
 $(gxarith_h) $(gxdevice_h) $(gxiparam_h) $(gxpath_h) $(gzstate_h)
	$(GLCC) $(GLO_)gsimage.$(OBJ) $(C_) $(GLSRC)gsimage.c

$(GLOBJ)gsimpath.$(OBJ) : $(GLSRC)gsimpath.c $(GXERR)\
 $(gsmatrix_h) $(gspaint_h) $(gspath_h) $(gsstate_h)
	$(GLCC) $(GLO_)gsimpath.$(OBJ) $(C_) $(GLSRC)gsimpath.c

$(GLOBJ)gsinit.$(OBJ) : $(GLSRC)gsinit.c $(memory__h) $(stdio__h)\
 $(gdebug_h) $(gp_h) $(gscdefs_h) $(gslib_h) $(gsmalloc_h) $(gsmemory_h)
	$(GLCC) $(GLO_)gsinit.$(OBJ) $(C_) $(GLSRC)gsinit.c

$(GLOBJ)gsiodev.$(OBJ) : $(GLSRC)gsiodev.c $(GXERR)\
 $(errno__h) $(string__h) $(unistd__h)\
 $(gp_h) $(gscdefs_h) $(gsparam_h) $(gsstruct_h) $(gxiodev_h)
	$(GLCC) $(GLO_)gsiodev.$(OBJ) $(C_) $(GLSRC)gsiodev.c

$(GLOBJ)gsistate.$(OBJ) : $(GLSRC)gsistate.c $(GXERR)\
 $(gscie_h) $(gscspace_h) $(gsstruct_h) $(gsutil_h)\
 $(gxbitmap_h) $(gxcmap_h) $(gxdht_h) $(gxistate_h) $(gzht_h) $(gzline_h)
	$(GLCC) $(GLO_)gsistate.$(OBJ) $(C_) $(GLSRC)gsistate.c

$(GLOBJ)gsline.$(OBJ) : $(GLSRC)gsline.c $(GXERR) $(math__h) $(memory__h)\
 $(gscoord_h) $(gsline_h) $(gxfixed_h) $(gxmatrix_h) $(gzstate_h) $(gzline_h)
	$(GLCC) $(GLO_)gsline.$(OBJ) $(C_) $(GLSRC)gsline.c

$(GLOBJ)gsmatrix.$(OBJ) : $(GLSRC)gsmatrix.c $(GXERR) $(math__h) $(memory__h)\
 $(gxfarith_h) $(gxfixed_h) $(gxmatrix_h) $(stream_h)
	$(GLCC) $(GLO_)gsmatrix.$(OBJ) $(C_) $(GLSRC)gsmatrix.c

$(GLOBJ)gspaint.$(OBJ) : $(GLSRC)gspaint.c $(GXERR) $(math__h) $(gpcheck_h)\
 $(gspaint_h) $(gspath_h) $(gsropt_h)\
 $(gxdevmem_h) $(gxdevice_h) $(gxfixed_h) $(gxmatrix_h) $(gxpaint_h) $(gxhldevc_h)\
 $(gzcpath_h) $(gzpath_h) $(gzstate_h)
	$(GLCC) $(GLO_)gspaint.$(OBJ) $(C_) $(GLSRC)gspaint.c

$(GLOBJ)gsparam.$(OBJ) : $(GLSRC)gsparam.c $(GXERR) $(memory__h) $(string__h)\
 $(gsparam_h) $(gsstruct_h)
	$(GLCC) $(GLO_)gsparam.$(OBJ) $(C_) $(GLSRC)gsparam.c

# gsparamx is not included in the base configuration.
$(GLOBJ)gsparamx.$(OBJ) : $(GLSRC)gsparamx.c $(string__h)\
 $(gserror_h) $(gserrors_h) $(gsmemory_h) $(gsparam_h) $(gsparamx_h)\
 $(gstypes_h)
	$(GLCC) $(GLO_)gsparamx.$(OBJ) $(C_) $(GLSRC)gsparamx.c

# Future replacement for gsparams.c
$(GLOBJ)gsparam2.$(OBJ) : $(GLSRC)gsparam2.c $(GXERR) $(memory__h)\
 $(gsparams_h)
	$(GLCC) $(GLO_)gsparam2.$(OBJ) $(C_) $(GLSRC)gsparam2.c

$(GLOBJ)gsparams.$(OBJ) : $(GLSRC)gsparams.c $(GXERR) $(memory__h)\
 $(gsparams_h)
	$(GLCC) $(GLO_)gsparams.$(OBJ) $(C_) $(GLSRC)gsparams.c

$(GLOBJ)gspath.$(OBJ) : $(GLSRC)gspath.c $(GXERR)\
 $(math__h) $(gscoord_h) $(gspath_h)\
 $(gxdevice_h) $(gxdevmem_h) $(gxfixed_h) $(gxmatrix_h)\
 $(gzcpath_h) $(gzpath_h) $(gzstate_h)
	$(GLCC) $(GLO_)gspath.$(OBJ) $(C_) $(GLSRC)gspath.c

$(GLOBJ)gsstate.$(OBJ) : $(GLSRC)gsstate.c $(GXERR) $(memory__h)\
 $(gsstruct_h) $(gsutil_h) $(gzstate_h) $(gxcspace_h)\
 $(gsalpha_h) $(gscolor2_h) $(gscoord_h) $(gscie_h)\
 $(gxclipsr_h) $(gxcmap_h) $(gxdevice_h) $(gxpcache_h)\
 $(gzht_h) $(gzline_h) $(gspath_h) $(gzpath_h) $(gzcpath_h)\
 $(gsovrc_h) $(gxcolor2_h) $(gxpcolor_h)
	$(GLCC) $(GLO_)gsstate.$(OBJ) $(C_) $(GLSRC)gsstate.c

$(GLOBJ)gstext.$(OBJ) : $(GLSRC)gstext.c $(memory__h) $(gdebug_h)\
 $(gserror_h) $(gserrors_h) $(gsmemory_h) $(gsstruct_h) $(gstypes_h) $(gxfcache_h)\
 $(gxdcolor_h) $(gxdevcli_h) $(gxfont_h) $(gxpath_h) $(gxtext_h) $(gzstate_h)
	$(GLCC) $(GLO_)gstext.$(OBJ) $(C_) $(GLSRC)gstext.c

# We make gsiodevs a separate module so the PS interpreter can replace it.

$(GLD)gsiodevs.dev : $(ECHOGS_XE) $(LIB_MAK) $(GLOBJ)gsiodevs.$(OBJ)\
 $(GLD)sfile.dev
	$(SETMOD) $(GLD)gsiodevs $(GLOBJ)gsiodevs.$(OBJ)
	$(ADDMOD) $(GLD)gsiodevs -include $(GLD)sfile
	$(ADDMOD) $(GLD)gsiodevs -iodev stdin stdout stderr

$(GLOBJ)gsiodevs.$(OBJ) : $(GLSRC)gsiodevs.c $(GXERR)\
 $(gxiodev_h) $(stream_h) $(strimpl_h)
	$(GLCC) $(GLO_)gsiodevs.$(OBJ) $(C_) $(GLSRC)gsiodevs.c

###### Internal devices

### Memory devices

$(GLOBJ)gdevmem.$(OBJ) : $(GLSRC)gdevmem.c $(GXERR) $(memory__h)\
 $(gsrect_h) $(gsstruct_h) $(gstrans_h)\
 $(gxarith_h) $(gxgetbit_h) $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevmem.$(OBJ) $(C_) $(GLSRC)gdevmem.c

$(GLOBJ)gdevm1.$(OBJ) : $(GLSRC)gdevm1.c $(GX) $(memory__h) $(gsrop_h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevm1.$(OBJ) $(C_) $(GLSRC)gdevm1.c

$(GLOBJ)gdevm2.$(OBJ) : $(GLSRC)gdevm2.c $(GX) $(memory__h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevm2.$(OBJ) $(C_) $(GLSRC)gdevm2.c

$(GLOBJ)gdevm4.$(OBJ) : $(GLSRC)gdevm4.c $(GX) $(memory__h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevm4.$(OBJ) $(C_) $(GLSRC)gdevm4.c

$(GLOBJ)gdevm8.$(OBJ) : $(GLSRC)gdevm8.c $(GX) $(memory__h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevm8.$(OBJ) $(C_) $(GLSRC)gdevm8.c

$(GLOBJ)gdevm16.$(OBJ) : $(GLSRC)gdevm16.c $(GX) $(memory__h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevm16.$(OBJ) $(C_) $(GLSRC)gdevm16.c

$(GLOBJ)gdevm24.$(OBJ) : $(GLSRC)gdevm24.c $(GX) $(memory__h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevm24.$(OBJ) $(C_) $(GLSRC)gdevm24.c

$(GLOBJ)gdevm32.$(OBJ) : $(GLSRC)gdevm32.c $(GX) $(memory__h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevm32.$(OBJ) $(C_) $(GLSRC)gdevm32.c

$(GLOBJ)gdevm40.$(OBJ) : $(GLSRC)gdevm40.c $(GX) $(memory__h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevm40.$(OBJ) $(C_) $(GLSRC)gdevm40.c

$(GLOBJ)gdevm48.$(OBJ) : $(GLSRC)gdevm48.c $(GX) $(memory__h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevm48.$(OBJ) $(C_) $(GLSRC)gdevm48.c

$(GLOBJ)gdevm56.$(OBJ) : $(GLSRC)gdevm56.c $(GX) $(memory__h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevm56.$(OBJ) $(C_) $(GLSRC)gdevm56.c

$(GLOBJ)gdevm64.$(OBJ) : $(GLSRC)gdevm64.c $(GX) $(memory__h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevm64.$(OBJ) $(C_) $(GLSRC)gdevm64.c

$(GLOBJ)gdevmpla.$(OBJ) : $(GLSRC)gdevmpla.c $(GXERR) $(memory__h)\
 $(gsbitops_h)\
 $(gxdevice_h) $(gxdevmem_h) $(gxgetbit_h) $(gdevmem_h) $(gdevmpla_h)
	$(GLCC) $(GLO_)gdevmpla.$(OBJ) $(C_) $(GLSRC)gdevmpla.c

### Alpha-channel devices

$(GLOBJ)gdevabuf.$(OBJ) : $(GLSRC)gdevabuf.c $(GXERR) $(memory__h)\
 $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)
	$(GLCC) $(GLO_)gdevabuf.$(OBJ) $(C_) $(GLSRC)gdevabuf.c

### Other built-in devices

# The bbox device can either be used as forwarding device to support
# graphics functions, or it can be a real target device. We create
# the bboxutil.dev pseudo device to allow inclusion without putting
# the bbox device on the list of devices.

$(GLD)bboxutil.dev : $(ECHOGS_XE) $(LIB_MAK) $(GLOBJ)gdevbbox.$(OBJ)
	$(ADDMOD) $(GLD)bboxutil $(GLOBJ)gdevbbox.$(OBJ)

$(GLD)bbox.dev : $(ECHOGS_XE) $(LIB_MAK) $(GLOBJ)gdevbbox.$(OBJ)
	$(SETDEV2) $(GLD)bbox $(GLOBJ)gdevbbox.$(OBJ)

$(GLOBJ)gdevbbox.$(OBJ) : $(GLSRC)gdevbbox.c $(GXERR) $(math__h) $(memory__h)\
 $(gdevbbox_h) $(gsdevice_h) $(gsparam_h)\
 $(gxcpath_h) $(gxdcolor_h) $(gxdevice_h) $(gxiparam_h) $(gxistate_h)\
 $(gxpaint_h) $(gxpath_h)
	$(GLCC) $(GLO_)gdevbbox.$(OBJ) $(C_) $(GLSRC)gdevbbox.c

$(GLOBJ)gdevhit.$(OBJ) : $(GLSRC)gdevhit.c $(std_h)\
  $(gserror_h) $(gserrors_h) $(gsmemory_h) $(gstypes_h) $(gxdevice_h)
	$(GLCC) $(GLO_)gdevhit.$(OBJ) $(C_) $(GLSRC)gdevhit.c

# Define a device that implements the PCL 5 special color mapping
# algorithms.  This is not included in any PostScript or PDF system.

$(GLD)devcmap.dev : $(LIB_MAK) $(ECHOGS_XE) $(GLOBJ)gdevcmap.$(OBJ)
	$(SETMOD) $(GLD)devcmap $(GLOBJ)gdevcmap.$(OBJ)

gdevcmap_h=$(GLSRC)gdevcmap.h

$(GLOBJ)gdevcmap.$(OBJ) : $(GLSRC)gdevcmap.c $(GXERR)\
 $(gxdcconv_h) $(gxdevice_h) $(gxfrac_h) $(gxlum_h) $(gdevcmap_h)
	$(GLCC) $(GLO_)gdevcmap.$(OBJ) $(C_) $(GLSRC)gdevcmap.c

# A device that stores its data using run-length encoding.

$(GLOBJ)gdevmrun.$(OBJ) : $(GLSRC)gdevmrun.c $(GXERR) $(memory__h)\
 $(gxdevice_h) $(gdevmrun_h)
	$(GLCC) $(GLO_)gdevmrun.$(OBJ) $(C_) $(GLSRC)gdevmrun.c

# A device that extracts a single plane from multi-plane color.

$(GLOBJ)gdevplnx.$(OBJ) : $(GLSRC)gdevplnx.c $(GXERR)\
 $(gsbitops_h) $(gsrop_h) $(gsstruct_h) $(gsutil_h)\
 $(gdevplnx_h)\
 $(gxcmap_h) $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h) $(gxdither_h)\
 $(gxgetbit_h) $(gxiparam_h) $(gxistate_h)
	$(GLCC) $(GLO_)gdevplnx.$(OBJ) $(C_) $(GLSRC)gdevplnx.c

# A tracing device, also an example of a high-level device.

$(GLOBJ)gdevtrac.$(OBJ) : $(GLSRC)gdevtrac.c $(GXERR)\
 $(gscspace_h)\
 $(gxdevice_h) $(gxdht_h) $(gxfont_h) $(gxiparam_h) $(gxistate_h)\
 $(gxpaint_h) $(gxtmap_h) $(gzcpath_h) $(gzpath_h) 
	$(GLCC) $(GLO_)gdevtrac.$(OBJ) $(C_) $(GLSRC)gdevtrac.c

$(GLD)tracedev.dev : $(ECHOGS_XE) $(LIB_MAK) $(GLOBJ)gdevtrac.$(OBJ)
	$(SETMOD) $(GLD)tracedev -dev2 tr_mono tr_rgb tr_cmyk
	$(ADDMOD) $(GLD)tracedev -obj $(GLOBJ)gdevtrac.$(OBJ)

### Default driver procedure implementations

$(GLOBJ)gdevdbit.$(OBJ) : $(GLSRC)gdevdbit.c $(GXERR) $(gpcheck_h)\
 $(gdevmem_h) $(gsbittab_h) $(gsrect_h) $(gsropt_h)\
 $(gxcpath_h) $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h)
	$(GLCC) $(GLO_)gdevdbit.$(OBJ) $(C_) $(GLSRC)gdevdbit.c

$(GLOBJ)gdevddrw.$(OBJ) : $(GLSRC)gdevddrw.c $(GXERR) $(math__h) $(memory__h) $(stdint__h)\
 $(gpcheck_h)\
 $(gsrect_h)\
 $(gxdcolor_h) $(gxdevice_h) $(gxfixed_h) $(gxiparam_h) $(gxistate_h) $(gxmatrix_h)\
 $(gdevddrw_h) $(gxdtfill_h) $(vtrace_h)
	$(GLCC) $(GLO_)gdevddrw.$(OBJ) $(C_) $(GLSRC)gdevddrw.c

$(GLOBJ)gdevdsha.$(OBJ) : $(GLSRC)gdevdsha.c $(GXERR) $(gserrors_h)\
 $(gxdevice_h) $(gxcindex_h) $(vdtrace_h)
	$(GLCC) $(GLO_)gdevdsha.$(OBJ) $(C_) $(GLSRC)gdevdsha.c

$(GLOBJ)gdevdflt.$(OBJ) : $(GLSRC)gdevdflt.c $(GXERR)\
 $(gsropt_h) $(gxcomp_h) $(gxdevice_h)
	$(GLCC) $(GLO_)gdevdflt.$(OBJ) $(C_) $(GLSRC)gdevdflt.c

$(GLOBJ)gdevdgbr.$(OBJ) : $(GLSRC)gdevdgbr.c $(GXERR) $(memory__h)\
 $(gdevmem_h) $(gxdevice_h) $(gxdevmem_h) $(gxgetbit_h) $(gxlum_h)
	$(GLCC) $(GLO_)gdevdgbr.$(OBJ) $(C_) $(GLSRC)gdevdgbr.c

$(GLOBJ)gdevnfwd.$(OBJ) : $(GLSRC)gdevnfwd.c $(GXERR)\
 $(gxdevice_h) $(gxcmap_h) $(memory__h)
	$(GLCC) $(GLO_)gdevnfwd.$(OBJ) $(C_) $(GLSRC)gdevnfwd.c

### Other device support

# Provide a mapping between StandardEncoding and ISOLatin1Encoding.
$(GLOBJ)gdevemap.$(OBJ) : $(GLSRC)gdevemap.c $(AK) $(std_h)
	$(GLCC) $(GLO_)gdevemap.$(OBJ) $(C_) $(GLSRC)gdevemap.c

###### Create a pseudo-"feature" for the entire graphics library.

LIB0s=$(GLOBJ)gpmisc.$(OBJ) $(GLOBJ)stream.$(OBJ)
LIB1s=$(GLOBJ)gsalloc.$(OBJ) $(GLOBJ)gsalpha.$(OBJ)
LIB2s=$(GLOBJ)gsbitcom.$(OBJ) $(GLOBJ)gsbitops.$(OBJ) $(GLOBJ)gsbittab.$(OBJ)
# Note: gschar.c is no longer required for a standard build;
# we include it only for backward compatibility for library clients.
LIB3s=$(GLOBJ)gscedata.$(OBJ) $(GLOBJ)gscencs.$(OBJ) $(GLOBJ)gschar.$(OBJ) $(GLOBJ)gscolor.$(OBJ)
LIB4s= $(GLOBJ)gscoord.$(OBJ) $(GLOBJ)gscparam.$(OBJ) $(GLOBJ)gscspace.$(OBJ) $(GLOBJ)gsovrc.$(OBJ) $(GLOBJ)gxoprect.$(OBJ)
LIB5s=$(GLOBJ)gsdevice.$(OBJ) $(GLOBJ)gsdevmem.$(OBJ) $(GLOBJ)gsdparam.$(OBJ) $(GLOBJ)gsdfilt.$(OBJ)
LIB6s=$(GLOBJ)gsfname.$(OBJ) $(GLOBJ)gsfont.$(OBJ) $(GLOBJ)gsgdata.$(OBJ) $(GLOBJ)gsgcache.$(OBJ)
LIB7s=$(GLOBJ)gsht.$(OBJ) $(GLOBJ)gshtscr.$(OBJ) $(GLOBJ)gswts.$(OBJ)
LIB8s=$(GLOBJ)gsimage.$(OBJ) $(GLOBJ)gsimpath.$(OBJ) $(GLOBJ)gsinit.$(OBJ)
LIB9s=$(GLOBJ)gsiodev.$(OBJ) $(GLOBJ)gsistate.$(OBJ) $(GLOBJ)gsline.$(OBJ)
LIB10s=$(GLOBJ)gsmalloc.$(OBJ) $(GLOBJ)gsmatrix.$(OBJ) $(GLOBJ)gsmemlok.$(OBJ)
LIB11s=$(GLOBJ)gsmemory.$(OBJ) $(GLOBJ)gsmemret.$(OBJ) $(GLOBJ)gsmisc.$(OBJ) $(GLOBJ)gsnotify.$(OBJ) $(GLOBJ)gslibctx.$(OBJ) 
LIB12s=$(GLOBJ)gspaint.$(OBJ) $(GLOBJ)gsparam.$(OBJ) $(GLOBJ)gspath.$(OBJ)
LIB13s=$(GLOBJ)gsserial.$(OBJ) $(GLOBJ)gsstate.$(OBJ) $(GLOBJ)gstext.$(OBJ)\
  $(GLOBJ)gsutil.$(OBJ)
LIB1x=$(GLOBJ)gxacpath.$(OBJ) $(GLOBJ)gxbcache.$(OBJ) $(GLOBJ)gxccache.$(OBJ)
LIB2x=$(GLOBJ)gxccman.$(OBJ) $(GLOBJ)gxchar.$(OBJ) $(GLOBJ)gxcht.$(OBJ)
LIB3x=$(GLOBJ)gxclip.$(OBJ) $(GLOBJ)gxcmap.$(OBJ) $(GLOBJ)gxcpath.$(OBJ)
LIB4x=$(GLOBJ)gxdcconv.$(OBJ) $(GLOBJ)gxdcolor.$(OBJ) $(GLOBJ)gxhldevc.$(OBJ)
LIB5x=$(GLOBJ)gxfill.$(OBJ) $(GLOBJ)gxfdrop.$(OBJ) $(GLOBJ)gxht.$(OBJ) $(GLOBJ)gxhtbit.$(OBJ)
LIB6x=$(GLOBJ)gxwts.$(OBJ) $(GLOBJ)gxidata.$(OBJ) $(GLOBJ)gxifast.$(OBJ) $(GLOBJ)gximage.$(OBJ)
LIB7x=$(GLOBJ)gximage1.$(OBJ) $(GLOBJ)gximono.$(OBJ) $(GLOBJ)gxipixel.$(OBJ)
LIB8x=$(GLOBJ)gxpaint.$(OBJ) $(GLOBJ)gxpath.$(OBJ) $(GLOBJ)gxpath2.$(OBJ)
LIB9x=$(GLOBJ)gxpcopy.$(OBJ) $(GLOBJ)gxpdash.$(OBJ) $(GLOBJ)gxpflat.$(OBJ)
LIB10x=$(GLOBJ)gxsample.$(OBJ) $(GLOBJ)gxstroke.$(OBJ) $(GLOBJ)gxsync.$(OBJ) $(GLOBJ)vdtrace.$(OBJ)
LIB1d=$(GLOBJ)gdevabuf.$(OBJ) $(GLOBJ)gdevdbit.$(OBJ) $(GLOBJ)gdevddrw.$(OBJ) $(GLOBJ)gdevdflt.$(OBJ)
LIB2d=$(GLOBJ)gdevdgbr.$(OBJ) $(GLOBJ)gdevnfwd.$(OBJ) $(GLOBJ)gdevmem.$(OBJ) $(GLOBJ)gdevplnx.$(OBJ)
LIB3d=$(GLOBJ)gdevm1.$(OBJ) $(GLOBJ)gdevm2.$(OBJ) $(GLOBJ)gdevm4.$(OBJ) $(GLOBJ)gdevm8.$(OBJ)
LIB4d=$(GLOBJ)gdevm16.$(OBJ) $(GLOBJ)gdevm24.$(OBJ) $(GLOBJ)gdevm32.$(OBJ) $(GLOBJ)gdevmpla.$(OBJ)
LIB5d=$(GLOBJ)gdevm40.$(OBJ) $(GLOBJ)gdevm48.$(OBJ) $(GLOBJ)gdevm56.$(OBJ) $(GLOBJ)gdevm64.$(OBJ)
LIB6d=$(GLOBJ)gdevdsha.$(OBJ)
LIBs=$(LIB0s) $(LIB1s) $(LIB2s) $(LIB3s) $(LIB4s) $(LIB5s) $(LIB6s) $(LIB7s)\
 $(LIB8s) $(LIB9s) $(LIB10s) $(LIB11s) $(LIB12s) $(LIB13s)
LIBx=$(LIB1x) $(LIB2x) $(LIB3x) $(LIB4x) $(LIB5x) $(LIB6x) $(LIB7x) $(LIB8x) $(LIB9x) $(LIB10x)
LIBd=$(LIB1d) $(LIB2d) $(LIB3d) $(LIB4d) $(LIB5d) $(LIB6d)
LIB_ALL=$(LIBs) $(LIBx) $(LIBd)
# We include some optional library modules in the dependency list,
# but not in the link, to catch compilation problems.
LIB_O=$(GLOBJ)gdevmpla.$(OBJ) $(GLOBJ)gdevmrun.$(OBJ) $(GLOBJ)gshtx.$(OBJ) $(GLOBJ)gsnogc.$(OBJ)
$(GLD)libs.dev : $(LIB_MAK) $(ECHOGS_XE) $(LIBs) $(LIB_O) $(GLD)gsiodevs.dev
	$(SETMOD) $(GLD)libs $(LIB0s)
	$(ADDMOD) $(GLD)libs $(LIB1s)
	$(ADDMOD) $(GLD)libs $(LIB2s)
	$(ADDMOD) $(GLD)libs $(LIB3s)
	$(ADDMOD) $(GLD)libs $(LIB4s)
	$(ADDMOD) $(GLD)libs $(LIB5s)
	$(ADDMOD) $(GLD)libs $(LIB6s)
	$(ADDMOD) $(GLD)libs $(LIB7s)
	$(ADDMOD) $(GLD)libs $(LIB8s)
	$(ADDMOD) $(GLD)libs $(LIB9s)
	$(ADDMOD) $(GLD)libs $(LIB10s)
	$(ADDMOD) $(GLD)libs $(LIB11s)
	$(ADDMOD) $(GLD)libs $(LIB12s)
	$(ADDMOD) $(GLD)libs $(LIB13s)
	$(ADDCOMP) $(GLD)libs overprint
	$(ADDCOMP) $(GLD)libs pdf14trans
	$(ADDMOD) $(GLD)libs -init gshtscr
	$(ADDMOD) $(GLD)libs -include $(GLD)gsiodevs

$(GLD)libx.dev : $(LIB_MAK) $(ECHOGS_XE) $(LIBx)
	$(SETMOD) $(GLD)libx $(LIB1x)
	$(ADDMOD) $(GLD)libx $(LIB2x)
	$(ADDMOD) $(GLD)libx $(LIB3x)
	$(ADDMOD) $(GLD)libx $(LIB4x)
	$(ADDMOD) $(GLD)libx $(LIB5x)
	$(ADDMOD) $(GLD)libx $(LIB6x)
	$(ADDMOD) $(GLD)libx $(LIB7x)
	$(ADDMOD) $(GLD)libx $(LIB8x)
	$(ADDMOD) $(GLD)libx $(LIB9x)
	$(ADDMOD) $(GLD)libx $(LIB10x)
	$(ADDMOD) $(GLD)libx -imageclass 1_simple 3_mono
	$(ADDMOD) $(GLD)libx -imagetype 1 mask1

$(GLD)libd.dev : $(LIB_MAK) $(ECHOGS_XE) $(LIBd)
	$(SETMOD) $(GLD)libd $(LIB1d)
	$(ADDMOD) $(GLD)libd $(LIB2d)
	$(ADDMOD) $(GLD)libd $(LIB3d)
	$(ADDMOD) $(GLD)libd $(LIB4d)
	$(ADDMOD) $(GLD)libd $(LIB5d)
	$(ADDMOD) $(GLD)libd $(LIB6d)

$(GLD)libcore.dev : $(LIB_MAK) $(ECHOGS_XE)\
 $(GLD)libs.dev $(GLD)libx.dev $(GLD)libd.dev\
 $(GLD)iscale.dev $(GLD)no16bit.dev $(GLD)no12bit.dev $(GLD)noroplib.dev $(GLD)strdline.dev
	$(SETMOD) $(GLD)libcore
	$(ADDMOD) $(GLD)libcore -dev2 nullpage
	$(ADDMOD) $(GLD)libcore -include $(GLD)libs $(GLD)libx $(GLD)libd
	$(ADDMOD) $(GLD)libcore -include $(GLD)iscale $(GLD)no16bit $(GLD)no12bit $(GLD)noroplib
	$(ADDMOD) $(GLD)libcore -include $(GLD)strdline

# ---------------- Stream support ---------------- #
# Currently the only things in the library that use this are clists
# and file streams.

$(GLOBJ)stream.$(OBJ) : $(GLSRC)stream.c $(AK) $(stdio__h) $(memory__h)\
 $(gdebug_h) $(gpcheck_h) $(stream_h) $(strimpl_h)
	$(GLCC) $(GLO_)stream.$(OBJ) $(C_) $(GLSRC)stream.c

# Default, stream-based readline.
strdline_=$(GLOBJ)gp_strdl.$(OBJ)
$(GLD)strdline.dev : $(LIB_MAK) $(ECHOGS_XE) $(strdline_)
	$(SETMOD) $(GLD)strdline $(strdline_)

$(GLOBJ)gp_strdl.$(OBJ) : $(GLSRC)gp_strdl.c $(AK) $(std_h) $(gp_h)\
 $(gsmemory_h) $(gstypes_h)
	$(GLCC) $(GLO_)gp_strdl.$(OBJ) $(C_) $(GLSRC)gp_strdl.c

# ---------------- File streams ---------------- #
# Currently only the high-level drivers use these, but more drivers will
# probably use them eventually.

sfile_=$(GLOBJ)sfx$(FILE_IMPLEMENTATION).$(OBJ) $(GLOBJ)stream.$(OBJ)
$(GLD)sfile.dev : $(LIB_MAK) $(ECHOGS_XE) $(sfile_)
	$(SETMOD) $(GLD)sfile $(sfile_)

$(GLOBJ)sfxstdio.$(OBJ) : $(GLSRC)sfxstdio.c $(AK) $(stdio__h) $(memory__h)\
 $(gdebug_h) $(gpcheck_h) $(stream_h) $(strimpl_h)
	$(GLCC) $(GLO_)sfxstdio.$(OBJ) $(C_) $(GLSRC)sfxstdio.c

$(GLOBJ)sfxfd.$(OBJ) : $(GLSRC)sfxfd.c $(AK)\
 $(stdio__h) $(errno__h) $(memory__h) $(unistd__h)\
 $(gdebug_h) $(gpcheck_h) $(stream_h) $(strimpl_h)
	$(GLCC) $(GLO_)sfxfd.$(OBJ) $(C_) $(GLSRC)sfxfd.c

$(GLOBJ)sfxboth.$(OBJ) : $(GLSRC)sfxboth.c $(GLSRC)sfxstdio.c $(GLSRC)sfxfd.c
	$(GLCC) $(GLO_)sfxboth.$(OBJ) $(C_) $(GLSRC)sfxboth.c

# ---------------- BCP filters ---------------- #

$(GLOBJ)sbcp.$(OBJ) : $(GLSRC)sbcp.c $(AK) $(stdio__h)\
 $(sbcp_h) $(strimpl_h)
	$(GLCC) $(GLO_)sbcp.$(OBJ) $(C_) $(GLSRC)sbcp.c

# ---------------- CCITTFax filters ---------------- #
# These are used by clists, some drivers, and Level 2 in general.

cfe_=$(GLOBJ)scfe.$(OBJ) $(GLOBJ)scfetab.$(OBJ) $(GLOBJ)shc.$(OBJ)
$(GLD)cfe.dev : $(LIB_MAK) $(ECHOGS_XE) $(cfe_)
	$(SETMOD) $(GLD)cfe $(cfe_)

$(GLOBJ)scfe.$(OBJ) : $(GLSRC)scfe.c $(AK) $(memory__h) $(stdio__h) $(gdebug_h)\
 $(scf_h) $(strimpl_h) $(scfx_h)
	$(GLCC) $(GLO_)scfe.$(OBJ) $(C_) $(GLSRC)scfe.c

$(GLOBJ)scfetab.$(OBJ) : $(GLSRC)scfetab.c $(AK) $(std_h) $(scommon_h) $(scf_h)
	$(GLCC) $(GLO_)scfetab.$(OBJ) $(C_) $(GLSRC)scfetab.c

$(GLOBJ)shc.$(OBJ) : $(GLSRC)shc.c $(AK) $(std_h) $(scommon_h) $(shc_h)
	$(GLCC) $(GLO_)shc.$(OBJ) $(C_) $(GLSRC)shc.c

cfd_=$(GLOBJ)scfd.$(OBJ) $(GLOBJ)scfdtab.$(OBJ)
$(GLD)cfd.dev : $(LIB_MAK) $(ECHOGS_XE) $(cfd_)
	$(SETMOD) $(GLD)cfd $(cfd_)

$(GLOBJ)scfd.$(OBJ) : $(GLSRC)scfd.c $(AK) $(memory__h) $(stdio__h) $(gdebug_h)\
 $(scf_h) $(strimpl_h) $(scfx_h)
	$(GLCC) $(GLO_)scfd.$(OBJ) $(C_) $(GLSRC)scfd.c

$(GLOBJ)scfdtab.$(OBJ) : $(GLSRC)scfdtab.c $(AK) $(std_h) $(scommon_h) $(scf_h)
	$(GLCC) $(GLO_)scfdtab.$(OBJ) $(C_) $(GLSRC)scfdtab.c

# scfparam is used by the filter operator and the PS/PDF writer.
# It is not included automatically in cfe or cfd.
$(GLOBJ)scfparam.$(OBJ) : $(GLSRC)scfparam.c $(AK) $(std_h)\
 $(gserror_h) $(gserrors_h) $(gsmemory_h) $(gsparam_h) $(gstypes_h)\
 $(scommon_h) $(scf_h) $(scfx_h)
	$(GLCC) $(GLO_)scfparam.$(OBJ) $(C_) $(GLSRC)scfparam.c

# ---------------- DCT (JPEG) filters ---------------- #
# These are used by Level 2, and by the JPEG-writing driver.

# Common code

sdcparam_h=$(GLSRC)sdcparam.h

sdctc_=$(GLOBJ)sdctc.$(OBJ) $(GLOBJ)sjpegc.$(OBJ)

$(GLOBJ)sdctc.$(OBJ) : $(GLSRC)sdctc.c $(AK) $(stdio__h) $(jpeglib__h)\
 $(gsmalloc_h) $(gsmemory_h) $(sdct_h) $(strimpl_h)
	$(GLCC) $(GLO_)sdctc.$(OBJ) $(C_) $(GLSRC)sdctc.c

$(GLOBJ)sjpegc.$(OBJ) : $(GLSRC)sjpegc.c $(AK) $(stdio__h) $(string__h) $(gx_h)\
 $(jerror__h) $(jpeglib__h)\
 $(gserrors_h) $(sjpeg_h) $(sdct_h) $(strimpl_h)
	$(GLJCC) $(GLO_)sjpegc.$(OBJ) $(C_) $(GLSRC)sjpegc.c

# sdcparam is used by the filter operator and the PS/PDF writer.
# It is not included automatically in sdcte/d.
$(GLOBJ)sdcparam.$(OBJ) : $(GLSRC)sdcparam.c $(AK) $(memory__h)\
 $(jpeglib__h)\
 $(gserror_h) $(gserrors_h) $(gsmemory_h) $(gsparam_h) $(gstypes_h)\
 $(sdcparam_h) $(sdct_h) $(sjpeg_h) $(strimpl_h)
	$(GLCC) $(GLO_)sdcparam.$(OBJ) $(C_) $(GLSRC)sdcparam.c

# Encoding (compression)

sdcte_=$(sdctc_) $(GLOBJ)sdcte.$(OBJ) $(GLOBJ)sjpege.$(OBJ)
$(GLD)sdcte.dev : $(LIB_MAK) $(ECHOGS_XE) $(sdcte_) $(JGENDIR)$(D)jpege.dev
	$(SETMOD) $(GLD)sdcte $(sdcte_)
	$(ADDMOD) $(GLD)sdcte -include $(JGENDIR)$(D)jpege.dev

$(GLOBJ)sdcte.$(OBJ) : $(GLSRC)sdcte.c $(AK)\
 $(memory__h) $(stdio__h) $(gdebug_h) $(gsmalloc_h) $(gsmemory_h)\
 $(jerror__h) $(jpeglib__h)\
 $(sdct_h) $(sjpeg_h) $(strimpl_h)
	$(GLJCC) $(GLO_)sdcte.$(OBJ) $(C_) $(GLSRC)sdcte.c

$(GLOBJ)sjpege.$(OBJ) : $(GLSRC)sjpege.c $(AK)\
 $(stdio__h) $(string__h) $(gx_h)\
 $(jerror__h) $(jpeglib__h)\
 $(gserrors_h) $(sjpeg_h) $(sdct_h) $(strimpl_h)
	$(GLJCC) $(GLO_)sjpege.$(OBJ) $(C_) $(GLSRC)sjpege.c

# sdeparam is used by the filter operator and the PS/PDF writer.
# It is not included automatically in sdcte.
sdeparam_=$(GLOBJ)sdeparam.$(OBJ) $(GLOBJ)sdcparam.$(OBJ)
$(GLD)sdeparam.dev : $(LIB_MAK) $(ECHOGS_XE) $(sdeparam_)
	$(SETMOD) $(GLD)sdeparam $(sdeparam_)

$(GLOBJ)sdeparam.$(OBJ) : $(GLSRC)sdeparam.c $(AK) $(memory__h)\
 $(jpeglib__h)\
 $(gserror_h) $(gserrors_h) $(gsmemory_h) $(gsparam_h) $(gstypes_h)\
 $(sdcparam_h) $(sdct_h) $(sjpeg_h) $(strimpl_h)
	$(GLCC) $(GLO_)sdeparam.$(OBJ) $(C_) $(GLSRC)sdeparam.c

# Decoding (decompression)

sdctd_=$(sdctc_) $(GLOBJ)sdctd.$(OBJ) $(GLOBJ)sjpegd.$(OBJ)
$(GLD)sdctd.dev : $(LIB_MAK) $(ECHOGS_XE) $(sdctd_) $(JGENDIR)$(D)jpegd.dev
	$(SETMOD) $(GLD)sdctd $(sdctd_)
	$(ADDMOD) $(GLD)sdctd -include $(JGENDIR)$(D)jpegd.dev

$(GLOBJ)sdctd.$(OBJ) : $(GLSRC)sdctd.c $(AK)\
 $(memory__h) $(stdio__h) $(gdebug_h) $(gsmalloc_h) $(gsmemory_h)\
 $(jerror__h) $(jpeglib__h)\
 $(sdct_h) $(sjpeg_h) $(strimpl_h)
	$(GLJCC) $(GLO_)sdctd.$(OBJ) $(C_) $(GLSRC)sdctd.c

$(GLOBJ)sjpegd.$(OBJ) : $(GLSRC)sjpegd.c $(AK)\
 $(stdio__h) $(string__h) $(gx_h)\
 $(jerror__h) $(jpeglib__h)\
 $(gserrors_h) $(sjpeg_h) $(sdct_h) $(strimpl_h)
	$(GLJCC) $(GLO_)sjpegd.$(OBJ) $(C_) $(GLSRC)sjpegd.c

# sddparam is used by the filter operator.
# It is not included automatically in sdctd.
sddparam_=$(GLOBJ)sddparam.$(OBJ) $(GLOBJ)sdcparam.$(OBJ)
$(GLD)sddparam.dev : $(LIB_MAK) $(ECHOGS_XE) $(sddparam_)
	$(SETMOD) $(GLD)sddparam $(sddparam_)

$(GLOBJ)sddparam.$(OBJ) : $(GLSRC)sddparam.c $(AK) $(std_h)\
 $(jpeglib__h)\
 $(gserror_h) $(gserrors_h) $(gsmemory_h) $(gsparam_h) $(gstypes_h)\
 $(sdcparam_h) $(sdct_h) $(sjpeg_h) $(strimpl_h)
	$(GLCC) $(GLO_)sddparam.$(OBJ) $(C_) $(GLSRC)sddparam.c

# ---------------- LZW filters ---------------- #
# These are used by Level 2 in general.

lzwe_=$(GLOBJ)slzwe.$(OBJ) $(GLOBJ)slzwc.$(OBJ)
$(GLD)lzwe.dev : $(LIB_MAK) $(ECHOGS_XE) $(lzwe_)
	$(SETMOD) $(GLD)lzwe $(lzwe_)

# We need slzwe.dev as a synonym for lzwe.dev for BAND_LIST_STORAGE = memory.
$(GLD)slzwe.dev : $(GLD)lzwe.dev
	$(CP_) $(GLD)lzwe.dev $(GLD)slzwe.dev

$(GLOBJ)slzwe.$(OBJ) : $(GLSRC)slzwe.c $(AK) $(stdio__h) $(gdebug_h)\
 $(slzwx_h) $(strimpl_h)
	$(GLCC) $(GLO_)slzwe.$(OBJ) $(C_) $(GLSRC)slzwe.c

$(GLOBJ)slzwc.$(OBJ) : $(GLSRC)slzwc.c $(AK) $(std_h)\
 $(slzwx_h) $(strimpl_h)
	$(GLCC) $(GLO_)slzwc.$(OBJ) $(C_) $(GLSRC)slzwc.c

lzwd_=$(GLOBJ)slzwd.$(OBJ) $(GLOBJ)slzwc.$(OBJ)
$(GLD)lzwd.dev : $(LIB_MAK) $(ECHOGS_XE) $(lzwd_)
	$(SETMOD) $(GLD)lzwd $(lzwd_)

# We need slzwd.dev as a synonym for lzwd.dev for BAND_LIST_STORAGE = memory.
$(GLD)slzwd.dev : $(GLD)lzwd.dev
	$(CP_) $(GLD)lzwd.dev $(GLD)slzwd.dev

$(GLOBJ)slzwd.$(OBJ) : $(GLSRC)slzwd.c $(AK) $(stdio__h) $(gdebug_h)\
 $(slzwx_h) $(strimpl_h)
	$(GLCC) $(GLO_)slzwd.$(OBJ) $(C_) $(GLSRC)slzwd.c

# ---------------- MD5 digest filter ---------------- #

smd5_=$(GLOBJ)smd5.$(OBJ)
$(GLD)smd5.dev : $(LIB_MAK) $(ECHOGS_XE) $(smd5_) $(md5_)
	$(SETMOD) $(GLD)smd5 $(smd5_) $(md5_)

$(GLOBJ)smd5.$(OBJ) : $(GLSRC)smd5.c $(AK) $(memory__h)\
 $(smd5_h) $(strimpl_h) $(stream_h)
	$(GLCC) $(GLO_)smd5.$(OBJ) $(C_) $(GLSRC)smd5.c

# -------------- Arcfour cipher filter --------------- #

sarc4_=$(GLOBJ)sarc4.$(OBJ)
$(GLD)sarc4.dev : $(LIB_MAK) $(ECHOGS_XE) $(sarc4_)
	$(SETMOD) $(GLD)sarc4 $(sarc4_)

$(GLOBJ)sarc4.$(OBJ) : $(GLSRC)sarc4.c $(AK) $(memory__h)\
 $(gserror_h) $(gserrors_h) $(sarc4_h) $(strimpl_h)
	$(GLCC) $(GLO_)sarc4.$(OBJ) $(C_) $(GLSRC)sarc4.c

# ---------------- JBIG2 compression filter ---------------- #

sjbig2_=$(GLOBJ)sjbig2.$(OBJ)
$(GLD)sjbig2.dev : $(LIB_MAK) $(ECHOGS_XE) $(GLD)libjbig2.dev $(sjbig2_)
	$(SETMOD) $(GLD)sjbig2 $(sjbig2_)
	$(ADDMOD) $(GLD)sjbig2 -include $(GLD)libjbig2.dev

$(GLOBJ)sjbig2.$(OBJ) : $(GLSRC)sjbig2.c $(AK) \
 $(stdint__h) $(memory__h) $(stdio__h) $(gserror_h) $(gserrors_h) $(gdebug_h) \
 $(sjbig2_h) $(strimpl_h)
	$(GLJBIG2CC) $(GLO_)sjbig2.$(OBJ) $(C_) $(GLSRC)sjbig2.c

# ---------------- JPEG 2000 compression filter ---------------- #

sjpx_=$(GLOBJ)sjpx.$(OBJ)
$(GLD)sjpx.dev : $(LIB_MAK) $(ECHOGS_XE) $(GLD)libjasper.dev $(sjpx_)
	$(SETMOD) $(GLD)sjpx $(sjpx_)
	$(ADDMOD) $(GLD)sjpx -include $(GLD)libjasper.dev

$(GLOBJ)sjpx.$(OBJ) : $(GLSRC)sjpx.c $(AK) \
 $(memory__h) $(stdio__h) $(gsmalloc_h) $(gserror_h) $(gserrors_h) \
 $(gdebug_h) $(strimpl_h) $(sjpx_h)
	$(GLCC) $(I_)$(JASI_)$(_I) $(JASCF_) $(GLO_)sjpx.$(OBJ) $(C_) $(GLSRC)sjpx.c

# libjasper.dev is created in jasper.mak

# ---------------- Pixel-difference filters ---------------- #
# The Predictor facility of the LZW and Flate filters uses these.

pdiff_=$(GLOBJ)spdiff.$(OBJ)
$(GLD)pdiff.dev : $(LIB_MAK) $(ECHOGS_XE) $(pdiff_)
	$(SETMOD) $(GLD)pdiff $(pdiff_)

$(GLOBJ)spdiff.$(OBJ) : $(GLSRC)spdiff.c $(AK) $(memory__h) $(stdio__h)\
 $(spdiffx_h) $(strimpl_h)
	$(GLCC) $(GLO_)spdiff.$(OBJ) $(C_) $(GLSRC)spdiff.c

# ---------------- PNG pixel prediction filters ---------------- #
# The Predictor facility of the LZW and Flate filters uses these.

pngp_=$(GLOBJ)spngp.$(OBJ)
$(GLD)pngp.dev : $(LIB_MAK) $(ECHOGS_XE) $(pngp_)
	$(SETMOD) $(GLD)pngp $(pngp_)

$(GLOBJ)spngp.$(OBJ) : $(GLSRC)spngp.c $(AK) $(memory__h)\
 $(spngpx_h) $(strimpl_h)
	$(GLCC) $(GLO_)spngp.$(OBJ) $(C_) $(GLSRC)spngp.c

# ---------------- RunLength filters ---------------- #
# These are used by clists and also by Level 2 in general.

rle_=$(GLOBJ)srle.$(OBJ)
$(GLD)rle.dev : $(LIB_MAK) $(ECHOGS_XE) $(rle_)
	$(SETMOD) $(GLD)rle $(rle_)

$(GLOBJ)srle.$(OBJ) : $(GLSRC)srle.c $(AK) $(stdio__h) $(memory__h)\
 $(srlx_h) $(strimpl_h)
	$(GLCC) $(GLO_)srle.$(OBJ) $(C_) $(GLSRC)srle.c

rld_=$(GLOBJ)srld.$(OBJ)
$(GLD)rld.dev : $(LIB_MAK) $(ECHOGS_XE) $(rld_)
	$(SETMOD) $(GLD)rld $(rld_)

$(GLOBJ)srld.$(OBJ) : $(GLSRC)srld.c $(AK) $(stdio__h) $(memory__h)\
 $(srlx_h) $(strimpl_h)
	$(GLCC) $(GLO_)srld.$(OBJ) $(C_) $(GLSRC)srld.c

# ---------------- String encoding/decoding filters ---------------- #
# These are used by the PostScript and PDF writers, and also by the
# PostScript interpreter.

$(GLOBJ)sa85d.$(OBJ) : $(GLSRC)sa85d.c $(AK) $(std_h)\
 $(sa85d_h) $(scanchar_h) $(strimpl_h)
	$(GLCC) $(GLO_)sa85d.$(OBJ) $(C_) $(GLSRC)sa85d.c

$(GLOBJ)scantab.$(OBJ) : $(GLSRC)scantab.c $(AK) $(stdpre_h)\
 $(scanchar_h) $(scommon_h)
	$(GLCC) $(GLO_)scantab.$(OBJ) $(C_) $(GLSRC)scantab.c

$(GLOBJ)sfilter2.$(OBJ) : $(GLSRC)sfilter2.c $(AK) $(memory__h) $(stdio__h)\
 $(gdebug_h) $(sa85x_h) $(scanchar_h) $(sbtx_h) $(strimpl_h)
	$(GLCC) $(GLO_)sfilter2.$(OBJ) $(C_) $(GLSRC)sfilter2.c

$(GLOBJ)sstring.$(OBJ) : $(GLSRC)sstring.c $(AK)\
 $(stdio__h) $(memory__h) $(string__h)\
 $(scanchar_h) $(sstring_h) $(strimpl_h)
	$(GLCC) $(GLO_)sstring.$(OBJ) $(C_) $(GLSRC)sstring.c

$(GLOBJ)spprint.$(OBJ) : $(GLSRC)spprint.c\
 $(math__h) $(stdio__h) $(string__h)\
 $(spprint_h) $(stream_h)
	$(GLCC) $(GLO_)spprint.$(OBJ) $(C_) $(GLSRC)spprint.c

$(GLOBJ)spsdf.$(OBJ) : $(GLSRC)spsdf.c $(stdio__h) $(string__h)\
 $(gserror_h) $(gserrors_h) $(gsmemory_h) $(gstypes_h)\
 $(sa85x_h) $(scanchar_h) $(spprint_h) $(spsdf_h)\
 $(sstring_h) $(stream_h) $(strimpl_h)
	$(GLCC) $(GLO_)spsdf.$(OBJ) $(C_) $(GLSRC)spsdf.c

# ---------------- zlib filters ---------------- #
# These are used by clists and are also available as filters.

szlibc_=$(GLOBJ)szlibc.$(OBJ)

$(GLOBJ)szlibc.$(OBJ) : $(GLSRC)szlibc.c $(AK) $(std_h)\
 $(gserror_h) $(gserrors_h) $(gsmalloc_h) $(gsmemory_h)\
 $(gsstruct_h) $(gstypes_h)\
 $(strimpl_h) $(szlibxx_h)
	$(GLZCC) $(GLO_)szlibc.$(OBJ) $(C_) $(GLSRC)szlibc.c

szlibe_=$(szlibc_) $(GLOBJ)szlibe.$(OBJ)
$(GLD)szlibe.dev : $(LIB_MAK) $(ECHOGS_XE) $(ZGENDIR)$(D)zlibe.dev $(szlibe_)
	$(SETMOD) $(GLD)szlibe $(szlibe_)
	$(ADDMOD) $(GLD)szlibe -include $(ZGENDIR)$(D)zlibe.dev

$(GLOBJ)szlibe.$(OBJ) : $(GLSRC)szlibe.c $(AK) $(std_h)\
 $(gsmalloc_h) $(gsmemory_h) $(strimpl_h) $(szlibxx_h)
	$(GLZCC) $(GLO_)szlibe.$(OBJ) $(C_) $(GLSRC)szlibe.c

szlibd_=$(szlibc_) $(GLOBJ)szlibd.$(OBJ)
$(GLD)szlibd.dev : $(LIB_MAK) $(ECHOGS_XE) $(ZGENDIR)$(D)zlibd.dev $(szlibd_)
	$(SETMOD) $(GLD)szlibd $(szlibd_)
	$(ADDMOD) $(GLD)szlibd -include $(ZGENDIR)$(D)zlibd.dev

$(GLOBJ)szlibd.$(OBJ) : $(GLSRC)szlibd.c $(AK) $(std_h) $(memory__h)\
 $(gsmalloc_h) $(gsmemory_h) $(strimpl_h) $(szlibxx_h)
	$(GLZCC) $(GLO_)szlibd.$(OBJ) $(C_) $(GLSRC)szlibd.c

# ---------------- Page devices ---------------- #
# We include this here, rather than in devs.mak, because it is more like
# a feature than a simple device.

gdevprn_h=$(GLSRC)gdevprn.h $(memory__h) $(string__h) $(gp_h) $(gx_h)\
 $(gserrors_h) $(gsmatrix_h) $(gsparam_h) $(gsutil_h)\
 $(gxclist_h) $(gxdevice_h) $(gxdevmem_h) $(gxrplane_h)

page_=$(GLOBJ)gdevprn.$(OBJ)
$(GLD)page.dev : $(LIB_MAK) $(ECHOGS_XE) $(page_) $(GLD)clist.dev
	$(SETMOD) $(GLD)page $(page_)
	$(ADDMOD) $(GLD)page -include $(GLD)clist

$(GLOBJ)gdevprn.$(OBJ) : $(GLSRC)gdevprn.c $(ctype__h)\
 $(gdevprn_h) $(gp_h) $(gsdevice_h) $(gsfname_h) $(gsparam_h)\
 $(gxclio_h) $(gxgetbit_h) $(gdevplnx_h) $(gstrans_h)
	$(GLCC) $(GLO_)gdevprn.$(OBJ) $(C_) $(GLSRC)gdevprn.c

# Planar page devices
gdevppla_h=$(GLSRC)gdevppla.h

$(GLOBJ)gdevppla.$(OBJ) : $(GLSRC)gdevppla.c\
 $(gdevmpla_h) $(gdevppla_h) $(gdevprn_h)
	$(GLCC) $(GLO_)gdevppla.$(OBJ) $(C_) $(GLSRC)gdevppla.c

# ---------------- Masked images ---------------- #
# This feature is out of level order because Patterns require it
# (which they shouldn't) and because band lists treat ImageType 4
# images as a special case (which they shouldn't).

gsiparm3_h=$(GLSRC)gsiparm3.h $(gsiparam_h)
gsiparm4_h=$(GLSRC)gsiparm4.h $(gsiparam_h)
gximage3_h=$(GLSRC)gximage3.h $(gsiparm3_h) $(gxiparam_h)

$(GLOBJ)gxclipm.$(OBJ) : $(GLSRC)gxclipm.c $(GX) $(memory__h)\
 $(gsbittab_h) $(gxclipm_h) $(gxdevice_h) $(gxdevmem_h)
	$(GLCC) $(GLO_)gxclipm.$(OBJ) $(C_) $(GLSRC)gxclipm.c

$(GLOBJ)gximage3.$(OBJ) : $(GLSRC)gximage3.c $(GXERR) $(math__h) $(memory__h)\
 $(gsbitops_h) $(gscspace_h) $(gsstruct_h)\
 $(gxclipm_h) $(gxdevice_h) $(gxdevmem_h) $(gximage3_h) $(gxistate_h)
	$(GLCC) $(GLO_)gximage3.$(OBJ) $(C_) $(GLSRC)gximage3.c

$(GLOBJ)gximage4.$(OBJ) : $(GLSRC)gximage4.c $(memory__h) $(GXERR)\
 $(gscspace_h) $(gsiparm4_h) $(gxiparam_h) $(gximage_h)\
 $(stream_h)
	$(GLCC) $(GLO_)gximage4.$(OBJ) $(C_) $(GLSRC)gximage4.c

imasklib_=$(GLOBJ)gxclipm.$(OBJ) $(GLOBJ)gximage3.$(OBJ) $(GLOBJ)gximage4.$(OBJ) $(GLOBJ)gxmclip.$(OBJ)
$(GLD)imasklib.dev : $(LIB_MAK) $(ECHOGS_XE) $(imasklib_)
	$(SETMOD) $(GLD)imasklib $(imasklib_)
	$(ADDMOD) $(GLD)imasklib -imagetype 3 4

# ---------------- Banded ("command list") devices ---------------- #

gxcldev_h=$(GLSRC)gxcldev.h $(gxclist_h) $(gsropt_h) $(gxht_h) $(gxtmap_h) $(gxdht_h)\
 $(strimpl_h) $(scfx_h) $(srlx_h) $(gsdcolor_h)
gxclpage_h=$(GLSRC)gxclpage.h $(gxclio_h)
gxclpath_h=$(GLSRC)gxclpath.h

clbase1_=$(GLOBJ)gxclist.$(OBJ) $(GLOBJ)gxclbits.$(OBJ) $(GLOBJ)gxclpage.$(OBJ)
clbase2_=$(GLOBJ)gxclrast.$(OBJ) $(GLOBJ)gxclread.$(OBJ) $(GLOBJ)gxclrect.$(OBJ)
clbase3_=$(GLOBJ)gxclutil.$(OBJ) $(GLOBJ)gsparams.$(OBJ)
# gxclrect.c requires rop_proc_table, so we need gsroptab here.
clbase4_=$(GLOBJ)gsroptab.$(OBJ) $(GLOBJ)stream.$(OBJ)
clpath_=$(GLOBJ)gxclimag.$(OBJ) $(GLOBJ)gxclpath.$(OBJ) $(GLOBJ)gxdhtserial.$(OBJ)
clist_=$(clbase1_) $(clbase2_) $(clbase3_) $(clbase4_) $(clpath_)
$(GLD)clist.dev : $(LIB_MAK) $(ECHOGS_XE) $(clist_)\
 $(GLD)cl$(BAND_LIST_STORAGE).dev\
 $(GLD)cfe.dev $(GLD)cfd.dev $(GLD)rle.dev $(GLD)rld.dev $(GLD)psl2cs.dev
	$(SETMOD) $(GLD)clist $(clbase1_)
	$(ADDMOD) $(GLD)clist -obj $(clbase2_)
	$(ADDMOD) $(GLD)clist -obj $(clbase3_)
	$(ADDMOD) $(GLD)clist -obj $(clbase4_)
	$(ADDMOD) $(GLD)clist -obj $(clpath_)
	$(ADDMOD) $(GLD)clist -include $(GLD)cl$(BAND_LIST_STORAGE)
	$(ADDMOD) $(GLD)clist -include $(GLD)cfe $(GLD)cfd $(GLD)rle $(GLD)rld $(GLD)psl2cs

$(GLOBJ)gxclist.$(OBJ) : $(GLSRC)gxclist.c $(GXERR) $(memory__h) $(string__h)\
 $(gp_h) $(gpcheck_h) $(gsparams_h)\
 $(gxcldev_h) $(gxclpath_h) $(gxdevice_h) $(gxdevmem_h) $(gxdcolor_h)
	$(GLCC) $(GLO_)gxclist.$(OBJ) $(C_) $(GLSRC)gxclist.c

$(GLOBJ)gxclbits.$(OBJ) : $(GLSRC)gxclbits.c $(GXERR) $(memory__h) $(gpcheck_h)\
 $(gsbitops_h) $(gxcldev_h) $(gxdevice_h) $(gxdevmem_h) $(gxfmap_h)
	$(GLCC) $(GLO_)gxclbits.$(OBJ) $(C_) $(GLSRC)gxclbits.c

$(GLOBJ)gxclpage.$(OBJ) : $(GLSRC)gxclpage.c $(AK)\
 $(gdevprn_h) $(gxcldev_h) $(gxclpage_h)
	$(GLCC) $(GLO_)gxclpage.$(OBJ) $(C_) $(GLSRC)gxclpage.c

$(GLOBJ)gxclrast.$(OBJ) : $(GLSRC)gxclrast.c $(GXERR)\
 $(memory__h) $(gp_h) $(gpcheck_h)\
 $(gscdefs_h) $(gsbitops_h) $(gsparams_h) $(gsstate_h)\
 $(gxdcolor_h) $(gxdevice_h)\
 $(gsdevice_h) $(gsiparm4_h)\
 $(gxdevmem_h) $(gxcldev_h) $(gxclpath_h) $(gxcmap_h)\
 $(gxcolor2_h) $(gxcspace_h) $(gxdhtres_h) $(gxgetbit_h)\
 $(gxpaint_h) $(gxhttile_h) $(gxiparam_h)\
 $(gzpath_h) $(gxcpath_h) $(gzacpath_h)\
 $(stream_h) $(strimpl_h) $(gxcomp_h)\
 $(gsserial_h) $(gxdhtserial_h) $(gzht_h)
	$(GLCC) $(GLO_)gxclrast.$(OBJ) $(C_) $(GLSRC)gxclrast.c

$(GLOBJ)gxclread.$(OBJ) : $(GLSRC)gxclread.c $(GXERR)\
 $(memory__h) $(gp_h) $(gpcheck_h)\
 $(gdevplnx_h) $(gdevprn_h)\
 $(gscoord_h) $(gsdevice_h)\
 $(gxcldev_h) $(gxdevice_h) $(gxdevmem_h) $(gxgetbit_h) $(gxhttile_h)\
 $(stream_h) $(strimpl_h)
	$(GLCC) $(GLO_)gxclread.$(OBJ) $(C_) $(GLSRC)gxclread.c

$(GLOBJ)gxclrect.$(OBJ) : $(GLSRC)gxclrect.c $(GXERR)\
 $(gsutil_h) $(gxcldev_h) $(gxdevice_h) $(gxdevmem_h)
	$(GLCC) $(GLO_)gxclrect.$(OBJ) $(C_) $(GLSRC)gxclrect.c

$(GLOBJ)gxclimag.$(OBJ) : $(GLSRC)gxclimag.c $(GXERR) $(math__h) $(memory__h)\
 $(gscdefs_h) $(gscspace_h)\
 $(gxarith_h) $(gxcldev_h) $(gxclpath_h) $(gxcspace_h)\
 $(gxdevice_h) $(gxdevmem_h) $(gxfmap_h) $(gxiparam_h) $(gxpath_h)\
 $(sisparam_h) $(stream_h) $(strimpl_h) $(gxcomp_h) $(gsserial_h)\
 $(gxdhtserial_h)
	$(GLCC) $(GLO_)gxclimag.$(OBJ) $(C_) $(GLSRC)gxclimag.c

$(GLOBJ)gxclpath.$(OBJ) : $(GLSRC)gxclpath.c $(GXERR)\
 $(math__h) $(memory__h) $(gpcheck_h)\
 $(gxcldev_h) $(gxclpath_h) $(gxcolor2_h)\
 $(gxdcolor_h) $(gxpaint_h)\
 $(gzpath_h) $(gzcpath_h) $(stream_h) $(gsserial_h)
	$(GLCC) $(GLO_)gxclpath.$(OBJ) $(C_) $(GLSRC)gxclpath.c

$(GLOBJ)gxdhtserial.$(OBJ) : $(GLSRC)gxdhtserial.c $(memory__h) $(GXERR)\
 $(gscdefs_h) $(gsstruct_h) $(gsutil_h) $(gzstate_h) $(gxdevice_h) $(gzht_h)\
 $(gswts_h) $(gxdhtres_h) $(gsserial_h) $(gxdhtserial_h)
	$(GLCC) $(GLO_)gxdhtserial.$(OBJ) $(C_) $(GLSRC)gxdhtserial.c

$(GLOBJ)gxclutil.$(OBJ) : $(GLSRC)gxclutil.c $(GXERR) $(memory__h) $(string__h)\
 $(gp_h) $(gpcheck_h)\
 $(gsparams_h)\
 $(gxcldev_h) $(gxclpath_h) $(gxdevice_h) $(gxdevmem_h)
	$(GLCC) $(GLO_)gxclutil.$(OBJ) $(C_) $(GLSRC)gxclutil.c

# Implement band lists on files.

clfile_=$(GLOBJ)gxclfile.$(OBJ)
$(GLD)clfile.dev : $(LIB_MAK) $(ECHOGS_XE) $(clfile_)
	$(SETMOD) $(GLD)clfile $(clfile_)

$(GLOBJ)gxclfile.$(OBJ) : $(GLSRC)gxclfile.c $(stdio__h) $(string__h)\
 $(gp_h) $(gsmemory_h) $(gserror_h) $(gserrors_h) $(gxclio_h)
	$(GLCC) $(GLO_)gxclfile.$(OBJ) $(C_) $(GLSRC)gxclfile.c

# Implement band lists in memory (RAM).

clmemory_=$(GLOBJ)gxclmem.$(OBJ) $(GLOBJ)gxcl$(BAND_LIST_COMPRESSOR).$(OBJ)
$(GLD)clmemory.dev : $(LIB_MAK) $(ECHOGS_XE) $(clmemory_) $(GLD)s$(BAND_LIST_COMPRESSOR)e.dev $(GLD)s$(BAND_LIST_COMPRESSOR)d.dev
	$(SETMOD) $(GLD)clmemory $(clmemory_)
	$(ADDMOD) $(GLD)clmemory -include $(GLD)s$(BAND_LIST_COMPRESSOR)e
	$(ADDMOD) $(GLD)clmemory -include $(GLD)s$(BAND_LIST_COMPRESSOR)d
	$(ADDMOD) $(GLD)clmemory -init cl_$(BAND_LIST_COMPRESSOR)

gxclmem_h=$(GLSRC)gxclmem.h $(gxclio_h) $(strimpl_h)

$(GLOBJ)gxclmem.$(OBJ) : $(GLSRC)gxclmem.c $(GXERR) $(LIB_MAK) $(memory__h)\
 $(gxclmem_h)
	$(GLCC) $(GLO_)gxclmem.$(OBJ) $(C_) $(GLSRC)gxclmem.c

# Implement the compression method for RAM-based band lists.

$(GLOBJ)gxcllzw.$(OBJ) : $(GLSRC)gxcllzw.c $(std_h)\
 $(gsmemory_h) $(gstypes_h) $(gxclmem_h) $(slzwx_h)
	$(GLCC) $(GLO_)gxcllzw.$(OBJ) $(C_) $(GLSRC)gxcllzw.c

$(GLOBJ)gxclzlib.$(OBJ) : $(GLSRC)gxclzlib.c $(std_h)\
 $(gsmemory_h) $(gstypes_h) $(gxclmem_h) $(szlibx_h)
	$(GLCC) $(GLO_)gxclzlib.$(OBJ) $(C_) $(GLSRC)gxclzlib.c

# ---------------- Vector devices ---------------- #
# We include this here for the same reasons as page.dev.

gdevvec_h=$(GLSRC)gdevvec.h $(gdevbbox_h) $(gp_h)\
 $(gsropt_h) $(gxdevice_h) $(gxiparam_h) $(gxistate_h) $(gxhldevc_h) $(stream_h)

vector_=$(GLOBJ)gdevvec.$(OBJ)
$(GLD)vector.dev : $(LIB_MAK) $(ECHOGS_XE) $(vector_)\
 $(GLD)bboxutil.dev $(GLD)sfile.dev
	$(SETMOD) $(GLD)vector $(vector_)
	$(ADDMOD) $(GLD)vector -include $(GLD)bboxutil $(GLD)sfile

$(GLOBJ)gdevvec.$(OBJ) : $(GLSRC)gdevvec.c $(GXERR)\
 $(math__h) $(memory__h) $(string__h)\
 $(gdevvec_h) $(gp_h) $(gscspace_h) $(gsparam_h) $(gsutil_h)\
 $(gxdcolor_h) $(gxfixed_h) $(gxpaint_h)\
 $(gzcpath_h) $(gzpath_h)
	$(GLCC) $(GLO_)gdevvec.$(OBJ) $(C_) $(GLSRC)gdevvec.c

# ---------------- Image scaling filters ---------------- #

iscale_=$(GLOBJ)siinterp.$(OBJ) $(GLOBJ)siscale.$(OBJ)
$(GLD)iscale.dev : $(LIB_MAK) $(ECHOGS_XE) $(iscale_)
	$(SETMOD) $(GLD)iscale $(iscale_)

$(GLOBJ)siinterp.$(OBJ) : $(GLSRC)siinterp.c $(AK)\
 $(memory__h) $(gxdda_h) $(gxfixed_h) $(gxfrac_h)\
 $(siinterp_h) $(strimpl_h)
	$(GLCC) $(GLO_)siinterp.$(OBJ) $(C_) $(GLSRC)siinterp.c

$(GLOBJ)siscale.$(OBJ) : $(GLSRC)siscale.c $(AK)\
 $(math__h) $(memory__h) $(stdio__h)\
 $(gconfigv_h) $(gdebug_h)\
 $(siscale_h) $(strimpl_h)
	$(GLCC) $(GLO_)siscale.$(OBJ) $(C_) $(GLSRC)siscale.c

# ---------------- Extended halftone support ---------------- #
# This is only used by one non-PostScript-based project.

gshtx_h=$(GLSRC)gshtx.h $(gsht1_h) $(gsmemory_h) $(gxtmap_h) $(gscspace_h)

htxlib_=$(GLOBJ)gshtx.$(OBJ)
$(GLD)htxlib.dev : $(LIB_MAK) $(ECHOGS_XE) $(htxlib_)
	$(SETMOD) $(GLD)htxlib $(htxlib_)

$(GLOBJ)gshtx.$(OBJ) : $(GLSRC)gshtx.c $(GXERR) $(memory__h)\
 $(gsstruct_h) $(gsutil_h)\
 $(gxfmap_h) $(gshtx_h) $(gzht_h) $(gzstate_h)
	$(GLCC) $(GLO_)gshtx.$(OBJ) $(C_) $(GLSRC)gshtx.c

# ---------------- RasterOp et al ---------------- #
# Note that noroplib is a default, roplib replaces it.

gsropc_h=$(GLSRC)gsropc.h $(gscompt_h) $(gsropt_h)
gxropc_h=$(GLSRC)gxropc.h $(gsropc_h) $(gxcomp_h)

noroplib_=$(GLOBJ)gsnorop.$(OBJ)
$(GLD)noroplib.dev : $(LIB_MAK) $(ECHOGS_XE) $(noroplib_)
	$(SETMOD) $(GLD)noroplib $(noroplib_)

$(GLOBJ)gsnorop.$(OBJ) : $(GLSRC)gsnorop.c $(GXERR)\
 $(gdevmem_h) $(gdevmrop_h) $(gsrop_h)\
 $(gxdevcli_h) $(gxdevice_h) $(gxdevmem_h)
	$(GLCC) $(GLO_)gsnorop.$(OBJ) $(C_) $(GLSRC)gsnorop.c

roplib1_=$(GLOBJ)gdevdrop.$(OBJ)
roplib2_=$(GLOBJ)gdevmr1.$(OBJ) $(GLOBJ)gdevmr2n.$(OBJ) $(GLOBJ)gdevmr8n.$(OBJ)
roplib3_=$(GLOBJ)gdevrops.$(OBJ) $(GLOBJ)gsrop.$(OBJ) $(GLOBJ)gsroptab.$(OBJ)
roplib_=$(roplib1_) $(roplib2_) $(roplib3_)
$(GLD)roplib.dev : $(LIB_MAK) $(ECHOGS_XE) $(roplib_)
	$(SETMOD) $(GLD)roplib $(roplib1_)
	$(ADDMOD) $(GLD)roplib $(roplib2_)
	$(ADDMOD) $(GLD)roplib $(roplib3_)
	$(ADDMOD) $(GLD)roplib -replace $(GLD)noroplib

$(GLOBJ)gdevdrop.$(OBJ) : $(GLSRC)gdevdrop.c $(GXERR) $(memory__h)\
 $(gsbittab_h) $(gsropt_h)\
 $(gxcindex_h) $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h) $(gxdevrop_h)\
 $(gxgetbit_h)\
 $(gdevmem_h) $(gdevmrop_h)
	$(GLCC) $(GLO_)gdevdrop.$(OBJ) $(C_) $(GLSRC)gdevdrop.c

$(GLOBJ)gdevmr1.$(OBJ) : $(GLSRC)gdevmr1.c $(GXERR) $(memory__h)\
 $(gsbittab_h) $(gsropt_h)\
 $(gxcindex_h) $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h) $(gxdevrop_h)\
 $(gdevmrop_h)
	$(GLCC) $(GLO_)gdevmr1.$(OBJ) $(C_) $(GLSRC)gdevmr1.c

$(GLOBJ)gdevmr2n.$(OBJ) : $(GLSRC)gdevmr2n.c $(GXERR) $(memory__h)\
 $(gsbittab_h) $(gsropt_h)\
 $(gxcindex_h) $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h) $(gxdevrop_h)\
 $(gdevmem_h) $(gdevmrop_h)
	$(GLCC) $(GLO_)gdevmr2n.$(OBJ) $(C_) $(GLSRC)gdevmr2n.c

$(GLOBJ)gdevmr8n.$(OBJ) : $(GLSRC)gdevmr8n.c $(GXERR) $(memory__h)\
 $(gsbittab_h) $(gsropt_h)\
 $(gxcindex_h) $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h) $(gxdevrop_h)\
 $(gdevmem_h) $(gdevmrop_h)
	$(GLCC) $(GLO_)gdevmr8n.$(OBJ) $(C_) $(GLSRC)gdevmr8n.c

$(GLOBJ)gdevrops.$(OBJ) : $(GLSRC)gdevrops.c $(GXERR)\
 $(gxdcolor_h) $(gxdevice_h) $(gdevmrop_h)
	$(GLCC) $(GLO_)gdevrops.$(OBJ) $(C_) $(GLSRC)gdevrops.c

$(GLOBJ)gsrop.$(OBJ) : $(GLSRC)gsrop.c $(GXERR)\
 $(gsrop_h) $(gzstate_h)
	$(GLCC) $(GLO_)gsrop.$(OBJ) $(C_) $(GLSRC)gsrop.c

$(GLOBJ)gsroptab.$(OBJ) : $(GLSRC)gsroptab.c $(stdpre_h) $(gsropt_h)
	$(GLCC) $(GLO_)gsroptab.$(OBJ) $(C_) $(GLSRC)gsroptab.c

# The following is not used yet.
$(GLOBJ)gsropc.$(OBJ) : $(GLSRC)gsropc.c $(GXERR)\
 $(gsutil_h) $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h) $(gxropc_h)
	$(GLCC) $(GLO_)gsropc.$(OBJ) $(C_) $(GLSRC)gsropc.c

# ---------------- Async rendering ---------------- #

gxpageq_h=$(GLSRC)gxpageq.h $(gsmemory_h) $(gxband_h) $(gxsync_h)
gdevprna_h=$(GLSRC)gdevprna.h $(gdevprn_h) $(gxsync_h)

async_=$(GLOBJ)gdevprna.$(OBJ) $(GLOBJ)gxpageq.$(OBJ)
async_inc=$(GLD)clist.dev $(GLD)gsnogc.dev $(GLD)$(SYNC).dev
$(GLD)async.dev : $(LIB_MAK) $(ECHOGS_XE) $(async_) $(async_inc)
	$(SETMOD) $(GLD)async $(async_)
	$(ADDMOD) $(GLD)async -include $(async_inc)

$(GLOBJ)gdevprna.$(OBJ) : $(GLSRC)gdevprna.c $(AK)\
 $(gdevprna_h)\
 $(gsalloc_h) $(gsdevice_h) $(gsmemlok_h) $(gsmemret_h) $(gsnogc_h)\
 $(gxcldev_h) $(gxclpath_h) $(gxpageq_h) $(gzht_h)
	$(GLCC) $(GLO_)gdevprna.$(OBJ) $(C_) $(GLSRC)gdevprna.c

$(GLOBJ)gxpageq.$(OBJ) : $(GLSRC)gxpageq.c $(GXERR)\
 $(gsstruct_h) $(gxdevice_h) $(gxclist_h) $(gxpageq_h)
	$(GLCC) $(GLO_)gxpageq.$(OBJ) $(C_) $(GLSRC)gxpageq.c

# ---------------- TrueType and PostScript Type 42 fonts ---------------- #

ttflib_=$(GLOBJ)gstype42.$(OBJ) $(GLOBJ)gxchrout.$(OBJ) \
 $(GLOBJ)ttcalc.$(OBJ) $(GLOBJ)ttfinp.$(OBJ) $(GLOBJ)ttfmain.$(OBJ) $(GLOBJ)ttfmemd.$(OBJ) \
 $(GLOBJ)ttinterp.$(OBJ) $(GLOBJ)ttload.$(OBJ) $(GLOBJ)ttobjs.$(OBJ) \
 $(GLOBJ)gxttfb.$(OBJ) $(GLOBJ)gzspotan.$(OBJ)

$(GLD)ttflib.dev : $(LIB_MAK) $(ECHOGS_XE) $(ttflib_)
	$(SETMOD) $(GLD)ttflib $(ttflib_)

gxfont42_h=$(GLSRC)gxfont42.h
gxttf_h=$(GLSRC)gxttf.h

$(GLOBJ)gstype42.$(OBJ) : $(GLSRC)gstype42.c $(GXERR) $(memory__h)\
 $(gsccode_h) $(gsline_h) $(gsmatrix_h) $(gsstruct_h) $(gsutil_h)\
 $(gxchrout_h) $(gxfixed_h) $(gxfont_h) $(gxfont42_h) $(gxistate_h)\
 $(gxpath_h) $(gxttf_h) $(gxttfb_h) $(gxfcache_h) $(stream_h)
	$(GLCC) $(GLO_)gstype42.$(OBJ) $(C_) $(GLSRC)gstype42.c

ttfsfnt_h=$(GLSRC)ttfsfnt.h $(stdint__h)
ttcommon_h=$(GLSRC)ttcommon.h
ttconf_h=$(GLSRC)ttconf.h
ttfinp_h=$(GLSRC)ttfinp.h
ttfmemd_h=$(GLSRC)ttfmemd.h $(gsstype_h)
tttype_h=$(GLSRC)tttype.h
ttconfig_h=$(GLSRC)ttconfig.h $(ttconf_h)
tttypes_h=$(GLSRC)tttypes.h $(ttconfig_h) $(tttype_h)
ttmisc_h=$(GLSRC)ttmisc.h $(std_h) $(gx_h) $(string__h) $(math__h) $(tttypes_h)
tttables_h=$(GLSRC)tttables.h $(tttypes_h)
ttobjs_h=$(GLSRC)ttobjs.h $(ttcommon_h) $(tttypes_h) $(tttables_h)
ttcalc_h=$(GLSRC)ttcalc.h $(ttcommon_h) $(tttypes_h)
ttinterp_h=$(GLSRC)ttinterp.h $(ttcommon_h) $(ttobjs_h)
ttload_h=$(GLSRC)ttload.h $(ttcommon_h)
gxhintn_h=$(GLSRC)gxhintn.h $(stdint__h)

$(GLOBJ)ttcalc.$(OBJ) : $(GLSRC)ttcalc.c $(GXERR) $(ttmisc_h) $(ttcalc_h)
	$(GLCC) $(GLO_)ttcalc.$(OBJ) $(C_) $(GLSRC)ttcalc.c

$(GLOBJ)ttfinp.$(OBJ) : $(GLSRC)ttfinp.c $(GXERR) $(ttmisc_h)\
 $(ttfoutl_h) $(ttfsfnt_h) $(ttfinp_h)
	$(GLCC) $(GLO_)ttfinp.$(OBJ) $(C_) $(GLSRC)ttfinp.c

$(GLOBJ)ttfmain.$(OBJ) : $(GLSRC)ttfmain.c $(GXERR) $(ttmisc_h)\
 $(ttfoutl_h) $(ttfmemd_h) $(ttfsfnt_h) $(ttobjs_h) $(ttinterp_h) $(ttcalc_h)
	$(GLCC) $(GLO_)ttfmain.$(OBJ) $(C_) $(GLSRC)ttfmain.c

$(GLOBJ)ttfmemd.$(OBJ) : $(GLSRC)ttfmemd.c $(GXERR) $(ttmisc_h)\
 $(ttfoutl_h) $(ttobjs_h)
	$(GLCC) $(GLO_)ttfmemd.$(OBJ) $(C_) $(GLSRC)ttfmemd.c

$(GLOBJ)ttinterp.$(OBJ) : $(GLSRC)ttinterp.c $(GXERR) $(ttmisc_h)\
 $(ttfoutl_h) $(tttypes_h) $(ttcalc_h) $(ttinterp_h) $(ttfinp_h) 
	$(GLCC) $(GLO_)ttinterp.$(OBJ) $(C_) $(GLSRC)ttinterp.c

$(GLOBJ)ttload.$(OBJ) : $(GLSRC)ttload.c $(GXERR) $(ttmisc_h)\
 $(ttfoutl_h) $(tttypes_h) $(ttcalc_h) $(ttobjs_h) $(ttload_h) $(ttfinp_h)
	$(GLCC) $(GLO_)ttload.$(OBJ) $(C_) $(GLSRC)ttload.c

$(GLOBJ)ttobjs.$(OBJ) : $(GLSRC)ttobjs.c $(GXERR) $(ttmisc_h)\
 $(ttfoutl_h) $(ttobjs_h) $(ttcalc_h) $(ttload_h) $(ttinterp_h)
	$(GLCC) $(GLO_)ttobjs.$(OBJ) $(C_) $(GLSRC)ttobjs.c

$(GLOBJ)gxttfb.$(OBJ) : $(GLSRC)gxttfb.c $(GXERR) \
 $(gx_h) $(gxfont_h) $(gxfont42_h) $(gxttfb_h) $(gxfcache_h)\
 $(gxmatrix_h) $(gxhintn_h) $(gzpath_h) $(ttfmemd_h)\
 $(gsstruct_h) $(gserrors_h) $(gsfont_h) $(gsdebug_h) $(memory__h) $(math__h)\
 $(gxistate_h) $(gxpaint_h) $(gzspotan_h)
	$(GLCC) $(GLO_)gxttfb.$(OBJ) $(C_) $(GLSRC)gxttfb.c

$(GLOBJ)gzspotan.$(OBJ) : $(GLSRC)gzspotan.c $(GXERR)\
 $(gx_h) $(gserrors_h) $(gsdevice_h) $(gzspotan_h)\
 $(gsfixed_h) $(gxdevice_h) $(gxfdrop_h) $(gzpath_h)\
 $(memory__h) $(math__h) $(vdtrace_h)
	$(GLCC) $(GLO_)gzspotan.$(OBJ) $(C_) $(GLSRC)gzspotan.c


# -------- Composite (PostScript Type 0) font support -------- #

gxcid_h=$(GLSRC)gxcid.h $(gsstype_h)
gxfcid_h=$(GLSRC)gxfcid.h $(gxcid_h) $(gxfont_h) $(gxfont42_h)
gxfcmap_h=$(GLSRC)gxfcmap.h $(gsfcmap_h) $(gsuid_h) $(gxcid_h)
gxfcmap1_h=$(GLSRC)gxfcmap1.h $(gxfcmap_h)
gxfont0c_h=$(GLSRC)gxfont0c.h $(gxfcid_h) $(gxfont0_h)

cidlib_=$(GLOBJ)gsfcid.$(OBJ) $(GLOBJ)gsfcid2.$(OBJ)
# cidlib requires ttflib for CIDFontType 2 fonts.
$(GLD)cidlib.dev : $(LIB_MAK) $(ECHOGS_XE) $(cidlib_) $(GLD)ttflib.dev
	$(SETMOD) $(GLD)cidlib $(cidlib_)
	$(ADDMOD) $(GLD)cidlib -include $(GLD)ttflib

$(GLOBJ)gsfcid.$(OBJ) : $(GLSRC)gsfcid.c $(GX) $(memory__h)\
 $(gsmatrix_h) $(gsstruct_h) $(gxfcid_h)
	$(GLCC) $(GLO_)gsfcid.$(OBJ) $(C_) $(GLSRC)gsfcid.c

$(GLOBJ)gsfcid2.$(OBJ) : $(GLSRC)gsfcid2.c $(GXERR) $(memory__h)\
 $(gsstruct_h) $(gsutil_h) $(gxfcid_h) $(gxfcmap_h) $(gxfont_h) $(gxfont0c_h)
	$(GLCC) $(GLO_)gsfcid2.$(OBJ) $(C_) $(GLSRC)gsfcid2.c

cmaplib_=$(GLOBJ)gsfcmap.$(OBJ) $(GLOBJ)gsfcmap1.$(OBJ)
$(GLD)cmaplib.dev : $(LIB_MAK) $(ECHOGS_XE) $(cmaplib_) $(GLD)cidlib.dev
	$(SETMOD) $(GLD)cmaplib $(cmaplib_)
	$(ADDMOD) $(GLD)cmaplib -include $(GLD)cidlib

$(GLOBJ)gsfcmap.$(OBJ) : $(GLSRC)gsfcmap.c $(GXERR) $(memory__h) $(string__h)\
 $(gsstruct_h) $(gsutil_h) $(gxfcmap_h)
	$(GLCC) $(GLO_)gsfcmap.$(OBJ) $(C_) $(GLSRC)gsfcmap.c

$(GLOBJ)gsfcmap1.$(OBJ) : $(GLSRC)gsfcmap1.c $(GXERR)\
 $(memory__h) $(string__h)\
 $(gsstruct_h) $(gsutil_h) $(gxfcmap1_h)
	$(GLCC) $(GLO_)gsfcmap1.$(OBJ) $(C_) $(GLSRC)gsfcmap1.c

psf0lib_=$(GLOBJ)gschar0.$(OBJ) $(GLOBJ)gsfont0.$(OBJ)
$(GLD)psf0lib.dev : $(LIB_MAK) $(ECHOGS_XE) $(GLD)cmaplib.dev $(psf0lib_)
	$(SETMOD) $(GLD)psf0lib $(psf0lib_)
	$(ADDMOD) $(GLD)psf0lib -include $(GLD)cmaplib

$(GLOBJ)gschar0.$(OBJ) : $(GLSRC)gschar0.c $(GXERR) $(memory__h)\
 $(gsfcmap_h) $(gsstruct_h)\
 $(gxdevice_h) $(gxfcmap_h) $(gxfixed_h) $(gxfont_h) $(gxfont0_h) $(gxtext_h)
	$(GLCC) $(GLO_)gschar0.$(OBJ) $(C_) $(GLSRC)gschar0.c

$(GLOBJ)gsfont0.$(OBJ) : $(GLSRC)gsfont0.c $(GXERR) $(memory__h)\
 $(gsmatrix_h) $(gsstruct_h) $(gxfixed_h) $(gxdevmem_h) $(gxfcache_h)\
 $(gxfont_h) $(gxfont0_h) $(gxdevice_h)
	$(GLCC) $(GLO_)gsfont0.$(OBJ) $(C_) $(GLSRC)gsfont0.c

# gsfont0c is not needed for the PS interpreter, other than for testing,
# but it is used by pdfwrite and by the PCL interpreter.
$(GLOBJ)gsfont0c.$(OBJ) : $(GLSRC)gsfont0c.c $(GXERR) $(memory__h)\
 $(gxfont_h) $(gxfont0_h) $(gxfont0c_h) $(gxfcid_h) $(gxfcmap_h)
	$(GLCC) $(GLO_)gsfont0c.$(OBJ) $(C_) $(GLSRC)gsfont0c.c

# ---------------- Pattern color ---------------- #

patlib_1=$(GLOBJ)gspcolor.$(OBJ) $(GLOBJ)gsptype1.$(OBJ) $(GLOBJ)gxclip2.$(OBJ)
patlib_2=$(GLOBJ)gxmclip.$(OBJ) $(GLOBJ)gxp1fill.$(OBJ) $(GLOBJ)gxpcmap.$(OBJ)
patlib_=$(patlib_1) $(patlib_2)
# Currently this feature requires masked images, but it shouldn't.
$(GLD)patlib.dev : $(LIB_MAK) $(ECHOGS_XE) $(GLD)cmyklib.dev $(GLD)imasklib.dev $(GLD)psl2cs.dev $(patlib_)
	$(SETMOD) $(GLD)patlib -include $(GLD)cmyklib $(GLD)imasklib $(GLD)psl2cs
	$(ADDMOD) $(GLD)patlib -obj $(patlib_1)
	$(ADDMOD) $(GLD)patlib -obj $(patlib_2)

$(GLOBJ)gspcolor.$(OBJ) : $(GLSRC)gspcolor.c $(GXERR) $(math__h)\
 $(gsimage_h) $(gsiparm4_h) $(gspath_h) $(gsrop_h) $(gsstruct_h) $(gsutil_h)\
 $(gxarith_h) $(gxcolor2_h) $(gxcoord_h) $(gxclip2_h) $(gxcspace_h)\
 $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h) $(gxfixed_h) $(gxmatrix_h)\
 $(gxpath_h) $(gxpcolor_h) $(gzstate_h) $(stream_h)
	$(GLCC) $(GLO_)gspcolor.$(OBJ) $(C_) $(GLSRC)gspcolor.c

$(GLOBJ)gsptype1.$(OBJ) : $(GLSRC)gsptype1.c $(math__h) $(GXERR)\
 $(gsrop_h) $(gsstruct_h) $(gsutil_h)\
 $(gxarith_h)  $(gxfixed_h) $(gxmatrix_h) $(gxcoord_h) $(gxcspace_h)\
 $(gxcolor2_h) $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h) $(gxclip2_h)\
 $(gspath_h) $(gxpath_h) $(gxpcolor_h) $(gxp1impl_h) $(gzstate_h)\
 $(gsimage_h) $(gsiparm4_h) $(gsovrc_h)
	$(GLCC) $(GLO_)gsptype1.$(OBJ) $(C_) $(GLSRC)gsptype1.c

$(GLOBJ)gxclip2.$(OBJ) : $(GLSRC)gxclip2.c $(GXERR) $(memory__h)\
 $(gsstruct_h) $(gxclip2_h) $(gxdevice_h) $(gxdevmem_h)
	$(GLCC) $(GLO_)gxclip2.$(OBJ) $(C_) $(GLSRC)gxclip2.c

$(GLOBJ)gxp1fill.$(OBJ) : $(GLSRC)gxp1fill.c $(GXERR) $(math__h)\
 $(gsrop_h) $(gsmatrix_h)\
 $(gxcolor2_h) $(gxclip2_h) $(gxcspace_h) $(gxdcolor_h) $(gxdevcli_h)\
 $(gxdevmem_h) $(gxp1impl_h) $(gxpcolor_h)
	$(GLCC) $(GLO_)gxp1fill.$(OBJ) $(C_) $(GLSRC)gxp1fill.c

$(GLOBJ)gxpcmap.$(OBJ) : $(GLSRC)gxpcmap.c $(GXERR) $(math__h) $(memory__h)\
 $(gsstruct_h) $(gsutil_h)\
 $(gxcolor2_h) $(gxcspace_h) $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h)\
 $(gxfixed_h) $(gxmatrix_h) $(gxpcolor_h)\
 $(gzstate_h)
	$(GLCC) $(GLO_)gxpcmap.$(OBJ) $(C_) $(GLSRC)gxpcmap.c

# ---------------- PostScript Type 1 (and Type 4) fonts ---------------- #

type1lib_=$(GLOBJ)gxtype1.$(OBJ)\
 $(GLOBJ)gxhintn.$(OBJ) $(GLOBJ)gscrypt1.$(OBJ) $(GLOBJ)gxchrout.$(OBJ)

gscrypt1_h=$(GLSRC)gscrypt1.h
gstype1_h=$(GLSRC)gstype1.h
gxfont1_h=$(GLSRC)gxfont1.h $(gstype1_h) $(gxfixed_h)
gxtype1_h=$(GLSRC)gxtype1.h $(gscrypt1_h) $(gsgdata_h) $(gstype1_h) $(gxhintn_h)

$(GLOBJ)gxtype1.$(OBJ) : $(GLSRC)gxtype1.c $(GXERR) $(math__h)\
 $(gsccode_h) $(gsline_h) $(gsstruct_h)\
 $(gxarith_h) $(gxchrout_h) $(gxcoord_h) $(gxfixed_h) $(gxmatrix_h)\
 $(gxfont_h) $(gxfont1_h) $(gxistate_h) $(gxtype1_h)\
 $(gzpath_h)
	$(GLCC) $(GLO_)gxtype1.$(OBJ) $(C_) $(GLSRC)gxtype1.c

$(GLOBJ)gxhintn.$(OBJ) : $(GLSRC)gxhintn.c $(GXERR)\
 $(gxfixed_h) $(gxarith_h) $(gstypes_h) $(gxmatrix_h)\
 $(gxpath_h) $(gzpath_h) $(gxhintn_h) $(gxfont_h) $(gxfont1_h) $(gxtype1_h)\
 $(vdtrace_h)
	$(GLCC) $(GLO_)gxhintn.$(OBJ) $(C_) $(GLSRC)gxhintn.c

# CharString and eexec encryption

# Note that seexec is not needed for rasterizing Type 1/2/4 fonts,
# only for reading or writing them.
seexec_=$(GLOBJ)seexec.$(OBJ) $(GLOBJ)gscrypt1.$(OBJ)
$(GLD)seexec.dev : $(LIB_MAK) $(ECHOGS_XE) $(seexec_)
	$(SETMOD) $(GLD)seexec $(seexec_)

$(GLOBJ)seexec.$(OBJ) : $(GLSRC)seexec.c $(AK) $(stdio__h)\
 $(gscrypt1_h) $(scanchar_h) $(sfilter_h) $(strimpl_h)
	$(GLCC) $(GLO_)seexec.$(OBJ) $(C_) $(GLSRC)seexec.c

$(GLOBJ)gscrypt1.$(OBJ) : $(GLSRC)gscrypt1.c $(stdpre_h)\
 $(gscrypt1_h) $(gstypes_h)
	$(GLCC) $(GLO_)gscrypt1.$(OBJ) $(C_) $(GLSRC)gscrypt1.c

# Type 1 charstrings

psf1lib_=$(GLOBJ)gstype1.$(OBJ)
$(GLD)psf1lib.dev : $(LIB_MAK) $(ECHOGS_XE) $(psf1lib_) $(type1lib_)
	$(SETMOD) $(GLD)psf1lib $(psf1lib_)
	$(ADDMOD) $(GLD)psf1lib $(type1lib_)

$(GLOBJ)gstype1.$(OBJ) : $(GLSRC)gstype1.c $(GXERR) $(math__h) $(memory__h)\
 $(gsstruct_h)\
 $(gxarith_h) $(gxcoord_h) $(gxfixed_h) $(gxmatrix_h)\
 $(gxfont_h) $(gxfont1_h) $(gxistate_h) $(gxtype1_h)\
 $(gxpath_h)
	$(GLCC) $(GLO_)gstype1.$(OBJ) $(C_) $(GLSRC)gstype1.c

# Type 2 charstrings

psf2lib_=$(GLOBJ)gstype2.$(OBJ)
$(GLD)psf2lib.dev : $(LIB_MAK) $(ECHOGS_XE) $(psf2lib_) $(type1lib_)
	$(SETMOD) $(GLD)psf2lib $(psf2lib_)
	$(ADDMOD) $(GLD)psf2lib $(type1lib_)

$(GLOBJ)gstype2.$(OBJ) : $(GLSRC)gstype2.c $(GXERR) $(math__h) $(memory__h)\
 $(gsstruct_h)\
 $(gxarith_h) $(gxcoord_h) $(gxfixed_h) $(gxmatrix_h)\
 $(gxfont_h) $(gxfont1_h) $(gxistate_h) $(gxtype1_h)\
 $(gxpath_h) $(gxhintn_h)
	$(GLCC) $(GLO_)gstype2.$(OBJ) $(C_) $(GLSRC)gstype2.c

# ---------------- Font writing ---------------- #
# This is not really a library facility, but one piece of interpreter test
# code uses it.

# Support for PostScript and PDF font writing

gdevpsf_h=$(GLSRC)gdevpsf.h $(gsccode_h) $(gsgdata_h)

psf_1=$(GLOBJ)gdevpsf1.$(OBJ) $(GLOBJ)gdevpsf2.$(OBJ) $(GLOBJ)gdevpsfm.$(OBJ)
psf_2=$(GLOBJ)gdevpsft.$(OBJ) $(GLOBJ)gdevpsfu.$(OBJ) $(GLOBJ)gdevpsfx.$(OBJ)
psf_3=$(GLOBJ)spsdf.$(OBJ)
psf_=$(psf_1) $(psf_2) $(psf_3)
$(GLD)psf.dev : $(LIB_MAK) $(ECHOGS_XE) $(psf_)
	$(SETMOD) $(DD)psf $(psf_1)
	$(ADDMOD) $(DD)psf -obj $(psf_2)
	$(ADDMOD) $(DD)psf -obj $(psf_3)

$(GLOBJ)gdevpsf1.$(OBJ) : $(GLSRC)gdevpsf1.c $(GXERR) $(memory__h)\
 $(gsccode_h) $(gsmatrix_h)\
 $(gxfixed_h) $(gxfont_h) $(gxfont1_h) $(gxmatrix_h) $(gxtype1_h)\
 $(sfilter_h) $(sstring_h) $(stream_h) $(strimpl_h)\
 $(gdevpsf_h) $(spprint_h) $(spsdf_h)
	$(GLCC) $(GLO_)gdevpsf1.$(OBJ) $(C_) $(GLSRC)gdevpsf1.c

$(GLOBJ)gdevpsf2.$(OBJ) : $(GLSRC)gdevpsf2.c $(GXERR)\
 $(math__h) $(memory__h)\
 $(gsarith_h) $(gsccode_h) $(gscencs_h) $(gscrypt1_h) $(gsmatrix_h) $(gsutil_h)\
 $(gxfcid_h) $(gxfixed_h) $(gxfont_h) $(gxfont1_h)\
 $(sfilter_h) $(stream_h)\
 $(gdevpsf_h)
	$(GLCC) $(GLO_)gdevpsf2.$(OBJ) $(C_) $(GLSRC)gdevpsf2.c

$(GLOBJ)gdevpsfm.$(OBJ) : $(GLSRC)gdevpsfm.c $(GXERR)\
 $(gdevpsf_h)\
 $(gxfcmap_h)\
 $(spprint_h) $(spsdf_h) $(stream_h) $(memory__h)
	$(GLCC) $(GLO_)gdevpsfm.$(OBJ) $(C_) $(GLSRC)gdevpsfm.c

$(GLOBJ)gdevpsft.$(OBJ) : $(GLSRC)gdevpsft.c $(GXERR) $(memory__h)\
 $(gscencs_h) $(gsmatrix_h) $(gsutil_h)\
 $(gxfcid_h) $(gxfont_h) $(gxfont42_h) $(gxttf_h)\
 $(spprint_h) $(stream_h)\
 $(gdevpsf_h)
	$(GLCC) $(GLO_)gdevpsft.$(OBJ) $(C_) $(GLSRC)gdevpsft.c

$(GLOBJ)gdevpsfu.$(OBJ) : $(GLSRC)gdevpsfu.c $(GXERR) $(memory__h)\
 $(gsmatrix_h) $(gxfont_h) $(gdevpsf_h)
	$(GLCC) $(GLO_)gdevpsfu.$(OBJ) $(C_) $(GLSRC)gdevpsfu.c

$(GLOBJ)gdevpsfx.$(OBJ) : $(GLSRC)gdevpsfx.c $(GXERR)\
 $(math__h) $(memory__h)\
 $(gxfixed_h) $(gxfont_h) $(gxfont1_h) $(gxmatrix_h) $(gxtype1_h)\
 $(stream_h)\
 $(gdevpsf_h)
	$(GLCC) $(GLO_)gdevpsfx.$(OBJ) $(C_) $(GLSRC)gdevpsfx.c

# ---------------- Font copying ---------------- #

# This facility is not included in the core library.  Currently it is used
# only by pdfwrite.

fcopy_=$(GLOBJ)gxfcopy.$(OBJ)
$(GLD)fcopy.dev : $(DEVS_MAK) $(ECHOGS_XE) $(fcopy_)
	$(SETMOD) $(GLD)fcopy $(fcopy_)

$(GLOBJ)gxfcopy.$(OBJ) : $(GLSRC)gxfcopy.c $(memory__h) $(GXERR)\
 $(gscencs_h) $(gsline_h) $(gspaint_h) $(gspath_h) $(gsstruct_h) $(gsutil_h)\
 $(gxfont_h) $(gxfont1_h) $(gxfont42_h) $(gxfcid_h) $(gxfcopy_h) $(gxfcache_h)\
 $(gxistate_h) $(gxtext_h) $(gxtype1_h)\
 $(gzstate_h)\
 $(gdevpsf_h) $(stream_h)
	$(GLCC) $(GLO_)gxfcopy.$(OBJ) $(C_) $(GLSRC)gxfcopy.c

# -------- Level 1 color extensions (CMYK color and colorimage) -------- #

cmyklib_=$(GLOBJ)gscolor1.$(OBJ) $(GLOBJ)gsht1.$(OBJ)
$(GLD)cmyklib.dev : $(LIB_MAK) $(ECHOGS_XE) $(cmyklib_)
	$(SETMOD) $(GLD)cmyklib $(cmyklib_)

$(GLOBJ)gscolor1.$(OBJ) : $(GLSRC)gscolor1.c $(GXERR)\
 $(gsccolor_h) $(gscolor1_h) $(gsstruct_h) $(gsutil_h)\
 $(gxcmap_h) $(gxcspace_h) $(gxdcconv_h) $(gxdevice_h)\
 $(gzstate_h) $(gxhttype_h)
	$(GLCC) $(GLO_)gscolor1.$(OBJ) $(C_) $(GLSRC)gscolor1.c

$(GLOBJ)gsht1.$(OBJ) : $(GLSRC)gsht1.c $(GXERR) $(memory__h) $(string__h)\
 $(gsstruct_h) $(gsutil_h) $(gxdevice_h) $(gzht_h) $(gzstate_h)\
 $(gxwts_h) $(gswts_h)
	$(GLCC) $(GLO_)gsht1.$(OBJ) $(C_) $(GLSRC)gsht1.c

colimlib_=$(GLOBJ)gxicolor.$(OBJ)
$(GLD)colimlib.dev : $(LIB_MAK) $(ECHOGS_XE) $(colimlib_)
	$(SETMOD) $(GLD)colimlib $(colimlib_)
	$(ADDMOD) $(GLD)colimlib -imageclass 4_color

$(GLOBJ)gxicolor.$(OBJ) : $(GLSRC)gxicolor.c $(GXERR) $(memory__h) $(gpcheck_h)\
 $(gsccolor_h) $(gspaint_h)\
 $(gxarith_h) $(gxcmap_h) $(gxcpath_h) $(gxdcconv_h) $(gxdcolor_h)\
 $(gxdevice_h) $(gxdevmem_h) $(gxfixed_h) $(gxfrac_h)\
 $(gximage_h) $(gxistate_h) $(gxmatrix_h)\
 $(gzstate_h)
	$(GLCC) $(GLO_)gxicolor.$(OBJ) $(C_) $(GLSRC)gxicolor.c

# ---- Level 1 path miscellany (arcs, pathbbox, path enumeration) ---- #

path1lib_=$(GLOBJ)gspath1.$(OBJ)
$(GLD)path1lib.dev : $(LIB_MAK) $(ECHOGS_XE) $(path1lib_)
	$(SETMOD) $(GLD)path1lib $(path1lib_)

$(GLOBJ)gspath1.$(OBJ) : $(GLSRC)gspath1.c $(GXERR) $(math__h)\
 $(gscoord_h) $(gspath_h) $(gsstruct_h)\
 $(gxfarith_h) $(gxfixed_h) $(gxmatrix_h)\
 $(gzstate_h) $(gzpath_h)
	$(GLCC) $(GLO_)gspath1.$(OBJ) $(C_) $(GLSRC)gspath1.c

# --------------- Level 2 color space and color image support --------------- #
# no12bit and n16bit are the default, psl2lib replaces no12bit and pdfread replaces no16bit

no12bit_=$(GLOBJ)gxino12b.$(OBJ)
$(GLD)no12bit.dev : $(LIB_MAK) $(ECHOGS_XE) $(no12bit_)
	$(SETMOD) $(GLD)no12bit $(no12bit_)

$(GLOBJ)gxino12b.$(OBJ) : $(GLSRC)gxino12b.c $(std_h)\
 $(gstypes_h) $(gxsample_h)
	$(GLCC) $(GLO_)gxino12b.$(OBJ) $(C_) $(GLSRC)gxino12b.c

psl2cs_=$(GLOBJ)gscolor2.$(OBJ)
$(GLD)psl2cs.dev : $(LIB_MAK) $(ECHOGS_XE) $(psl2cs_)
	$(SETMOD) $(GLD)psl2cs $(psl2cs_)

$(GLOBJ)gscolor2.$(OBJ) : $(GLSRC)gscolor2.c $(GXERR) $(memory__h)\
 $(gxarith_h) $(gxfixed_h) $(gxmatrix_h) $(gxcspace_h)\
 $(gxcolor2_h) $(gzstate_h) $(gxpcolor_h) $(stream_h)
	$(GLCC) $(GLO_)gscolor2.$(OBJ) $(C_) $(GLSRC)gscolor2.c

psl2lib_=$(GLOBJ)gxi12bit.$(OBJ) $(GLOBJ)gxiscale.$(OBJ)
$(GLD)psl2lib.dev : $(LIB_MAK) $(ECHOGS_XE) $(psl2lib_)\
 $(GLD)colimlib.dev $(GLD)psl2cs.dev
	$(SETMOD) $(GLD)psl2lib $(psl2lib_)
	$(ADDMOD) $(GLD)psl2lib -imageclass 0_interpolate 2_fracs
	$(ADDMOD) $(GLD)psl2lib -include $(GLD)colimlib $(GLD)psl2cs
	$(ADDMOD) $(GLD)psl2lib -replace $(GLD)no12bit

$(GLOBJ)gxi12bit.$(OBJ) : $(GLSRC)gxi12bit.c $(GXERR)\
 $(memory__h) $(gpcheck_h)\
 $(gsccolor_h) $(gspaint_h)\
 $(gxarith_h) $(gxcmap_h) $(gxcpath_h) $(gxdcolor_h) $(gxdevice_h)\
 $(gxdevmem_h) $(gxfixed_h) $(gxfrac_h) $(gximage_h) $(gxistate_h)\
 $(gxmatrix_h)
	$(GLCC) $(GLO_)gxi12bit.$(OBJ) $(C_) $(GLSRC)gxi12bit.c

no16bit_=$(GLOBJ)gxino16b.$(OBJ)
$(GLD)no16bit.dev : $(LIB_MAK) $(ECHOGS_XE) $(no16bit_)
	$(SETMOD) $(GLD)no16bit $(no16bit_)

$(GLOBJ)gxino16b.$(OBJ) : $(GLSRC)gxino16b.c $(std_h)\
 $(gstypes_h) $(gxsample_h)
	$(GLCC) $(GLO_)gxino16b.$(OBJ) $(C_) $(GLSRC)gxino16b.c

$(GLOBJ)gxi16bit.$(OBJ) : $(GLSRC)gxi16bit.c $(GXERR)\
 $(memory__h) $(gpcheck_h)\
 $(gsccolor_h) $(gspaint_h)\
 $(gxarith_h) $(gxcmap_h) $(gxcpath_h) $(gxdcolor_h) $(gxdevice_h)\
 $(gxdevmem_h) $(gxfixed_h) $(gxfrac_h) $(gximage_h) $(gxistate_h)\
 $(gxmatrix_h)
	$(GLCC) $(GLO_)gxi16bit.$(OBJ) $(C_) $(GLSRC)gxi16bit.c

$(GLOBJ)gxiscale.$(OBJ) : $(GLSRC)gxiscale.c $(GXERR)\
 $(math__h) $(memory__h) $(gpcheck_h)\
 $(gsccolor_h) $(gspaint_h)\
 $(gxarith_h) $(gxcmap_h) $(gxcpath_h) $(gxdcolor_h) $(gxdevice_h)\
 $(gxdevmem_h) $(gxfixed_h) $(gxfrac_h) $(gximage_h) $(gxistate_h)\
 $(gxmatrix_h)\
 $(siinterp_h) $(siscale_h) $(stream_h)
	$(GLCC) $(GLO_)gxiscale.$(OBJ) $(C_) $(GLSRC)gxiscale.c

# ---------------- Display Postscript / Level 2 support ---------------- #

dps2lib_=$(GLOBJ)gsdps1.$(OBJ)
$(GLD)dps2lib.dev : $(LIB_MAK) $(ECHOGS_XE) $(dps2lib_)
	$(SETMOD) $(GLD)dps2lib $(dps2lib_)

$(GLOBJ)gsdps1.$(OBJ) : $(GLSRC)gsdps1.c $(GXERR) $(math__h)\
 $(gsmatrix_h) $(gscoord_h) $(gspaint_h) $(gxdevice_h)\
 $(gxfixed_h) $(gxmatrix_h) $(gspath_h) $(gspath2_h)\
 $(gxdevice_h) $(gxfixed_h) $(gxmatrix_h) $(gxhldevc_h)\
 $(gzpath_h) $(gzcpath_h) $(gzstate_h)
	$(GLCC) $(GLO_)gsdps1.$(OBJ) $(C_) $(GLSRC)gsdps1.c

# ---------------- Functions ---------------- #
# These are used by PDF and by LanguageLevel 3.  They are here because
# the implementation of Separation colors also uses them.

gsdsrc_h=$(GLSRC)gsdsrc.h $(gsstruct_h)
gsfunc_h=$(GLSRC)gsfunc.h $(gstypes_h)
gsfunc0_h=$(GLSRC)gsfunc0.h $(gsdsrc_h) $(gsfunc_h)
gxfunc_h=$(GLSRC)gxfunc.h $(gsfunc_h) $(gsstruct_h)

# Generic support, and FunctionType 0.
funclib_=$(GLOBJ)gsdsrc.$(OBJ) $(GLOBJ)gsfunc.$(OBJ) $(GLOBJ)gsfunc0.$(OBJ)
$(GLD)funclib.dev : $(LIB_MAK) $(ECHOGS_XE) $(funclib_)
	$(SETMOD) $(GLD)funclib $(funclib_)

$(GLOBJ)gsdsrc.$(OBJ) : $(GLSRC)gsdsrc.c $(GX) $(memory__h)\
 $(gsdsrc_h) $(gserrors_h) $(stream_h)
	$(GLCC) $(GLO_)gsdsrc.$(OBJ) $(C_) $(GLSRC)gsdsrc.c

$(GLOBJ)gsfunc.$(OBJ) : $(GLSRC)gsfunc.c $(GX) $(memory__h)\
 $(gserrors_h) $(gsparam_h) $(gxfunc_h) $(stream_h)
	$(GLCC) $(GLO_)gsfunc.$(OBJ) $(C_) $(GLSRC)gsfunc.c

$(GLOBJ)gsfunc0.$(OBJ) : $(GLSRC)gsfunc0.c $(GX) $(math__h)\
 $(gserrors_h) $(gsfunc0_h) $(gsparam_h) $(gxfarith_h) $(gxfunc_h)
	$(GLCC) $(GLO_)gsfunc0.$(OBJ) $(C_) $(GLSRC)gsfunc0.c

# FunctionType 4 may be is used for tintTransform and similar functions,
# in LanguageLevel 2 and above.

gsfunc4_h=$(GLSRC)gsfunc4.h $(gsfunc_h)

func4lib_=$(GLOBJ)gsfunc4.$(OBJ) $(GLOBJ)spprint.$(OBJ)
$(GLD)func4lib.dev : $(LIB_MAK) $(ECHOGS_XE) $(func4lib_) $(GLD)funclib.dev
	$(SETMOD) $(GLD)func4lib $(func4lib_)
	$(ADDMOD) $(GLD)func4lib -include $(GLD)funclib

$(GLOBJ)gsfunc4.$(OBJ) : $(GLSRC)gsfunc4.c $(GX) $(math__h) $(memory__h)\
 $(gsdsrc_h) $(gserrors_h) $(gsfunc4_h)\
 $(gxfarith_h) $(gxfunc_h)\
 $(sfilter_h) $(spprint_h) $(stream_h) $(strimpl_h)
	$(GLCC) $(GLO_)gsfunc4.$(OBJ) $(C_) $(GLSRC)gsfunc4.c

# ---------------- DevicePixel color space ---------------- #

gscpixel_h=$(GLSRC)gscpixel.h

cspixlib_=$(GLOBJ)gscpixel.$(OBJ)
$(GLD)cspixlib.dev : $(LIB_MAK) $(ECHOGS_XE) $(cspixlib_)
	$(SETMOD) $(GLD)cspixlib $(cspixlib_)

$(GLOBJ)gscpixel.$(OBJ) : $(GLSRC)gscpixel.c $(GXERR)\
 $(gsrefct_h) $(gxcspace_h) $(gscpixel_h) $(gxdevice_h) $(gxistate_h)\
 $(gsovrc_h) $(gsstate_h) $(gzstate_h) $(stream_h)
	$(GLCC) $(GLO_)gscpixel.$(OBJ) $(C_) $(GLSRC)gscpixel.c

# ---------------- CIE color ---------------- #

cielib1_=$(GLOBJ)gscie.$(OBJ) $(GLOBJ)gsciemap.$(OBJ) $(GLOBJ)gscscie.$(OBJ)
cielib2_=$(GLOBJ)gscrd.$(OBJ) $(GLOBJ)gscrdp.$(OBJ) $(GLOBJ)gxctable.$(OBJ)
cielib_=$(cielib1_) $(cielib2_)
$(GLD)cielib.dev : $(LIB_MAK) $(ECHOGS_XE) $(cielib_)
	$(SETMOD) $(GLD)cielib $(cielib1_)
	$(ADDMOD) $(GLD)cielib $(cielib2_)

$(GLOBJ)gscie.$(OBJ) : $(GLSRC)gscie.c $(GXERR) $(math__h) $(memory__h)\
 $(gscolor2_h) $(gsmatrix_h) $(gsstruct_h)\
 $(gxarith_h) $(gxcie_h) $(gxcmap_h) $(gxcspace_h) $(gxdevice_h) $(gzstate_h)\
 $(gsicc_h)
	$(GLCC) $(GLO_)gscie.$(OBJ) $(C_) $(GLSRC)gscie.c

$(GLOBJ)gsciemap.$(OBJ) : $(GLSRC)gsciemap.c $(GXERR) $(math__h)\
 $(gxarith_h) $(gxcie_h) $(gxcmap_h) $(gxcspace_h) $(gxdevice_h) $(gxistate_h)
	$(GLCC) $(GLO_)gsciemap.$(OBJ) $(C_) $(GLSRC)gsciemap.c

$(GLOBJ)gscrd.$(OBJ) : $(GLSRC)gscrd.c $(GXERR)\
 $(math__h) $(memory__h) $(string__h)\
 $(gscdefs_h) $(gscolor2_h) $(gscrd_h) $(gsdevice_h)\
 $(gsmatrix_h) $(gsparam_h) $(gsstruct_h) $(gsutil_h) $(gxcspace_h)
	$(GLCC) $(GLO_)gscrd.$(OBJ) $(C_) $(GLSRC)gscrd.c

$(GLOBJ)gscrdp.$(OBJ) : $(GLSRC)gscrdp.c $(GXERR)\
 $(math__h) $(memory__h) $(string__h)\
 $(gscolor2_h) $(gscrdp_h) $(gsdevice_h) $(gsmatrix_h) $(gsstruct_h)\
 $(gxarith_h) $(gxcspace_h)
	$(GLCC) $(GLO_)gscrdp.$(OBJ) $(C_) $(GLSRC)gscrdp.c

$(GLOBJ)gscscie.$(OBJ) : $(GLSRC)gscscie.c $(GXERR) $(math__h)\
 $(gscolor2_h) $(gsmatrix_h) $(gsstruct_h)\
 $(gxarith_h) $(gxcie_h) $(gxcmap_h) $(gxcspace_h) $(gxdevice_h) $(gzstate_h)\
 $(stream_h)
	$(GLCC) $(GLO_)gscscie.$(OBJ) $(C_) $(GLSRC)gscscie.c

$(GLOBJ)gxctable.$(OBJ) : $(GLSRC)gxctable.c $(GX)\
 $(gxfixed_h) $(gxfrac_h) $(gxctable_h)
	$(GLCC) $(GLO_)gxctable.$(OBJ) $(C_) $(GLSRC)gxctable.c

# ---------------- ICCBased color ---------------- #

sicclib_=$(GLOBJ)gsicc.$(OBJ)
$(GLD)sicclib.dev : $(LIB_MAK) $(ECHOGS_XE) $(sicclib_) $(GLD)cielib.dev\
 $(ICCGENDIR)$(D)icclib.dev
	$(SETMOD) $(GLD)sicclib $(sicclib_)
	$(ADDMOD) $(GLD)sicclib -include $(ICCGENDIR)$(D)icclib.dev

# icc_h is defined in icclib.mak, which is included after lib.mak, so we
# can't make gsicc.$(OBJ) depend on it.  Instead, we make it depend on
# the compilation of icc.$(OBJ).
$(GLOBJ)gsicc.$(OBJ) : $(GLSRC)gsicc.c $(GXERR) $(math__h) $(memory__h)\
 $(gsstruct_h) $(stream_h) $(gxcspace_h) $(gxarith_h) $(gxcie_h)\
 $(gzstate_h) $(gsicc_h) $(ICCOBJDIR)$(D)icc.$(OBJ)
	$(GLICCCC) $(GLO_)gsicc.$(OBJ) $(C_) $(GLSRC)gsicc.c

# ---------------- Separation colors ---------------- #

seprlib_=$(GLOBJ)gscsepr.$(OBJ)
$(GLD)seprlib.dev : $(LIB_MAK) $(ECHOGS_XE) $(seprlib_)
	$(SETMOD) $(GLD)seprlib $(seprlib_)

$(GLOBJ)gscsepr.$(OBJ) : $(GLSRC)gscsepr.c $(GXERR) $(memory__h)\
 $(gsfunc_h) $(gsrefct_h) $(gsmatrix_h) $(gscsepr_h) $(gxcspace_h)\
 $(gxfixed_h) $(gxcolor2_h) $(gzstate_h) $(gscdevn_h) $(gxcdevn_h)\
 $(gxcmap_h) $(gxdevcli_h) $(gsovrc_h) $(stream_h)
	$(GLCC) $(GLO_)gscsepr.$(OBJ) $(C_) $(GLSRC)gscsepr.c

# ================ Display Postscript extensions ================ #

gsiparm2_h=$(GLSRC)gsiparm2.h $(gsiparam_h)
gsdps_h=$(GLSRC)gsdps.h $(gsiparm2_h)

# Display PostScript needs the DevicePixel color space to implement
# the PixelCopy option of ImageType 2 images.
dpslib_=$(GLOBJ)gsdps.$(OBJ) $(GLOBJ)gximage2.$(OBJ)
$(GLD)dpslib.dev : $(LIB_MAK) $(ECHOGS_XE) $(dpslib_) $(GLD)cspixlib.dev
	$(SETMOD) $(GLD)dpslib $(dpslib_)
	$(ADDMOD) $(GLD)dpslib -imagetype 2
	$(ADDMOD) $(GLD)dpslib -include $(GLD)cspixlib

$(GLOBJ)gsdps.$(OBJ) : $(GLSRC)gsdps.c $(GX)\
 $(gsdps_h) $(gserrors_h) $(gspath_h)\
 $(gxdevice_h) $(gzcpath_h) $(gzpath_h) $(gzstate_h)
	$(GLCC) $(GLO_)gsdps.$(OBJ) $(C_) $(GLSRC)gsdps.c

$(GLOBJ)gximage2.$(OBJ) : $(GLSRC)gximage2.c $(math__h) $(memory__h) $(GXERR)\
 $(gscpixel_h) $(gscoord_h) $(gscspace_h) $(gsdevice_h) $(gsiparm2_h)\
 $(gsmatrix_h) $(gxgetbit_h) $(gxiparam_h) $(gxpath_h)
	$(GLCC) $(GLO_)gximage2.$(OBJ) $(C_) $(GLSRC)gximage2.c

# ---------------- NeXT Display PostScript ---------------- #

gsalphac_h=$(GLSRC)gsalphac.h $(gscompt_h)
gsdpnext_h=$(GLSRC)gsdpnext.h $(gsalpha_h) $(gsalphac_h)

$(GLOBJ)gsalphac.$(OBJ) : $(GLSRC)gsalphac.c $(GXERR) $(memory__h)\
 $(gsalphac_h) $(gsiparam_h) $(gsutil_h)\
 $(gxalpha_h) $(gxcomp_h) $(gxdevice_h) $(gxgetbit_h) $(gxlum_h)
	$(GLCC) $(GLO_)gsalphac.$(OBJ) $(C_) $(GLSRC)gsalphac.c

$(GLOBJ)gximagec.$(OBJ) : $(GLSRC)gximagec.c $(GXERR)\
 $(gsdpnext_h) $(gxiparam_h)
	$(GLCC) $(GLO_)gximagec.$(OBJ) $(C_) $(GLSRC)gximagec.c

dpnxtlib_=$(GLOBJ)gsalphac.$(OBJ)
$(GLD)dpnxtlib.dev : $(LIB_MAK) $(ECHOGS_XE) $(dpnxtlib_)
	$(SETMOD) $(GLD)dpnxtlib $(dpnxtlib_)
	$(ADDCOMP) $(GLD)dpnxtlib alpha

# ================ PostScript LanguageLevel 3 support ================ #

$(GLOBJ)gscdevn.$(OBJ) : $(GLSRC)gscdevn.c $(GXERR) $(memory__h) $(string__h)\
 $(gscdevn_h) $(gsfunc_h) $(gsmatrix_h) $(gsrefct_h) $(gsstruct_h)\
 $(gxcspace_h) $(gxcdevn_h) $(gxfarith_h) $(gxfrac_h) $(gxcmap_h)\
 $(gxistate_h) $(gscoord_h) $(gzstate_h) $(gxdevcli_h) $(gsovrc_h) $(stream_h)
	$(GLCC) $(GLO_)gscdevn.$(OBJ) $(C_) $(GLSRC)gscdevn.c

$(GLOBJ)gxdevndi.$(OBJ) : $(GLSRC)gxdevndi.c $(GX)\
 $(gsstruct_h) $(gsdcolor_h)\
 $(gxcmap_h) $(gxdevice_h) $(gxdevndi_h) $(gxlum_h) $(gzht_h)
	$(GLCC) $(GLO_)gxdevndi.$(OBJ) $(C_) $(GLSRC)gxdevndi.c

$(GLOBJ)gsclipsr.$(OBJ) : $(GLSRC)gsclipsr.c $(GXERR)\
 $(gsclipsr_h) $(gsstruct_h) $(gxclipsr_h) $(gxfixed_h) $(gxpath_h)\
 $(gzstate_h)
	$(GLCC) $(GLO_)gsclipsr.$(OBJ) $(C_) $(GLSRC)gsclipsr.c

psl3lib_=$(GLOBJ)gsclipsr.$(OBJ) $(GLOBJ)gscdevn.$(OBJ) $(GLOBJ)gxdevndi.$(OBJ)

$(GLD)psl3lib.dev : $(LIB_MAK) $(ECHOGS_XE) $(psl3lib_)\
 $(GLD)imasklib.dev $(GLD)shadelib.dev $(GLD)gxfapiu$(UFST_BRIDGE).dev
	$(SETMOD) $(GLD)psl3lib $(psl3lib_)
	$(ADDMOD) $(GLD)psl3lib -include $(GLD)imasklib $(GLD)shadelib
	$(ADDMOD) $(GLD)psl3lib -include $(GLD)gxfapiu$(UFST_BRIDGE)

# ---------------- Trapping ---------------- #

gstrap_h=$(GLSRC)gstrap.h $(gsparam_h)

$(GLOBJ)gstrap.$(OBJ) : $(GLSRC)gstrap.c $(string__h) $(GXERR)\
 $(gsparamx_h) $(gstrap_h)
	$(GLCC) $(GLO_)gstrap.$(OBJ) $(C_) $(GLSRC)gstrap.c

traplib_=$(GLOBJ)gsparamx.$(OBJ) $(GLOBJ)gstrap.$(OBJ)
$(GLD)traplib.dev : $(LIB_MAK) $(ECHOGS_XE) $(traplib_)
	$(SETMOD) $(GLD)traplib $(traplib_)

# ---------------- Transparency ---------------- #

gstrans_h=$(GLSRC)gstrans.h $(gstparam_h) $(gxcomp_h)
gsipar3x_h=$(GLSRC)gsipar3x.h $(gsiparam_h) $(gsiparm3_h)
gximag3x_h=$(GLSRC)gximag3x.h $(gsipar3x_h) $(gxiparam_h)
gxblend_h=$(GLSRC)gxblend.h
gdevp14_h=$(GLSRC)gdevp14.h

$(GLOBJ)gstrans.$(OBJ) : $(GLSRC)gstrans.c $(GXERR)\
 $(math__h) $(memory__h) $(gdevp14_h)\
 $(gstrans_h) $(gsutil_h) $(gxdevcli_h) $(gzstate_h)
	$(GLCC) $(GLO_)gstrans.$(OBJ) $(C_) $(GLSRC)gstrans.c

$(GLOBJ)gximag3x.$(OBJ) : $(GLSRC)gximag3x.c $(GXERR) $(math__h) $(memory__h)\
 $(gdevbbox_h)\
 $(gsbitops_h) $(gscpixel_h) $(gscspace_h) $(gsstruct_h)\
 $(gxdevice_h) $(gxdevmem_h) $(gximag3x_h) $(gxistate_h)
	$(GLCC) $(GLO_)gximag3x.$(OBJ) $(C_) $(GLSRC)gximag3x.c

$(GLOBJ)gxblend.$(OBJ) : $(GLSRC)gxblend.c $(GX) $(memory__h)\
 $(gstparam_h) $(gxblend_h)
	$(GLCC) $(GLO_)gxblend.$(OBJ) $(C_) $(GLSRC)gxblend.c

$(GLOBJ)gdevp14.$(OBJ) : $(GLSRC)gdevp14.c $(GXERR) $(math__h) $(memory__h)\
 $(gscdefs_h) $(gsdevice_h) $(gsdfilt_h) $(gsimage_h)\
 $(gsstruct_h) $(gstparam_h)\
 $(gxblend_h) $(gxdcolor_h) $(gxdevice_h) $(gxiparam_h) $(gxistate_h)\
 $(gxtext_h) $(gsrect_h) $(gstrans_h) $(gsutil_h) $(gxcldev_h)\
 $(gzstate_h) $(gdevp14_h) $(gsovrc_h) $(gxcmap_h) $(gscolor1_h)
	$(GLCC) $(GLO_)gdevp14.$(OBJ) $(C_) $(GLSRC)gdevp14.c

translib_=$(GLOBJ)gstrans.$(OBJ) $(GLOBJ)gximag3x.$(OBJ)\
 $(GLOBJ)gxblend.$(OBJ) $(GLOBJ)gdevp14.$(OBJ) 
$(GLD)translib.dev : $(LIB_MAK) $(ECHOGS_XE) $(translib_) $(GLD)cspixlib.dev $(GLD)bboxutil.dev
	$(SETMOD) $(GLD)translib $(translib_)
	$(ADDMOD) $(GLD)translib -imagetype 3x
	$(ADDMOD) $(GLD)translib -include $(GLD)cspixlib $(GLD)bboxutil

# ---------------- Smooth shading ---------------- #

gscolor3_h=$(GLSRC)gscolor3.h
gsfunc3_h=$(GLSRC)gsfunc3.h $(gsdsrc_h) $(gsfunc_h)
gsshade_h=$(GLSRC)gsshade.h\
 $(gsccolor_h) $(gscspace_h) $(gsdsrc_h) $(gsfunc_h) $(gsmatrix_h)\
 $(gxfixed_h)
gxshade_h=$(GLSRC)gxshade.h $(gsshade_h) $(gxfixed_h) $(gxmatrix_h) $(stream_h)
gxshade4_h=$(GLSRC)gxshade4.h

$(GLOBJ)gscolor3.$(OBJ) : $(GLSRC)gscolor3.c $(GXERR)\
 $(gscolor3_h) $(gsmatrix_h) $(gsptype2_h)\
 $(gxcolor2_h) $(gxcspace_h) $(gxpaint_h) $(gxdcolor_h) $(gxpcolor_h)\
 $(gxshade_h) $(gzpath_h) $(gzstate_h)
	$(GLCC) $(GLO_)gscolor3.$(OBJ) $(C_) $(GLSRC)gscolor3.c

$(GLOBJ)gsfunc3.$(OBJ) : $(GLSRC)gsfunc3.c $(math__h) $(memory__h) $(GXERR)\
 $(gsfunc3_h) $(gsparam_h) $(gxfunc_h) $(stream_h)
	$(GLCC) $(GLO_)gsfunc3.$(OBJ) $(C_) $(GLSRC)gsfunc3.c

$(GLOBJ)gsptype2.$(OBJ) : $(GLSRC)gsptype2.c $(GXERR)\
 $(gscspace_h) $(gsshade_h) $(gsmatrix_h) $(gsstate_h)\
 $(gxcolor2_h) $(gxdcolor_h) $(gsptype2_h) $(gxpcolor_h)\
 $(gxstate_h) $(gzpath_h) $(gzstate_h)
	$(GLCC) $(GLO_)gsptype2.$(OBJ) $(C_) $(GLSRC)gsptype2.c

$(GLOBJ)gsshade.$(OBJ) : $(GLSRC)gsshade.c $(GXERR)\
 $(gscspace_h) $(gsstruct_h) $(gsptype2_h)\
 $(gxcspace_h) $(gxcpath_h) $(gxdcolor_h) $(gxdevcli_h) $(gxistate_h)\
 $(gxpaint_h) $(gxpath_h) $(gxshade_h) $(gxshade4_h)\
 $(gzcpath_h) $(gzpath_h)
	$(GLCC) $(GLO_)gsshade.$(OBJ) $(C_) $(GLSRC)gsshade.c

$(GLOBJ)gxshade.$(OBJ) : $(GLSRC)gxshade.c $(GXERR) $(math__h)\
 $(gsrect_h)\
 $(gxcspace_h) $(gscindex_h) $(gscie_h) \
 $(gxdevcli_h) $(gxistate_h) $(gxdht_h) $(gxpaint_h) $(gxshade_h) $(gxshade4_h)\
 $(gsicc_h)
	$(GLCC) $(GLO_)gxshade.$(OBJ) $(C_) $(GLSRC)gxshade.c

$(GLOBJ)gxshade1.$(OBJ) : $(GLSRC)gxshade1.c $(GXERR) $(math__h) $(memory__h)\
 $(gscoord_h) $(gsmatrix_h) $(gspath_h) $(gsptype2_h)\
 $(gxcspace_h) $(gxdcolor_h) $(gxfarith_h) $(gxfixed_h) $(gxistate_h)\
 $(gxpath_h) $(gxshade_h) $(gxshade4_h) $(gxdevcli_h)
	$(GLCC) $(GLO_)gxshade1.$(OBJ) $(C_) $(GLSRC)gxshade1.c

$(GLOBJ)gxshade4.$(OBJ) : $(GLSRC)gxshade4.c $(GXERR)\
 $(math__h) $(memory__h)\
 $(gscoord_h) $(gsmatrix_h) $(gsptype2_h)\
 $(gxcspace_h) $(gxdcolor_h) $(gxdevcli_h) $(gxistate_h) $(gxpath_h)\
 $(gxshade_h) $(gxshade4_h) $(vdtrace_h)
	$(GLCC) $(GLO_)gxshade4.$(OBJ) $(C_) $(GLSRC)gxshade4.c

$(GLOBJ)gxshade6.$(OBJ) : $(GLSRC)gxshade6.c $(GXERR) $(memory__h)\
 $(gscoord_h) $(gsmatrix_h)\
 $(gxcspace_h) $(gxdcolor_h) $(gxistate_h) $(gxshade_h) $(gxshade4_h) $(gxdevcli_h)\
 $(gxarith_h) $(gzpath_h) $(math__h) $(vdtrace_h)
	$(GLCC) $(GLO_)gxshade6.$(OBJ) $(C_) $(GLSRC)gxshade6.c

shadelib_1=$(GLOBJ)gscolor3.$(OBJ) $(GLOBJ)gsfunc3.$(OBJ) $(GLOBJ)gsptype2.$(OBJ) $(GLOBJ)gsshade.$(OBJ)
shadelib_2=$(GLOBJ)gxshade.$(OBJ) $(GLOBJ)gxshade1.$(OBJ) $(GLOBJ)gxshade4.$(OBJ) $(GLOBJ)gxshade6.$(OBJ)
shadelib_=$(shadelib_1) $(shadelib_2)
$(GLD)shadelib.dev : $(LIB_MAK) $(ECHOGS_XE) $(shadelib_)\
 $(GLD)funclib.dev $(GLD)patlib.dev
	$(SETMOD) $(GLD)shadelib $(shadelib_1)
	$(ADDMOD) $(GLD)shadelib -obj $(shadelib_2)
	$(ADDMOD) $(GLD)shadelib -include $(GLD)funclib $(GLD)patlib

# ---------------- Support for %rom% IODevice ----------------- #
# This is used to access compressed, compiled-in support files
# define the romfs.dev FEATURE
romfs_=$(GLOBJ)gsiorom.$(OBJ)
$(GLD)romfs.dev : $(LIB_MAK) $(ECHO_XE) $(romfs_)
	$(SETMOD) $(GLD)romfs $(romfs_)
	$(ADDMOD) $(GLD)romfs -iodev rom

# the following module is only included if the romfs.dev FEATURE is enabled
$(GLOBJ)gsiorom.$(OBJ) : $(GLSRC)gsiorom.c \
 $(std_h) $(gx_h) $(gserrors_h) $(gsstruct_h) $(gxiodev_h)
	$(GLCC) $(GLO_)gsiorom.$(OBJ) $(C_) $(GLSRC)gsiorom.c

# ---------------- Support for %disk IODevices ---------------- #
# The following module is included only if the diskn.dev FEATURE is included
$(GLOBJ)gsiodisk.$(OBJ) : $(GLSRC)gsiodisk.c $(GXERR)\
 $(errno__h) $(string__h) $(unistd__h)\
 $(gp_h) $(gscdefs_h) $(gsparam_h) $(gsstruct_h) $(gxiodev_h) $(gsutil_h)
	$(GLCC) $(GLO_)gsiodisk.$(OBJ) $(C_) $(GLSRC)gsiodisk.c

# ------------ Support for %macresource% IODevice ------------- #
# This is used to load native-format fonts on MacOS
# Define the macres.dev FEATURE
macres_=$(GLOBJ)gsiomacres.$(OBJ)
$(GLD)macres.dev : $(LIB_MAK) $(ECHOGS_XE) $(macres_)
	$(SETMOD) $(GLD)macres $(macres_)
	$(ADDMOD) $(GLD)macres -iodev macresource

# The following module is included only if the macres.dev FEATURE is enabled
$(GLOBJ)gsiomacres.$(OBJ) : $(GLSRC)gsiomacres.c \
 $(std_h) $(gstypes_h) $(gsmemory_h) $(gxiodev_h)
	$(GLCC) $(GLO_)gsiomacres.$(OBJ) $(C_) $(GLSRC)gsiomacres.c

# ---------------- Font API ---------------- #

# UFST bridge support :
# (This stuff fixes Agfa's reentrancy problem with UFST callbacks.)
# (To be removed after Agfa fixes the problem.)

UFST_INC_1=$(I_)$(UFST_ROOT)$(D)sys$(D)inc$(_I) $(I_)$(UFST_ROOT)$(D)rts$(D)inc$(_I) 
UFST_INC_=$(UFST_INC_1) $(I_)$(UFST_ROOT)$(D)rts$(D)fco$(_I) $(I_)$(UFST_ROOT)$(D)rts$(D)gray$(_I)

gxfapi_h=$(GLSRC)gxfapi.h

$(GLD)gxfapiu1.dev : $(LIB_MAK) $(ECHOGS_XE) $(GLOBJ)gxfapi.$(OBJ)
	$(SETMOD) $(GLD)gxfapiu1 $(GLOBJ)gxfapi.$(OBJ)

$(GLOBJ)gxfapi.$(OBJ) : $(GLSRC)gxfapi.c \
 $(gx_h) $(gxfapi_h) \
 $(UFST_ROOT)$(D)rts$(D)inc$(D)cgconfig.h\
 $(UFST_ROOT)$(D)rts$(D)inc$(D)shareinc.h\
 $(UFST_ROOT)$(D)sys$(D)inc$(D)port.h
	$(GLCC) $(UFST_CFLAGS) $(UFST_INC_) $(GLO_)gxfapi.$(OBJ) $(C_) $(GLSRC)gxfapi.c


# stub for UFST bridge support	:

$(GLD)gxfapiu.dev : $(LIB_MAK) $(ECHOGS_XE)
	$(SETMOD) $(GLD)gxfapiu


# ================ Platform-specific modules ================ #
# Platform-specific code doesn't really belong here: this is code that is
# shared among multiple platforms.

# Standard implementation of gp_getenv.
$(GLOBJ)gp_getnv.$(OBJ) : $(GLSRC)gp_getnv.c $(AK) $(stdio__h) $(string__h)\
 $(gp_h) $(gsmemory_h) $(gstypes_h)
	$(GLCC) $(GLO_)gp_getnv.$(OBJ) $(C_) $(GLSRC)gp_getnv.c

# File system implementation.

# MS-DOS file system, also used by Desqview/X.
$(GLOBJ)gp_dosfs.$(OBJ) : $(GLSRC)gp_dosfs.c $(AK) $(dos__h) $(gp_h) $(gpmisc_h) $(gx_h)
	$(GLCC) $(GLO_)gp_dosfs.$(OBJ) $(C_) $(GLSRC)gp_dosfs.c

# MS-DOS file enumeration, *not* used by Desqview/X.
$(GLOBJ)gp_dosfe.$(OBJ) : $(GLSRC)gp_dosfe.c $(AK)\
 $(dos__h) $(memory__h) $(stdio__h) $(string__h)\
 $(gstypes_h) $(gsmemory_h) $(gsstruct_h) $(gp_h) $(gsutil_h)
	$(GLCC) $(GLO_)gp_dosfe.$(OBJ) $(C_) $(GLSRC)gp_dosfe.c

# Unix(-like) file system, also used by Desqview/X.
$(GLOBJ)gp_unifs.$(OBJ) : $(GLSRC)gp_unifs.c $(AK)\
 $(memory__h) $(string__h) $(stdio__h) $(unistd__h) \
 $(gx_h) $(gp_h) $(gpmisc_h) $(gsstruct_h) $(gsutil_h) \
 $(stat__h) $(dirent__h)
	$(GLCC) $(GLO_)gp_unifs.$(OBJ) $(C_) $(GLSRC)gp_unifs.c

# Unix(-like) file name syntax, *not* used by Desqview/X.
$(GLOBJ)gp_unifn.$(OBJ) : $(GLSRC)gp_unifn.c $(AK) $(gx_h) $(gp_h) $(gpmisc_h)
	$(GLCC) $(GLO_)gp_unifn.$(OBJ) $(C_) $(GLSRC)gp_unifn.c

# Pipes.  These are actually the same on all platforms that have them.

pipe_=$(GLOBJ)gdevpipe.$(OBJ)
$(GLD)pipe.dev : $(LIB_MAK) $(ECHOGS_XE) $(pipe_)
	$(SETMOD) $(GLD)pipe $(pipe_)
	$(ADDMOD) $(GLD)pipe -iodev pipe

$(GLOBJ)gdevpipe.$(OBJ) : $(GLSRC)gdevpipe.c $(AK)\
 $(errno__h) $(pipe__h) $(stdio__h) $(string__h) \
 $(gserror_h) $(gserrors_h) $(gsmemory_h) $(gstypes_h) $(gxiodev_h)
	$(GLCC) $(GLO_)gdevpipe.$(OBJ) $(C_) $(GLSRC)gdevpipe.c

# Thread / semaphore / monitor implementation.

# Dummy implementation.
nosync_=$(GLOBJ)gp_nsync.$(OBJ)
$(GLD)nosync.dev : $(LIB_MAK) $(ECHOGS_XE) $(nosync_)
	$(SETMOD) $(GLD)nosync $(nosync_)

$(GLOBJ)gp_nsync.$(OBJ) : $(GLSRC)gp_nsync.c $(AK) $(std_h)\
 $(gpsync_h) $(gserror_h) $(gserrors_h)
	$(GLCC) $(GLO_)gp_nsync.$(OBJ) $(C_) $(GLSRC)gp_nsync.c

# POSIX pthreads-based implementation.
pthreads_=$(GLOBJ)gp_psync.$(OBJ)
$(GLD)posync.dev : $(LIB_MAK) $(ECHOGS_XE) $(pthreads_)
	$(SETMOD) $(GLD)posync $(pthreads_)
	$(ADDMOD) $(GLD)posync -replace $(GLD)nosync

$(GLOBJ)gp_psync.$(OBJ) : $(GLSRC)gp_psync.c $(AK) $(malloc__h) $(std_h)\
 $(gpsync_h) $(gserror_h) $(gserrors_h)
	$(GLCC) $(GLO_)gp_psync.$(OBJ) $(C_) $(GLSRC)gp_psync.c

# Other stuff.

# Other MS-DOS facilities.
$(GLOBJ)gp_msdos.$(OBJ) : $(GLSRC)gp_msdos.c $(AK)\
 $(dos__h) $(stdio__h) $(string__h)\
 $(gsmemory_h) $(gstypes_h) $(gp_h)
	$(GLCC) $(GLO_)gp_msdos.$(OBJ) $(C_) $(GLSRC)gp_msdos.c

# ================ Dependencies for auxiliary programs ================ #

GENARCH_DEPS=$(stdpre_h)
GENCONF_DEPS=$(stdpre_h)
GENDEV_DEPS=$(stdpre_h)
# For the included .c files, we need to include both the .c and the
# compiled file in the dependencies, to express the implicit dependency
# on all .h files included by those .c files.
GENHT_DEPS=$(malloc__h) $(stdio__h) $(string__h)\
 $(gscdefs_h) $(gsmemory_h)\
 $(gxbitmap_h) $(gxdht_h) $(gxhttile_h) $(gxtmap_h)\
 $(sstring_h) $(strimpl_h)\
 $(GLSRC)gxhtbit.c $(GLOBJ)gxhtbit.$(OBJ)\
 $(GLSRC)scantab.c $(GLOBJ)scantab.$(OBJ)\
 $(GLSRC)sstring.c $(GLOBJ)sstring.$(OBJ)
GENHT_CFLAGS=$(I_)$(GLI_)$(_I) $(GLF_)
# GENINIT_DEPS is in int.mak

# ============================= Main program ============================== #

# Main program for library testing

$(GLOBJ)gslib.$(OBJ) : $(GLSRC)gslib.c $(AK)\
 $(math__h) $(stdio__h) $(string__h)\
 $(gx_h) $(gp_h)\
 $(gsalloc_h) $(gserrors_h) $(gsmatrix_h)\
 $(gsrop_h) $(gsstate_h) $(gscspace_h)\
 $(gscdefs_h) $(gscie_h) $(gscolor2_h) $(gscoord_h) $(gscrd_h)\
 $(gshtx_h) $(gsiparm3_h) $(gsiparm4_h) $(gslib_h) $(gsparam_h)\
 $(gspaint_h) $(gspath_h) $(gspath2_h) $(gsstruct_h) $(gsutil_h)\
 $(gxalloc_h) $(gxdcolor_h) $(gxdevice_h) $(gxht_h)\
 $(gdevbbox_h) $(gdevcmap_h)
	$(GLCC) $(GLO_)gslib.$(OBJ) $(C_) $(GLSRC)gslib.c
