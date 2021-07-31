#    Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: int.mak,v 1.50.2.2 2002/01/17 06:57:55 dancoby Exp $
# (Platform-independent) makefile for PostScript and PDF language
# interpreters.
# Users of this makefile must define the following:
#	GLSRCDIR - the graphics library source directory
#	GLGENDIR - the directory for graphics library source files
#		generated during building
#	PSSRCDIR - the source directory
#	PSOBJDIR - the object code directory

PSSRC=$(PSSRCDIR)$(D)
PSLIB=$(PSLIBDIR)$(D)
PSGEN=$(PSGENDIR)$(D)
PSOBJ=$(PSOBJDIR)$(D)
PSO_=$(O_)$(PSOBJ)
PSI_=$(PSSRCDIR) $(II)$(PSGENDIR) $(II)$(GLI_)
PSF_=
PSCC=$(CC_) $(I_)$(PSI_)$(_I) $(PSF_)
PSCCLEAF=$(CC_LEAF) $(I_)$(PSI_)$(_I) $(PSF_)
# All top-level makefiles define PSD.
#PSD=$(PSGEN)

# Define the name of this makefile.
INT_MAK=$(PSSRC)int.mak

# ======================== Interpreter support ======================== #

# This is support code for all interpreters, not just PostScript and PDF.
# It knows about the PostScript data types, but isn't supposed to
# depend on anything outside itself.

errors_h=$(PSSRC)errors.h
iconf_h=$(PSSRC)iconf.h
idebug_h=$(PSSRC)idebug.h
# Having iddstack.h at this level is unfortunate, but unavoidable.
iddstack_h=$(PSSRC)iddstack.h
idict_h=$(PSSRC)idict.h $(iddstack_h)
idictdef_h=$(PSSRC)idictdef.h
idosave_h=$(PSSRC)idosave.h
igcstr_h=$(PSSRC)igcstr.h
inames_h=$(PSSRC)inames.h
iname_h=$(PSSRC)iname.h $(inames_h)
inameidx_h=$(PSSRC)inameidx.h $(gconfigv_h)
inamestr_h=$(PSSRC)inamestr.h $(inameidx_h)
ipacked_h=$(PSSRC)ipacked.h
iref_h=$(PSSRC)iref.h
isave_h=$(PSSRC)isave.h $(idosave_h)
isstate_h=$(PSSRC)isstate.h
istruct_h=$(PSSRC)istruct.h $(gsstruct_h)
iutil_h=$(PSSRC)iutil.h
ivmspace_h=$(PSSRC)ivmspace.h $(gsgc_h)
opdef_h=$(PSSRC)opdef.h
# Nested include files
ghost_h=$(PSSRC)ghost.h $(gx_h) $(iref_h)
igc_h=$(PSSRC)igc.h $(istruct_h)
imemory_h=$(PSSRC)imemory.h $(gsalloc_h) $(ivmspace_h)
ialloc_h=$(PSSRC)ialloc.h $(imemory_h)
iastruct_h=$(PSSRC)iastruct.h $(gxobj_h) $(ialloc_h)
iastate_h=$(PSSRC)iastate.h $(gxalloc_h) $(ialloc_h) $(istruct_h)
inamedef_h=$(PSSRC)inamedef.h\
 $(gsstruct_h) $(inameidx_h) $(inames_h) $(inamestr_h)
store_h=$(PSSRC)store.h $(ialloc_h) $(idosave_h)

GH=$(AK) $(ghost_h)

isupport1_=$(PSOBJ)ialloc.$(OBJ) $(PSOBJ)igc.$(OBJ) $(PSOBJ)igcref.$(OBJ) $(PSOBJ)igcstr.$(OBJ)
isupport2_=$(PSOBJ)ilocate.$(OBJ) $(PSOBJ)iname.$(OBJ) $(PSOBJ)isave.$(OBJ)
isupport_=$(isupport1_) $(isupport2_)
$(PSD)isupport.dev : $(INT_MAK) $(ECHOGS_XE) $(isupport_)
	$(SETMOD) $(PSD)isupport $(isupport1_)
	$(ADDMOD) $(PSD)isupport -obj $(isupport2_)

$(PSOBJ)ialloc.$(OBJ) : $(PSSRC)ialloc.c $(AK) $(memory__h) $(gx_h)\
 $(errors_h) $(gsstruct_h)\
 $(iastate_h) $(igc_h) $(ipacked_h) $(iref_h) $(iutil_h) $(ivmspace_h)\
 $(store_h)
	$(PSCC) $(PSO_)ialloc.$(OBJ) $(C_) $(PSSRC)ialloc.c

# igc.c, igcref.c, and igcstr.c should really be in the dpsand2 list,
# but since all the GC enumeration and relocation routines refer to them,
# it's too hard to separate them out from the Level 1 base.
$(PSOBJ)igc.$(OBJ) : $(PSSRC)igc.c $(GH) $(memory__h)\
 $(errors_h) $(gsexit_h) $(gsmdebug_h) $(gsstruct_h) $(gsutil_h)\
 $(iastate_h) $(idict_h) $(igc_h) $(igcstr_h) $(inamedef_h)\
 $(ipacked_h) $(isave_h) $(isstate_h) $(istruct_h) $(opdef_h)
	$(PSCC) $(PSO_)igc.$(OBJ) $(C_) $(PSSRC)igc.c

$(PSOBJ)igcref.$(OBJ) : $(PSSRC)igcref.c $(GH) $(memory__h)\
 $(gsexit_h) $(gsstruct_h)\
 $(iastate_h) $(idebug_h) $(igc_h) $(iname_h) $(ipacked_h) $(store_h)
	$(PSCC) $(PSO_)igcref.$(OBJ) $(C_) $(PSSRC)igcref.c

$(PSOBJ)igcstr.$(OBJ) : $(PSSRC)igcstr.c $(GH) $(memory__h)\
 $(gsmdebug_h) $(gsstruct_h) $(iastate_h) $(igcstr_h)
	$(PSCC) $(PSO_)igcstr.$(OBJ) $(C_) $(PSSRC)igcstr.c

$(PSOBJ)ilocate.$(OBJ) : $(PSSRC)ilocate.c $(GH) $(memory__h)\
 $(errors_h) $(gsexit_h) $(gsstruct_h)\
 $(iastate_h) $(idict_h) $(igc_h) $(igcstr_h) $(iname_h)\
 $(ipacked_h) $(isstate_h) $(iutil_h) $(ivmspace_h)\
 $(store_h)
	$(PSCC) $(PSO_)ilocate.$(OBJ) $(C_) $(PSSRC)ilocate.c

$(PSOBJ)iname.$(OBJ) : $(PSSRC)iname.c $(GH) $(memory__h) $(string__h)\
 $(gsstruct_h) $(gxobj_h)\
 $(errors_h) $(imemory_h) $(inamedef_h) $(isave_h) $(store_h)
	$(PSCC) $(PSO_)iname.$(OBJ) $(C_) $(PSSRC)iname.c

$(PSOBJ)isave.$(OBJ) : $(PSSRC)isave.c $(GH) $(memory__h)\
 $(errors_h) $(gsexit_h) $(gsstruct_h) $(gsutil_h)\
 $(iastate_h) $(iname_h) $(inamedef_h) $(isave_h) $(isstate_h) $(ivmspace_h)\
 $(ipacked_h) $(store_h) $(stream_h)
	$(PSCC) $(PSO_)isave.$(OBJ) $(C_) $(PSSRC)isave.c

### Include files

idparam_h=$(PSSRC)idparam.h
ilevel_h=$(PSSRC)ilevel.h
interp_h=$(PSSRC)interp.h
iparam_h=$(PSSRC)iparam.h $(gsparam_h)
isdata_h=$(PSSRC)isdata.h
istack_h=$(PSSRC)istack.h $(isdata_h)
istkparm_h=$(PSSRC)istkparm.h
iutil2_h=$(PSSRC)iutil2.h
oparc_h=$(PSSRC)oparc.h
opcheck_h=$(PSSRC)opcheck.h
opextern_h=$(PSSRC)opextern.h
# Nested include files
idsdata_h=$(PSSRC)idsdata.h $(isdata_h)
idstack_h=$(PSSRC)idstack.h $(iddstack_h) $(idsdata_h) $(istack_h)
iesdata_h=$(PSSRC)iesdata.h $(isdata_h)
iestack_h=$(PSSRC)iestack.h $(istack_h) $(iesdata_h)
iosdata_h=$(PSSRC)iosdata.h $(isdata_h)
iostack_h=$(PSSRC)iostack.h $(istack_h) $(iosdata_h)
icstate_h=$(PSSRC)icstate.h $(imemory_h) $(iref_h) $(idsdata_h) $(iesdata_h) $(iosdata_h)
iddict_h=$(PSSRC)iddict.h $(icstate_h) $(idict_h)
dstack_h=$(PSSRC)dstack.h $(icstate_h) $(idstack_h)
estack_h=$(PSSRC)estack.h $(icstate_h) $(iestack_h)
ostack_h=$(PSSRC)ostack.h $(icstate_h) $(iostack_h)
oper_h=$(PSSRC)oper.h $(errors_h) $(iutil_h) $(opcheck_h) $(opdef_h) $(opextern_h) $(ostack_h)

$(PSOBJ)idebug.$(OBJ) : $(PSSRC)idebug.c $(GH) $(string__h)\
 $(gxalloc_h)\
 $(idebug_h) $(idict_h) $(iname_h) $(istack_h) $(iutil_h) $(ivmspace_h)\
 $(opdef_h) $(ipacked_h)
	$(PSCC) $(PSO_)idebug.$(OBJ) $(C_) $(PSSRC)idebug.c

$(PSOBJ)idict.$(OBJ) : $(PSSRC)idict.c $(GH) $(math__h) $(string__h)\
 $(errors_h)\
 $(gxalloc_h)\
 $(iddstack_h) $(idebug_h) $(idict_h) $(idictdef_h)\
 $(imemory_h) $(iname_h) $(inamedef_h) $(ipacked_h) $(isave_h)\
 $(iutil_h) $(ivmspace_h) $(store_h)
	$(PSCC) $(PSO_)idict.$(OBJ) $(C_) $(PSSRC)idict.c

$(PSOBJ)idparam.$(OBJ) : $(PSSRC)idparam.c $(GH) $(memory__h) $(string__h) $(errors_h)\
 $(gsmatrix_h) $(gsuid_h)\
 $(idict_h) $(idparam_h) $(ilevel_h) $(imemory_h) $(iname_h) $(iutil_h)\
 $(oper_h) $(store_h)
	$(PSCC) $(PSO_)idparam.$(OBJ) $(C_) $(PSSRC)idparam.c

$(PSOBJ)idstack.$(OBJ) : $(PSSRC)idstack.c $(GH)\
 $(idebug_h) $(idict_h) $(idictdef_h) $(idstack_h) $(iname_h) $(inamedef_h)\
 $(ipacked_h) $(iutil_h) $(ivmspace_h)
	$(PSCC) $(PSO_)idstack.$(OBJ) $(C_) $(PSSRC)idstack.c

$(PSOBJ)iparam.$(OBJ) : $(PSSRC)iparam.c $(GH)\
 $(memory__h) $(string__h) $(errors_h)\
 $(ialloc_h) $(idict_h) $(iname_h) $(imemory_h) $(iparam_h) $(istack_h) $(iutil_h) $(ivmspace_h)\
 $(opcheck_h) $(oper_h) $(store_h)
	$(PSCC) $(PSO_)iparam.$(OBJ) $(C_) $(PSSRC)iparam.c

$(PSOBJ)istack.$(OBJ) : $(PSSRC)istack.c $(GH) $(memory__h)\
 $(errors_h) $(gsstruct_h) $(gsutil_h)\
 $(ialloc_h) $(istack_h) $(istkparm_h) $(istruct_h) $(iutil_h) $(ivmspace_h)\
 $(store_h)
	$(PSCC) $(PSO_)istack.$(OBJ) $(C_) $(PSSRC)istack.c

$(PSOBJ)iutil.$(OBJ) : $(PSSRC)iutil.c $(GH) $(math__h) $(memory__h) $(string__h)\
 $(gsccode_h) $(gsmatrix_h) $(gsutil_h) $(gxfont_h)\
 $(sstring_h) $(strimpl_h)\
 $(errors_h) $(idict_h) $(imemory_h) $(iutil_h) $(ivmspace_h)\
 $(iname_h) $(ipacked_h) $(oper_h) $(store_h)
	$(PSCC) $(PSO_)iutil.$(OBJ) $(C_) $(PSSRC)iutil.c

# ======================== PostScript Level 1 ======================== #

###### Include files

# Binary tokens are a Level 2 feature, but we need to refer to them
# in the scanner.
btoken_h=$(PSSRC)btoken.h
files_h=$(PSSRC)files.h
fname_h=$(PSSRC)fname.h
iapi_h=$(PSSRC)iapi.h
ichar_h=$(PSSRC)ichar.h
ichar1_h=$(PSSRC)ichar1.h
icharout_h=$(PSSRC)icharout.h
icolor_h=$(PSSRC)icolor.h
icremap_h=$(PSSRC)icremap.h $(gsccolor_h)
icsmap_h=$(PSSRC)icsmap.h
idisp_h=$(PSSRC)idisp.h
ifilter2_h=$(PSSRC)ifilter2.h
ifont_h=$(PSSRC)ifont.h $(gsccode_h) $(gsstype_h)
ifont1_h=$(PSSRC)ifont1.h
ifont2_h=$(PSSRC)ifont2.h
ifont42_h=$(PSSRC)ifont42.h
ifrpred_h=$(PSSRC)ifrpred.h
ifwpred_h=$(PSSRC)ifwpred.h
iht_h=$(PSSRC)iht.h
iimage_h=$(PSSRC)iimage.h
iinit_h=$(PSSRC)iinit.h
imain_h=$(PSSRC)imain.h $(gsexit_h)
imainarg_h=$(PSSRC)imainarg.h
iminst_h=$(PSSRC)iminst.h
iparray_h=$(PSSRC)iparray.h
iscanbin_h=$(PSSRC)iscanbin.h
iscannum_h=$(PSSRC)iscannum.h
istream_h=$(PSSRC)istream.h
itoken_h=$(PSSRC)itoken.h
main_h=$(PSSRC)main.h $(imain_h) $(iminst_h)
sbwbs_h=$(PSSRC)sbwbs.h
shcgen_h=$(PSSRC)shcgen.h
smtf_h=$(PSSRC)smtf.h
# Nested include files
bfont_h=$(PSSRC)bfont.h $(ifont_h)
icontext_h=$(PSSRC)icontext.h $(gsstype_h) $(icstate_h)
ifilter_h=$(PSSRC)ifilter.h $(istream_h) $(ivmspace_h)
igstate_h=$(PSSRC)igstate.h $(gsstate_h) $(gxstate_h) $(imemory_h) $(istruct_h)
iscan_h=$(PSSRC)iscan.h $(sa85x_h) $(sstring_h)
sbhc_h=$(PSSRC)sbhc.h $(shc_h)
# Include files for optional features
ibnum_h=$(PSSRC)ibnum.h

### Initialization and scanning

$(PSOBJ)iconfig.$(OBJ) : $(PSSRC)iconf.c $(stdio__h)\
 $(gconf_h) $(gsmemory_h) $(gstypes_h)\
 $(iminst_h) $(iref_h) $(ivmspace_h) $(opdef_h)
	$(RM_) $(PSGEN)iconfig.c
	$(CP_) $(gconfig_h) $(PSGEN)gconfig.h
	$(CP_) $(PSSRC)iconf.c $(PSGEN)iconfig.c
	$(PSCC) $(PSO_)iconfig.$(OBJ) $(C_) $(PSGEN)iconfig.c

$(PSOBJ)iinit.$(OBJ) : $(PSSRC)iinit.c $(GH) $(string__h)\
 $(gscdefs_h) $(gsexit_h) $(gsstruct_h)\
 $(dstack_h) $(errors_h) $(ialloc_h) $(iddict_h)\
 $(iinit_h) $(ilevel_h) $(iname_h) $(interp_h) $(opdef_h)\
 $(ipacked_h) $(iparray_h) $(iutil_h) $(ivmspace_h) $(store_h)
	$(PSCC) $(PSO_)iinit.$(OBJ) $(C_) $(PSSRC)iinit.c

$(PSOBJ)iscan.$(OBJ) : $(PSSRC)iscan.c $(GH) $(memory__h)\
 $(btoken_h) $(dstack_h) $(errors_h) $(files_h)\
 $(ialloc_h) $(idict_h) $(ilevel_h) $(iname_h) $(ipacked_h) $(iparray_h)\
 $(iscan_h) $(iscanbin_h) $(iscannum_h)\
 $(istruct_h) $(istream_h) $(iutil_h) $(ivmspace_h)\
 $(ostack_h) $(store_h)\
 $(sa85d_h) $(stream_h) $(strimpl_h) $(sfilter_h) $(scanchar_h)
	$(PSCC) $(PSO_)iscan.$(OBJ) $(C_) $(PSSRC)iscan.c

$(PSOBJ)iscannum.$(OBJ) : $(PSSRC)iscannum.c $(GH) $(math__h)\
 $(errors_h) $(iscannum_h) $(scanchar_h) $(scommon_h) $(store_h)
	$(PSCC) $(PSO_)iscannum.$(OBJ) $(C_) $(PSSRC)iscannum.c

### Streams

$(PSOBJ)sfilter1.$(OBJ) : $(PSSRC)sfilter1.c $(AK) $(stdio__h) $(memory__h)\
 $(sfilter_h) $(strimpl_h)
	$(PSCC) $(PSO_)sfilter1.$(OBJ) $(C_) $(PSSRC)sfilter1.c

###### Operators

OP=$(GH) $(oper_h)

### Non-graphics operators

$(PSOBJ)zarith.$(OBJ) : $(PSSRC)zarith.c $(OP) $(math__h) $(store_h)
	$(PSCC) $(PSO_)zarith.$(OBJ) $(C_) $(PSSRC)zarith.c

$(PSOBJ)zarray.$(OBJ) : $(PSSRC)zarray.c $(OP) $(memory__h)\
 $(ialloc_h) $(ipacked_h) $(store_h)
	$(PSCC) $(PSO_)zarray.$(OBJ) $(C_) $(PSSRC)zarray.c

$(PSOBJ)zcontrol.$(OBJ) : $(PSSRC)zcontrol.c $(OP) $(string__h)\
 $(estack_h) $(files_h) $(ipacked_h) $(iutil_h) $(store_h) $(stream_h)
	$(PSCC) $(PSO_)zcontrol.$(OBJ) $(C_) $(PSSRC)zcontrol.c

$(PSOBJ)zdict.$(OBJ) : $(PSSRC)zdict.c $(OP)\
 $(dstack_h) $(iddict_h) $(ilevel_h) $(iname_h) $(ipacked_h) $(ivmspace_h)\
 $(store_h)
	$(PSCC) $(PSO_)zdict.$(OBJ) $(C_) $(PSSRC)zdict.c

$(PSOBJ)zfile.$(OBJ) : $(PSSRC)zfile.c $(OP) $(memory__h) $(string__h) $(gp_h)\
 $(gscdefs_h) $(gsfname_h) $(gsstruct_h) $(gxalloc_h) $(gxiodev_h)\
 $(ialloc_h) $(estack_h) $(files_h) $(ilevel_h) $(interp_h) $(iutil_h)\
 $(isave_h) $(main_h) $(sfilter_h) $(stream_h) $(strimpl_h) $(store_h)
	$(PSCC) $(PSO_)zfile.$(OBJ) $(C_) $(PSSRC)zfile.c

$(PSOBJ)zfileio.$(OBJ) : $(PSSRC)zfileio.c $(OP) $(memory__h) $(gp_h)\
 $(estack_h) $(files_h) $(ifilter_h) $(interp_h) $(store_h)\
 $(stream_h) $(strimpl_h)\
 $(gsmatrix_h) $(gxdevice_h) $(gxdevmem_h)
	$(PSCC) $(PSO_)zfileio.$(OBJ) $(C_) $(PSSRC)zfileio.c

$(PSOBJ)zfilter.$(OBJ) : $(PSSRC)zfilter.c $(OP) $(memory__h)\
 $(gsstruct_h)\
 $(files_h) $(ialloc_h) $(idict_h) $(idparam_h) $(ifilter_h) $(ilevel_h)\
 $(sfilter_h) $(srlx_h) $(sstring_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)zfilter.$(OBJ) $(C_) $(PSSRC)zfilter.c

$(PSOBJ)zfproc.$(OBJ) : $(PSSRC)zfproc.c $(GH) $(memory__h)\
 $(oper_h)\
 $(estack_h) $(files_h) $(gsstruct_h) $(ialloc_h) $(ifilter_h) $(istruct_h)\
 $(store_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)zfproc.$(OBJ) $(C_) $(PSSRC)zfproc.c

$(PSOBJ)zgeneric.$(OBJ) : $(PSSRC)zgeneric.c $(OP) $(memory__h)\
 $(gsstruct_h)\
 $(dstack_h) $(estack_h) $(iddict_h) $(iname_h) $(ipacked_h) $(ivmspace_h)\
 $(store_h)
	$(PSCC) $(PSO_)zgeneric.$(OBJ) $(C_) $(PSSRC)zgeneric.c

$(PSOBJ)ziodev.$(OBJ) : $(PSSRC)ziodev.c $(OP)\
 $(memory__h) $(stdio__h) $(string__h)\
 $(gp_h) $(gpcheck_h)\
 $(gxiodev_h)\
 $(files_h) $(ialloc_h) $(iscan_h) $(ivmspace_h)\
 $(scanchar_h) $(store_h) $(stream_h) $(istream_h)
	$(PSCC) $(PSO_)ziodev.$(OBJ) $(C_) $(PSSRC)ziodev.c

$(PSOBJ)ziodevs$(STDIO_IMPLEMENTATION).$(OBJ) : $(PSSRC)ziodevs$(STDIO_IMPLEMENTATION).c $(OP) $(stdio__h)\
 $(gpcheck_h)\
 $(gxiodev_h)\
 $(files_h) $(store_h) $(stream_h)
	$(PSCC) $(PSO_)ziodevs$(STDIO_IMPLEMENTATION).$(OBJ) $(C_) $(PSSRC)ziodevs$(STDIO_IMPLEMENTATION).c

$(PSOBJ)zmath.$(OBJ) : $(PSSRC)zmath.c $(OP) $(math__h) $(gxfarith_h) $(store_h)
	$(PSCC) $(PSO_)zmath.$(OBJ) $(C_) $(PSSRC)zmath.c

$(PSOBJ)zmisc.$(OBJ) : $(PSSRC)zmisc.c $(OP) $(gscdefs_h) $(gp_h)\
 $(errno__h) $(memory__h) $(string__h)\
 $(ialloc_h) $(idict_h) $(dstack_h) $(iname_h) $(ivmspace_h) $(ipacked_h) $(store_h)
	$(PSCC) $(PSO_)zmisc.$(OBJ) $(C_) $(PSSRC)zmisc.c

$(PSOBJ)zpacked.$(OBJ) : $(PSSRC)zpacked.c $(OP)\
 $(ialloc_h) $(idict_h) $(ivmspace_h) $(iname_h) $(ipacked_h) $(iparray_h)\
 $(istack_h) $(store_h)
	$(PSCC) $(PSO_)zpacked.$(OBJ) $(C_) $(PSSRC)zpacked.c

$(PSOBJ)zrelbit.$(OBJ) : $(PSSRC)zrelbit.c $(OP)\
 $(gsutil_h) $(store_h) $(idict_h)
	$(PSCC) $(PSO_)zrelbit.$(OBJ) $(C_) $(PSSRC)zrelbit.c

$(PSOBJ)zstack.$(OBJ) : $(PSSRC)zstack.c $(OP) $(memory__h)\
 $(ialloc_h) $(istack_h) $(store_h)
	$(PSCC) $(PSO_)zstack.$(OBJ) $(C_) $(PSSRC)zstack.c

$(PSOBJ)zstring.$(OBJ) : $(PSSRC)zstring.c $(OP) $(memory__h)\
 $(gsutil_h)\
 $(ialloc_h) $(iname_h) $(ivmspace_h) $(store_h)
	$(PSCC) $(PSO_)zstring.$(OBJ) $(C_) $(PSSRC)zstring.c

$(PSOBJ)zsysvm.$(OBJ) : $(PSSRC)zsysvm.c $(GH)\
 $(ialloc_h) $(ivmspace_h) $(oper_h) $(store_h)
	$(PSCC) $(PSO_)zsysvm.$(OBJ) $(C_) $(PSSRC)zsysvm.c

$(PSOBJ)ztoken.$(OBJ) : $(PSSRC)ztoken.c $(OP) $(string__h)\
 $(gsstruct_h)\
 $(dstack_h) $(estack_h) $(files_h)\
 $(idict_h) $(iname_h) $(iscan_h) $(itoken_h)\
 $(sfilter_h) $(store_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)ztoken.$(OBJ) $(C_) $(PSSRC)ztoken.c

$(PSOBJ)ztype.$(OBJ) : $(PSSRC)ztype.c $(OP)\
 $(math__h) $(memory__h) $(string__h)\
 $(gsexit_h)\
 $(dstack_h) $(idict_h) $(imemory_h) $(iname_h)\
 $(iscan_h) $(iutil_h) $(sfilter_h) $(store_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)ztype.$(OBJ) $(C_) $(PSSRC)ztype.c

$(PSOBJ)zvmem.$(OBJ) : $(PSSRC)zvmem.c $(OP)\
 $(dstack_h) $(estack_h) $(files_h)\
 $(ialloc_h) $(idict_h) $(igstate_h) $(isave_h) $(store_h) $(stream_h)\
 $(gsmalloc_h) $(gsmatrix_h) $(gsstate_h) $(gsstruct_h)
	$(PSCC) $(PSO_)zvmem.$(OBJ) $(C_) $(PSSRC)zvmem.c

### Graphics operators

$(PSOBJ)zbfont.$(OBJ) : $(PSSRC)zbfont.c $(OP) $(memory__h) $(string__h)\
 $(gsmatrix_h) $(gxdevice_h) $(gxfixed_h) $(gxfont_h)\
 $(bfont_h) $(ialloc_h) $(idict_h) $(idparam_h) $(ilevel_h)\
 $(iname_h) $(interp_h) $(istruct_h) $(ipacked_h) $(store_h)
	$(PSCC) $(PSO_)zbfont.$(OBJ) $(C_) $(PSSRC)zbfont.c

$(PSOBJ)zchar.$(OBJ) : $(PSSRC)zchar.c $(OP)\
 $(gsstruct_h) $(gstext_h) $(gxarith_h) $(gxfixed_h) $(gxmatrix_h)\
 $(gxdevice_h) $(gxfont_h) $(gzstate_h)\
 $(dstack_h) $(estack_h) $(ialloc_h) $(ichar_h) $(idict_h) $(ifont_h)\
 $(ilevel_h) $(iname_h) $(igstate_h) $(ipacked_h) $(store_h)
	$(PSCC) $(PSO_)zchar.$(OBJ) $(C_) $(PSSRC)zchar.c

# zcharout is used for Type 1 and Type 42 fonts only.
$(PSOBJ)zcharout.$(OBJ) : $(PSSRC)zcharout.c $(OP) $(memory__h)\
 $(gscrypt1_h) $(gstext_h) $(gxdevice_h) $(gxfont_h) $(gxfont1_h)\
 $(dstack_h) $(estack_h) $(ichar_h) $(icharout_h)\
 $(idict_h) $(ifont_h) $(igstate_h) $(iname_h) $(store_h)
	$(PSCC) $(PSO_)zcharout.$(OBJ) $(C_) $(PSSRC)zcharout.c

$(PSOBJ)zcolor.$(OBJ) : $(PSSRC)zcolor.c $(OP)\
 $(gxfixed_h) $(gxmatrix_h) $(gzstate_h) $(gxdevice_h) $(gxcmap_h)\
 $(ialloc_h) $(icolor_h) $(estack_h) $(iutil_h) $(igstate_h) $(store_h)
	$(PSCC) $(PSO_)zcolor.$(OBJ) $(C_) $(PSSRC)zcolor.c

$(PSOBJ)zdevice.$(OBJ) : $(PSSRC)zdevice.c $(OP) $(string__h)\
 $(ialloc_h) $(idict_h) $(igstate_h) $(iname_h) $(interp_h) $(iparam_h) $(ivmspace_h)\
 $(gsmatrix_h) $(gsstate_h) $(gxdevice_h) $(gxgetbit_h) $(store_h)
	$(PSCC) $(PSO_)zdevice.$(OBJ) $(C_) $(PSSRC)zdevice.c

$(PSOBJ)zdfilter.$(OBJ) : $(PSSRC)zdfilter.c $(OP) $(string__h)\
 $(ialloc_h) $(idict_h) $(igstate_h) $(iname_h) $(interp_h) $(iparam_h) $(ivmspace_h)\
 $(gsmatrix_h) $(gsstate_h) $(gxdevice_h) $(gxgetbit_h) $(store_h) $(gsdfilt_h)
	$(PSCC) $(PSO_)zdfilter.$(OBJ) $(C_) $(PSSRC)zdfilter.c

$(PSOBJ)zfont.$(OBJ) : $(PSSRC)zfont.c $(OP)\
 $(gsstruct_h) $(gxdevice_h) $(gxfont_h) $(gxfcache_h)\
 $(gzstate_h)\
 $(ialloc_h) $(iddict_h) $(igstate_h) $(iname_h) $(isave_h) $(ivmspace_h)\
 $(bfont_h) $(store_h)
	$(PSCC) $(PSO_)zfont.$(OBJ) $(C_) $(PSSRC)zfont.c

$(PSOBJ)zgstate.$(OBJ) : $(PSSRC)zgstate.c $(OP) $(math__h)\
 $(gsmatrix_h)\
 $(ialloc_h) $(icremap_h) $(idict_h) $(igstate_h) $(istruct_h) $(store_h)
	$(PSCC) $(PSO_)zgstate.$(OBJ) $(C_) $(PSSRC)zgstate.c

$(PSOBJ)zht.$(OBJ) : $(PSSRC)zht.c $(OP) $(memory__h)\
 $(gsmatrix_h) $(gsstate_h) $(gsstruct_h) $(gxdevice_h) $(gzht_h)\
 $(ialloc_h) $(estack_h) $(igstate_h) $(iht_h) $(store_h)
	$(PSCC) $(PSO_)zht.$(OBJ) $(C_) $(PSSRC)zht.c

$(PSOBJ)zimage.$(OBJ) : $(PSSRC)zimage.c $(OP) $(memory__h)\
 $(gscspace_h) $(gscssub_h) $(gsimage_h) $(gsmatrix_h) $(gsstruct_h)\
 $(gxiparam_h)\
 $(estack_h) $(ialloc_h) $(ifilter_h) $(igstate_h) $(iimage_h) $(ilevel_h)\
 $(store_h) $(stream_h)
	$(PSCC) $(PSO_)zimage.$(OBJ) $(C_) $(PSSRC)zimage.c

$(PSOBJ)zmatrix.$(OBJ) : $(PSSRC)zmatrix.c $(OP)\
 $(gsmatrix_h) $(igstate_h) $(gscoord_h) $(store_h)
	$(PSCC) $(PSO_)zmatrix.$(OBJ) $(C_) $(PSSRC)zmatrix.c

$(PSOBJ)zpaint.$(OBJ) : $(PSSRC)zpaint.c $(OP)\
 $(gspaint_h) $(igstate_h)
	$(PSCC) $(PSO_)zpaint.$(OBJ) $(C_) $(PSSRC)zpaint.c

$(PSOBJ)zpath.$(OBJ) : $(PSSRC)zpath.c $(OP) $(math__h)\
 $(gsmatrix_h) $(gspath_h) $(igstate_h) $(store_h)
	$(PSCC) $(PSO_)zpath.$(OBJ) $(C_) $(PSSRC)zpath.c

# Define the base PostScript language interpreter.
# This is the subset of PostScript Level 1 required by our PDF reader.

INT1=$(PSOBJ)iapi.$(OBJ) $(PSOBJ)icontext.$(OBJ) $(PSOBJ)idebug.$(OBJ)
INT2=$(PSOBJ)idict.$(OBJ) $(PSOBJ)idparam.$(OBJ) $(PSOBJ)idstack.$(OBJ)
INT3=$(PSOBJ)iinit.$(OBJ) $(PSOBJ)interp.$(OBJ)
INT4=$(PSOBJ)iparam.$(OBJ) $(PSOBJ)ireclaim.$(OBJ)
INT5=$(PSOBJ)iscan.$(OBJ) $(PSOBJ)iscannum.$(OBJ) $(PSOBJ)istack.$(OBJ)
INT6=$(PSOBJ)iutil.$(OBJ) $(GLOBJ)sa85d.$(OBJ) $(GLOBJ)scantab.$(OBJ)
INT7=$(PSOBJ)sfilter1.$(OBJ) $(GLOBJ)sstring.$(OBJ) $(GLOBJ)stream.$(OBJ)
Z1=$(PSOBJ)zarith.$(OBJ) $(PSOBJ)zarray.$(OBJ) $(PSOBJ)zcontrol.$(OBJ)
Z2=$(PSOBJ)zdict.$(OBJ) $(PSOBJ)zfile.$(OBJ) $(PSOBJ)zfileio.$(OBJ)
Z3=$(PSOBJ)zfilter.$(OBJ) $(PSOBJ)zfproc.$(OBJ) $(PSOBJ)zgeneric.$(OBJ)
Z4=$(PSOBJ)ziodev.$(OBJ) $(PSOBJ)ziodevs$(STDIO_IMPLEMENTATION).$(OBJ) $(PSOBJ)zmath.$(OBJ)
Z5=$(PSOBJ)zmisc.$(OBJ) $(PSOBJ)zpacked.$(OBJ) $(PSOBJ)zrelbit.$(OBJ)
Z6=$(PSOBJ)zstack.$(OBJ) $(PSOBJ)zstring.$(OBJ) $(PSOBJ)zsysvm.$(OBJ)
Z7=$(PSOBJ)ztoken.$(OBJ) $(PSOBJ)ztype.$(OBJ) $(PSOBJ)zvmem.$(OBJ)
Z8=$(PSOBJ)zbfont.$(OBJ) $(PSOBJ)zchar.$(OBJ) $(PSOBJ)zcolor.$(OBJ)
Z9=$(PSOBJ)zdevice.$(OBJ) $(PSOBJ)zfont.$(OBJ) $(PSOBJ)zgstate.$(OBJ) $(PSOBJ)zdfilter.$(OBJ)
Z10=$(PSOBJ)zht.$(OBJ) $(PSOBJ)zimage.$(OBJ) $(PSOBJ)zmatrix.$(OBJ)
Z11=$(PSOBJ)zpaint.$(OBJ) $(PSOBJ)zpath.$(OBJ)
Z1OPS=zarith zarray zcontrol1 zcontrol2 zcontrol3
Z2OPS=zdict1 zdict2 zfile zfileio1 zfileio2
Z3_4OPS=zfilter zfproc zgeneric ziodev zmath
Z5_6OPS=zmisc zpacked zrelbit zstack zstring zsysvm
Z7_8OPS=ztoken ztype zvmem zbfont zchar zcolor
Z9OPS=zdevice zfont zgstate1 zgstate2 zdfilter
Z10OPS=zht zimage zmatrix
Z11OPS=zpaint zpath
# We have to be a little underhanded with *config.$(OBJ) so as to avoid
# circular definitions.
INT_MAIN=$(PSOBJ)imain.$(OBJ) $(PSOBJ)imainarg.$(OBJ) $(GLOBJ)gsargs.$(OBJ) $(GLOBJ)idisp.$(OBJ)
INT_OBJS=$(INT_MAIN)\
 $(INT1) $(INT2) $(INT3) $(INT4) $(INT5) $(INT6) $(INT7)\
 $(Z1) $(Z2) $(Z3) $(Z4) $(Z5) $(Z6) $(Z7) $(Z8) $(Z9) $(Z10) $(Z11)
INT_CONFIG=$(GLOBJ)gconfig.$(OBJ) $(GLOBJ)gscdefs.$(OBJ)\
 $(PSOBJ)iconfig.$(OBJ) $(PSOBJ)iccinit$(COMPILE_INITS).$(OBJ)
INT_ALL=$(INT_OBJS) $(INT_CONFIG)
# We omit libcore.dev, which should be included here, because problems
# with the Unix linker require libcore to appear last in the link list
# when libcore is really a library.
# We omit $(INT_CONFIG) from the dependency list because they have special
# dependency requirements and are added to the link list at the very end.
# zfilter.c shouldn't include the RLE and RLD filters, but we don't want to
# change this now.
$(PSD)psbase.dev : $(INT_MAK) $(ECHOGS_XE) $(INT_OBJS)\
 $(PSD)isupport.dev $(PSD)nobtoken.dev $(PSD)nousparm.dev\
 $(GLD)rld.dev $(GLD)rle.dev $(GLD)sfile.dev
	$(SETMOD) $(PSD)psbase $(INT_MAIN)
	$(ADDMOD) $(PSD)psbase -obj $(INT_CONFIG)
	$(ADDMOD) $(PSD)psbase -obj $(INT1)
	$(ADDMOD) $(PSD)psbase -obj $(INT2)
	$(ADDMOD) $(PSD)psbase -obj $(INT3)
	$(ADDMOD) $(PSD)psbase -obj $(INT4)
	$(ADDMOD) $(PSD)psbase -obj $(INT5)
	$(ADDMOD) $(PSD)psbase -obj $(INT6)
	$(ADDMOD) $(PSD)psbase -obj $(INT7)
	$(ADDMOD) $(PSD)psbase -obj $(Z1)
	$(ADDMOD) $(PSD)psbase -obj $(Z2)
	$(ADDMOD) $(PSD)psbase -obj $(Z3)
	$(ADDMOD) $(PSD)psbase -obj $(Z4)
	$(ADDMOD) $(PSD)psbase -obj $(Z5)
	$(ADDMOD) $(PSD)psbase -obj $(Z6)
	$(ADDMOD) $(PSD)psbase -obj $(Z7)
	$(ADDMOD) $(PSD)psbase -obj $(Z8)
	$(ADDMOD) $(PSD)psbase -obj $(Z9)
	$(ADDMOD) $(PSD)psbase -obj $(Z10)
	$(ADDMOD) $(PSD)psbase -obj $(Z11)
	$(ADDMOD) $(PSD)psbase -oper $(Z1OPS)
	$(ADDMOD) $(PSD)psbase -oper $(Z2OPS)
	$(ADDMOD) $(PSD)psbase -oper $(Z3_4OPS)
	$(ADDMOD) $(PSD)psbase -oper $(Z5_6OPS)
	$(ADDMOD) $(PSD)psbase -oper $(Z7_8OPS)
	$(ADDMOD) $(PSD)psbase -oper $(Z9OPS)
	$(ADDMOD) $(PSD)psbase -oper $(Z10OPS)
	$(ADDMOD) $(PSD)psbase -oper $(Z11OPS)
	$(ADDMOD) $(PSD)psbase -iodev stdin stdout stderr lineedit statementedit
	$(ADDMOD) $(PSD)psbase -include $(PSD)isupport $(PSD)nobtoken $(PSD)nousparm
	$(ADDMOD) $(PSD)psbase -include $(GLD)rld $(GLD)rle $(GLD)sfile
	$(ADDMOD) $(PSD)psbase -replace $(GLD)gsiodevs

# -------------------------- Feature definitions -------------------------- #

# ---------------- Full Level 1 interpreter ---------------- #

# We keep the old name for backward compatibility.
$(PSD)level1.dev : $(PSD)psl1.dev
	$(CP_) $(PSD)psl1.dev $(PSD)level1.dev

$(PSD)psl1.dev : $(INT_MAK) $(ECHOGS_XE)\
 $(PSD)psbase.dev $(PSD)bcp.dev $(PSD)hsb.dev $(PSD)path1.dev $(PSD)type1.dev
	$(SETMOD) $(PSD)psl1 -include $(PSD)psbase $(PSD)bcp $(PSD)hsb $(PSD)path1 $(PSD)type1
	$(ADDMOD) $(PSD)psl1 -emulator PostScript PostScriptLevel1

# -------- Level 1 color extensions (CMYK color and colorimage) -------- #

$(PSD)color.dev : $(INT_MAK) $(ECHOGS_XE) $(GLD)cmyklib.dev $(GLD)colimlib.dev $(PSD)cmykread.dev
	$(SETMOD) $(PSD)color -include $(GLD)cmyklib $(GLD)colimlib $(PSD)cmykread

cmykread_=$(PSOBJ)zcolor1.$(OBJ) $(PSOBJ)zht1.$(OBJ)
$(PSD)cmykread.dev : $(INT_MAK) $(ECHOGS_XE) $(cmykread_)
	$(SETMOD) $(PSD)cmykread $(cmykread_)
	$(ADDMOD) $(PSD)cmykread -oper zcolor1 zht1

$(PSOBJ)zcolor1.$(OBJ) : $(PSSRC)zcolor1.c $(OP)\
 $(gscolor1_h) $(gscssub_h)\
 $(gxcmap_h) $(gxcspace_h) $(gxdevice_h) $(gxfixed_h) $(gxmatrix_h)\
 $(gzstate_h)\
 $(ialloc_h) $(icolor_h) $(iimage_h) $(estack_h) $(iutil_h) $(igstate_h) $(store_h)
	$(PSCC) $(PSO_)zcolor1.$(OBJ) $(C_) $(PSSRC)zcolor1.c

$(PSOBJ)zht1.$(OBJ) : $(PSSRC)zht1.c $(OP) $(memory__h)\
 $(gsmatrix_h) $(gsstate_h) $(gsstruct_h) $(gxdevice_h) $(gzht_h)\
 $(ialloc_h) $(estack_h) $(igstate_h) $(iht_h) $(store_h)
	$(PSCC) $(PSO_)zht1.$(OBJ) $(C_) $(PSSRC)zht1.c

# ---------------- DSC Parser ---------------- #

# The basic DSC parsing facility, used both for Orientation detection
# (to compensate for badly-written PostScript producers that don't emit
# the necessary setpagedevice calls) and by the PDF writer.

dscparse_h=$(PSSRC)dscparse.h

$(PSOBJ)zdscpars.$(OBJ) : $(PSSRC)zdscpars.c $(GH) $(memory__h) $(string__h)\
 $(dscparse_h) $(estack_h) $(ialloc_h) $(idict_h) $(iddict_h) $(iname_h)\
 $(iparam_h) $(istack_h) $(ivmspace_h) $(oper_h) $(store_h)\
 $(gsstruct_h)
	$(PSCC) $(PSO_)zdscpars.$(OBJ) $(C_) $(PSSRC)zdscpars.c

$(PSOBJ)dscparse.$(OBJ) : $(PSSRC)dscparse.c $(dscparse_h)
	$(PSCC) $(PSO_)dscparse.$(OBJ) $(C_) $(PSSRC)dscparse.c

dscparse_=$(PSOBJ)zdscpars.$(OBJ) $(PSOBJ)dscparse.$(OBJ)

$(PSD)dscparse.dev : $(INT_MAK) $(ECHOGS_XE) $(dscparse_)
	$(SETMOD) $(PSD)dscparse -obj $(dscparse_)
	$(ADDMOD) $(PSD)dscparse -oper zdscpars

# A feature to pass the Orientation information from the DSC comments
# to setpagedevice.

$(PSD)usedsc.dev : $(INT_MAK) $(ECHOGS_XE) $(PSD)dscparse.dev
	$(SETMOD) $(PSD)usedsc -include $(PSD)dscparse -ps gs_dscp

# ---------------- HSB color ---------------- #

hsb_=$(PSOBJ)zhsb.$(OBJ)
$(PSD)hsb.dev : $(INT_MAK) $(ECHOGS_XE) $(hsb_) $(GLD)hsblib.dev
	$(SETMOD) $(PSD)hsb $(hsb_)
	$(ADDMOD) $(PSD)hsb -include $(GLD)hsblib
	$(ADDMOD) $(PSD)hsb -oper zhsb

$(PSOBJ)zhsb.$(OBJ) : $(PSSRC)zhsb.c $(OP)\
 $(gshsb_h) $(igstate_h) $(store_h)
	$(PSCC) $(PSO_)zhsb.$(OBJ) $(C_) $(PSSRC)zhsb.c

# ---- Level 1 path miscellany (arcs, pathbbox, path enumeration) ---- #

path1_=$(PSOBJ)zpath1.$(OBJ)
$(PSD)path1.dev : $(INT_MAK) $(ECHOGS_XE) $(path1_) $(GLD)path1lib.dev
	$(SETMOD) $(PSD)path1 $(path1_)
	$(ADDMOD) $(PSD)path1 -include $(GLD)path1lib
	$(ADDMOD) $(PSD)path1 -oper zpath1

$(PSOBJ)zpath1.$(OBJ) : $(PSSRC)zpath1.c $(OP) $(memory__h)\
 $(ialloc_h) $(estack_h) $(gspath_h) $(gsstruct_h) $(igstate_h)\
 $(oparc_h) $(store_h)
	$(PSCC) $(PSO_)zpath1.$(OBJ) $(C_) $(PSSRC)zpath1.c

# ================ Level-independent PostScript options ================ #

# ---------------- BCP filters ---------------- #

bcp_=$(GLOBJ)sbcp.$(OBJ) $(PSOBJ)zfbcp.$(OBJ)
$(PSD)bcp.dev : $(INT_MAK) $(ECHOGS_XE) $(bcp_)
	$(SETMOD) $(PSD)bcp $(bcp_)
	$(ADDMOD) $(PSD)bcp -oper zfbcp

$(PSOBJ)zfbcp.$(OBJ) : $(PSSRC)zfbcp.c $(OP) $(memory__h)\
 $(gsstruct_h) $(ialloc_h) $(ifilter_h)\
 $(sbcp_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)zfbcp.$(OBJ) $(C_) $(PSSRC)zfbcp.c

# ---------------- Incremental font loading ---------------- #
# (This only works for Type 1 fonts without eexec encryption.)

$(PSD)diskfont.dev : $(INT_MAK) $(ECHOGS_XE)
	$(SETMOD) $(PSD)diskfont -ps gs_diskf

# ---------------- Double-precision floats ---------------- #

double_=$(PSOBJ)zdouble.$(OBJ)
$(PSD)double.dev : $(INT_MAK) $(ECHOGS_XE) $(double_)
	$(SETMOD) $(PSD)double $(double_)
	$(ADDMOD) $(PSD)double -oper zdouble1 zdouble2

$(PSOBJ)zdouble.$(OBJ) : $(PSSRC)zdouble.c $(OP)\
 $(ctype__h) $(math__h) $(memory__h) $(string__h)\
 $(gxfarith_h) $(store_h)
	$(PSCC) $(PSO_)zdouble.$(OBJ) $(C_) $(PSSRC)zdouble.c

# ---------------- EPSF files with binary headers ---------------- #

$(PSD)epsf.dev : $(INT_MAK) $(ECHOGS_XE)
	$(SETMOD) $(PSD)epsf -ps gs_epsf

# ---------------- RasterOp ---------------- #
# This should be a separable feature in the core also....

$(PSD)rasterop.dev : $(INT_MAK) $(ECHOGS_XE) $(GLD)roplib.dev $(PSD)ropread.dev
	$(SETMOD) $(PSD)rasterop -include $(GLD)roplib $(PSD)ropread

ropread_=$(PSOBJ)zrop.$(OBJ)
$(PSD)ropread.dev : $(INT_MAK) $(ECHOGS_XE) $(ropread_)
	$(SETMOD) $(PSD)ropread $(ropread_)
	$(ADDMOD) $(PSD)ropread -oper zrop

$(PSOBJ)zrop.$(OBJ) : $(PSSRC)zrop.c $(OP) $(memory__h)\
 $(gsrop_h) $(gsutil_h) $(gxdevice_h)\
 $(idict_h) $(idparam_h) $(igstate_h) $(store_h)
	$(PSCC) $(PSO_)zrop.$(OBJ) $(C_) $(PSSRC)zrop.c

# ---------------- PostScript Type 1 (and Type 4) fonts ---------------- #

$(PSD)type1.dev : $(INT_MAK) $(ECHOGS_XE) $(GLD)psf1lib.dev $(PSD)psf1read.dev
	$(SETMOD) $(PSD)type1 -include $(GLD)psf1lib $(PSD)psf1read

psf1read_1=$(PSOBJ)zchar1.$(OBJ) $(PSOBJ)zcharout.$(OBJ)
psf1read_2=$(PSOBJ)zfont1.$(OBJ) $(PSOBJ)zmisc1.$(OBJ)
psf1read_=$(psf1read_1) $(psf1read_2)
$(PSD)psf1read.dev : $(INT_MAK) $(ECHOGS_XE) $(psf1read_) $(GLD)seexec.dev
	$(SETMOD) $(PSD)psf1read $(psf1read_1)
	$(ADDMOD) $(PSD)psf1read -obj $(psf1read_2)
	$(ADDMOD) $(PSD)psf1read -include $(GLD)seexec
	$(ADDMOD) $(PSD)psf1read -oper zchar1 zfont1 zmisc1
	$(ADDMOD) $(PSD)psf1read -ps gs_type1

$(PSOBJ)zchar1.$(OBJ) : $(PSSRC)zchar1.c $(OP) $(memory__h)\
 $(gspaint_h) $(gspath_h) $(gsrect_h) $(gsstruct_h)\
 $(gxdevice_h) $(gxfixed_h) $(gxmatrix_h)\
 $(gxfont_h) $(gxfont1_h) $(gxtype1_h) $(gzstate_h)\
 $(estack_h) $(ialloc_h) $(ichar_h) $(ichar1_h) $(icharout_h)\
 $(idict_h) $(ifont_h) $(igstate_h) $(iname_h) $(store_h)
	$(PSCC) $(PSO_)zchar1.$(OBJ) $(C_) $(PSSRC)zchar1.c

$(PSOBJ)zfont1.$(OBJ) : $(PSSRC)zfont1.c $(OP) $(memory__h)\
 $(gsmatrix_h) $(gxdevice_h)\
 $(gxfixed_h) $(gxfont_h) $(gxfont1_h)\
 $(bfont_h) $(ialloc_h) $(ichar1_h) $(icharout_h) $(idict_h) $(idparam_h)\
 $(ifont1_h) $(iname_h) $(store_h)
	$(PSCC) $(PSO_)zfont1.$(OBJ) $(C_) $(PSSRC)zfont1.c

$(PSOBJ)zmisc1.$(OBJ) : $(PSSRC)zmisc1.c $(OP) $(memory__h)\
 $(gscrypt1_h)\
 $(idict_h) $(idparam_h) $(ifilter_h)\
 $(sfilter_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)zmisc1.$(OBJ) $(C_) $(PSSRC)zmisc1.c

# -------------- Compact Font Format and Type 2 charstrings ------------- #

$(PSD)cff.dev : $(INT_MAK) $(ECHOGS_XE) $(PSD)psl2int.dev
	$(SETMOD) $(PSD)cff -include $(PSD)psl2int -ps gs_css_e gs_cff

$(PSOBJ)zchar2.$(OBJ) : $(PSSRC)zchar2.c $(OP)\
 $(gxfixed_h) $(gxmatrix_h) $(gxfont_h) $(gxfont1_h) $(gxtype1_h)\
 $(ichar1_h)
	$(PSCC) $(PSO_)zchar2.$(OBJ) $(C_) $(PSSRC)zchar2.c

$(PSOBJ)zfont2.$(OBJ) : $(PSSRC)zfont2.c $(OP)\
 $(gsmatrix_h) $(gxfixed_h) $(gxfont_h) $(gxfont1_h)\
 $(bfont_h) $(idict_h) $(idparam_h) $(ifont1_h) $(ifont2_h)
	$(PSCC) $(PSO_)zfont2.$(OBJ) $(C_) $(PSSRC)zfont2.c

type2_=$(PSOBJ)zchar2.$(OBJ) $(PSOBJ)zfont2.$(OBJ)
$(PSD)type2.dev : $(INT_MAK) $(ECHOGS_XE) $(type2_)\
 $(PSD)type1.dev $(GLD)psf2lib.dev
	$(SETMOD) $(PSD)type2 $(type2_)
	$(ADDMOD) $(PSD)type2 -oper zchar2 zfont2
	$(ADDMOD) $(PSD)type2 -include $(PSD)type1 $(GLD)psf2lib

# ---------------- Type 32 (downloaded bitmap) fonts ---------------- #

$(PSOBJ)zchar32.$(OBJ) : $(PSSRC)zchar32.c $(OP)\
 $(gsccode_h) $(gsmatrix_h) $(gsutil_h)\
 $(gxfcache_h) $(gxfixed_h) $(gxfont_h)\
 $(ifont_h) $(igstate_h) $(store_h)
	$(PSCC) $(PSO_)zchar32.$(OBJ) $(C_) $(PSSRC)zchar32.c

$(PSOBJ)zfont32.$(OBJ) : $(PSSRC)zfont32.c $(OP)\
 $(gsccode_h) $(gsmatrix_h) $(gsutil_h) $(gxfont_h)\
 $(bfont_h) $(store_h)
	$(PSCC) $(PSO_)zfont32.$(OBJ) $(C_) $(PSSRC)zfont32.c

type32_=$(PSOBJ)zchar32.$(OBJ) $(PSOBJ)zfont32.$(OBJ)
$(PSD)type32.dev : $(INT_MAK) $(ECHOGS_XE) $(type32_)
	$(SETMOD) $(PSD)type32 $(type32_)
	$(ADDMOD) $(PSD)type32 -oper zchar32 zfont32
	$(ADDMOD) $(PSD)type32 -ps gs_res gs_typ32

# ---------------- TrueType and PostScript Type 42 fonts ---------------- #

# Mac glyph support (has an internal dependency)
$(PSD)macroman.dev : $(INT_MAK) $(ECHOGS_XE) $(PSLIB)gs_mro_e.ps
	$(SETMOD) $(PSD)macroman -ps gs_mro_e

$(PSD)macglyph.dev : $(INT_MAK) $(ECHOGS_XE) $(PSLIB)gs_mgl_e.ps\
 $(PSD)macroman.dev 
	$(SETMOD) $(PSD)macglyph -include $(PSD)macroman -ps gs_mgl_e

# Native TrueType support
$(PSD)ttfont.dev : $(INT_MAK) $(ECHOGS_XE) $(PSD)macglyph.dev $(PSD)type42.dev
	$(SETMOD) $(PSD)ttfont -include $(PSD)macglyph $(PSD)type42
	$(ADDMOD) $(PSD)ttfont -ps gs_wan_e gs_agl gs_ttf

# Type 42 (embedded TrueType) support
type42read_=$(PSOBJ)zchar42.$(OBJ) $(PSOBJ)zcharout.$(OBJ) $(PSOBJ)zfont42.$(OBJ)
$(PSD)type42.dev : $(INT_MAK) $(ECHOGS_XE) $(type42read_) $(GLD)ttflib.dev
	$(SETMOD) $(PSD)type42 $(type42read_)
	$(ADDMOD) $(PSD)type42 -include $(GLD)ttflib	
	$(ADDMOD) $(PSD)type42 -oper zchar42 zfont42
	$(ADDMOD) $(PSD)type42 -ps gs_typ42

$(PSOBJ)zchar42.$(OBJ) : $(PSSRC)zchar42.c $(OP)\
 $(gsmatrix_h) $(gspaint_h) $(gspath_h)\
 $(gxfixed_h) $(gxfont_h) $(gxfont42_h)\
 $(gxistate_h) $(gxpath_h) $(gxtext_h) $(gzstate_h)\
 $(dstack_h) $(estack_h) $(ichar_h) $(icharout_h)\
 $(ifont_h) $(igstate_h) $(store_h)
	$(PSCC) $(PSO_)zchar42.$(OBJ) $(C_) $(PSSRC)zchar42.c

$(PSOBJ)zfont42.$(OBJ) : $(PSSRC)zfont42.c $(OP) $(memory__h)\
 $(gsccode_h) $(gsmatrix_h) $(gxfont_h) $(gxfont42_h)\
 $(bfont_h) $(icharout_h) $(idict_h) $(idparam_h) $(ifont42_h) $(iname_h)\
 $(store_h)
	$(PSCC) $(PSO_)zfont42.$(OBJ) $(C_) $(PSSRC)zfont42.c

# ======================== Precompilation options ======================== #

# ---------------- Precompiled fonts ---------------- #
# See Fonts.htm for more information.

ccfont_h=$(PSSRC)ccfont.h $(stdpre_h) $(gsmemory_h)\
 $(iref_h) $(ivmspace_h) $(store_h)

CCFONT=$(OP) $(ccfont_h)

# List the fonts we are going to compile.
# Because of intrinsic limitations in `make', we have to list
# the object file names and the font names separately.
# Because of limitations in the DOS shell, we have to break the fonts up
# into lists that will fit on a single line (120 characters).
# The rules for constructing the .c files from the fonts themselves,
# and for compiling the .c files, are in cfonts.mak, not here.
# For example, to compile the Courier fonts, you should invoke
#	make Courier_o
# By convention, the names of the 35 standard compiled fonts use '0' for
# the foundry name.  This allows users to substitute different foundries
# without having to change this makefile.
ccfonts_ps=gs_ccfnt
ccfonts1_=$(PSOBJ)0agk.$(OBJ) $(PSOBJ)0agko.$(OBJ) $(PSOBJ)0agd.$(OBJ) $(PSOBJ)0agdo.$(OBJ)
ccfonts1=agk agko agd agdo
ccfonts2_=$(PSOBJ)0bkl.$(OBJ) $(PSOBJ)0bkli.$(OBJ) $(PSOBJ)0bkd.$(OBJ) $(PSOBJ)0bkdi.$(OBJ)
ccfonts2=bkl bkli bkd bkdi
ccfonts3_=$(PSOBJ)0crr.$(OBJ) $(PSOBJ)0cri.$(OBJ) $(PSOBJ)0crb.$(OBJ) $(PSOBJ)0crbi.$(OBJ)
ccfonts3=crr cri crb crbi
ccfonts4_=$(PSOBJ)0hvr.$(OBJ) $(PSOBJ)0hvro.$(OBJ) $(PSOBJ)0hvb.$(OBJ) $(PSOBJ)0hvbo.$(OBJ)
ccfonts4=hvr hvro hvb hvbo
ccfonts5_=$(PSOBJ)0hvrrn.$(OBJ) $(PSOBJ)0hvrorn.$(OBJ) $(PSOBJ)0hvbrn.$(OBJ) $(PSOBJ)0hvborn.$(OBJ)
ccfonts5=hvrrn hvrorn hvbrn hvborn
ccfonts6_=$(PSOBJ)0ncr.$(OBJ) $(PSOBJ)0ncri.$(OBJ) $(PSOBJ)0ncb.$(OBJ) $(PSOBJ)0ncbi.$(OBJ)
ccfonts6=ncr ncri ncb ncbi
ccfonts7_=$(PSOBJ)0plr.$(OBJ) $(PSOBJ)0plri.$(OBJ) $(PSOBJ)0plb.$(OBJ) $(PSOBJ)0plbi.$(OBJ)
ccfonts7=plr plri plb plbi
ccfonts8_=$(PSOBJ)0tmr.$(OBJ) $(PSOBJ)0tmri.$(OBJ) $(PSOBJ)0tmb.$(OBJ) $(PSOBJ)0tmbi.$(OBJ)
ccfonts8=tmr tmri tmb tmbi
ccfonts9_=$(PSOBJ)0syr.$(OBJ) $(PSOBJ)0zcmi.$(OBJ) $(PSOBJ)0zdr.$(OBJ)
ccfonts9=syr zcmi zdr
# The free distribution includes Bitstream Charter, Utopia, and
# freeware Cyrillic and Kana fonts.  We only provide for compiling
# Charter and Utopia.
ccfonts10free_=$(PSOBJ)bchr.$(OBJ) $(PSOBJ)bchri.$(OBJ) $(PSOBJ)bchb.$(OBJ) $(PSOBJ)bchbi.$(OBJ)
ccfonts10free=chr chri chb chbi
ccfonts11free_=$(PSOBJ)putr.$(OBJ) $(PSOBJ)putri.$(OBJ) $(PSOBJ)putb.$(OBJ) $(PSOBJ)putbi.$(OBJ)
ccfonts11free=utr utri utb utbi
# Uncomment the alternatives in the next 4 lines if you want
# Charter and Utopia compiled in.
#ccfonts10_=$(ccfonts10free_)
ccfonts10_=
#ccfonts10=$(ccfonts10free)
ccfonts10=
#ccfonts11_=$(ccfonts11free_)
ccfonts11_=
#ccfonts11=$(ccfonts11free)
ccfonts11=
# Add your own fonts here if desired.
ccfonts12_=
ccfonts12=
ccfonts13_=
ccfonts13=
ccfonts14_=
ccfonts14=
ccfonts15_=
ccfonts15=

# font2c has the prefix "gs" built into it, so we need to instruct
# genconf to use the same one.
$(gconfigf_h) : $(TOP_MAKEFILES) $(INT_MAK) $(ECHOGS_XE) $(GENCONF_XE)
	$(SETMOD) $(PSD)ccfonts_ -font $(ccfonts1)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts2)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts3)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts4)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts5)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts6)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts7)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts8)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts9)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts10)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts11)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts12)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts13)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts14)
	$(ADDMOD) $(PSD)ccfonts_ -font $(ccfonts15)
	$(EXP)$(GENCONF_XE) $(PSGEN)ccfonts_.dev -n gs -f $(gconfigf_h)

# We separate icfontab.dev from ccfonts.dev so that a customer can put
# compiled fonts into a separate shared library.

# Define ccfont_table separately, so it can be set from the command line
# to select an alternate compiled font table.
ccfont_table=icfontab

$(PSD)icfontab.dev : $(TOP_MAKEFILES) $(INT_MAK) $(ECHOGS_XE)\
 $(PSOBJ)icfontab.$(OBJ)\
 $(ccfonts1_) $(ccfonts2_) $(ccfonts3_) $(ccfonts4_) $(ccfonts5_)\
 $(ccfonts6_) $(ccfonts7_) $(ccfonts8_) $(ccfonts9_) $(ccfonts10_)\
 $(ccfonts11_) $(ccfonts12_) $(ccfonts13_) $(ccfonts14_) $(ccfonts15_)
	$(SETMOD) $(PSD)icfontab -obj $(PSOBJ)icfontab.$(OBJ)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts1_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts2_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts3_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts4_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts5_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts6_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts7_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts8_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts9_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts10_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts11_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts12_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts13_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts14_)
	$(ADDMOD) $(PSD)icfontab -obj $(ccfonts15_)

$(PSOBJ)icfontab.$(OBJ) : $(PSSRC)icfontab.c $(AK) $(ccfont_h) $(gconfigf_h)
	$(CP_) $(gconfigf_h) $(PSGEN)gconfigf.h
	$(PSCC) $(PSO_)icfontab.$(OBJ) $(C_) $(PSSRC)icfontab.c

# Strictly speaking, ccfonts shouldn't need to include type1,
# since one could choose to precompile only Type 0 fonts,
# but getting this exactly right would be too much work.
$(PSD)ccfonts.dev : $(TOP_MAKEFILES) $(INT_MAK)\
 $(PSD)type1.dev $(PSOBJ)iccfont.$(OBJ) $(PSD)$(ccfont_table).dev
	$(SETMOD) $(PSD)ccfonts -include $(PSD)type1
	$(ADDMOD) $(PSD)ccfonts -include $(PSD)$(ccfont_table)
	$(ADDMOD) $(PSD)ccfonts -obj $(PSOBJ)iccfont.$(OBJ)
	$(ADDMOD) $(PSD)ccfonts -oper ccfonts
	$(ADDMOD) $(PSD)ccfonts -ps $(ccfonts_ps)

$(PSOBJ)iccfont.$(OBJ) : $(PSSRC)iccfont.c $(GH) $(string__h)\
 $(gsstruct_h) $(ccfont_h) $(errors_h)\
 $(ialloc_h) $(idict_h) $(ifont_h) $(iname_h) $(isave_h) $(iutil_h)\
 $(oper_h) $(ostack_h) $(store_h) $(stream_h) $(strimpl_h) $(sfilter_h) $(iscan_h)
	$(PSCC) $(PSO_)iccfont.$(OBJ) $(C_) $(PSSRC)iccfont.c

# ---------------- Compiled initialization code ---------------- #

# We select either iccinit0 or iccinit1 depending on COMPILE_INITS.

$(PSOBJ)iccinit0.$(OBJ) : $(PSSRC)iccinit0.c $(stdpre_h)
	$(PSCC) $(PSO_)iccinit0.$(OBJ) $(C_) $(PSSRC)iccinit0.c

$(PSOBJ)iccinit1.$(OBJ) : $(PSOBJ)gs_init.$(OBJ)
	$(CP_) $(PSOBJ)gs_init.$(OBJ) $(PSOBJ)iccinit1.$(OBJ)

# All the gs_*.ps files should be prerequisites of gs_init.c,
# but we don't have any convenient list of them.
$(PSGEN)gs_init.c : $(PSLIB)$(GS_INIT) $(GENINIT_XE) $(gconfig_h)
	$(EXP)$(GENINIT_XE) -I $(PSLIB) $(GS_INIT) $(gconfig_h) -c $(PSGEN)gs_init.c

$(PSOBJ)gs_init.$(OBJ) : $(PSGEN)gs_init.c $(stdpre_h)
	$(PSCC) $(PSO_)gs_init.$(OBJ) $(C_) $(PSGEN)gs_init.c

# ---------------- Stochastic halftone ---------------- #

$(PSD)stocht.dev : $(INT_MAK) $(ECHOGS_XE) $(PSD)stocht$(COMPILE_INITS).dev
	$(SETMOD) $(PSD)stocht -include $(PSD)stocht$(COMPILE_INITS)

# If we aren't compiling, just include the PostScript code.
# Note that the resource machinery must be loaded first.
$(PSD)stocht0.dev : $(INT_MAK) $(ECHOGS_XE)
	$(SETMOD) $(PSD)stocht0 -ps gs_res ht_ccsto

# If we are compiling, a special compilation step is needed.
stocht1_=$(PSOBJ)ht_ccsto.$(OBJ)
$(PSD)stocht1.dev : $(INT_MAK) $(ECHOGS_XE) $(stocht1_) $(PSD)stocht0.dev
	$(SETMOD) $(PSD)stocht1 $(stocht1_)
	$(ADDMOD) $(PSD)stocht1 -halftone $(Q)StochasticDefault$(Q)
	$(ADDMOD) $(PSD)stocht1 -include $(PSD)stocht0

$(PSOBJ)ht_ccsto.$(OBJ) : $(PSGEN)ht_ccsto.c $(gxdhtres_h)
	$(PSCC) $(PSO_)ht_ccsto.$(OBJ) $(C_) $(PSGEN)ht_ccsto.c

$(PSGEN)ht_ccsto.c : $(PSLIB)ht_ccsto.ps $(GENHT_XE)
	$(EXP)$(GENHT_XE) $(PSLIB)ht_ccsto.ps $(PSGEN)ht_ccsto.c

# ================ PS LL3 features used internally in L2 ================ #

# ---------------- Functions ---------------- #

ifunc_h=$(PSSRC)ifunc.h $(gsfunc_h)

# Generic support, and FunctionType 0.
funcread_=$(PSOBJ)zfunc.$(OBJ) $(PSOBJ)zfunc0.$(OBJ)
$(PSD)func.dev : $(INT_MAK) $(ECHOGS_XE) $(funcread_) $(GLD)funclib.dev
	$(SETMOD) $(PSD)func $(funcread_)
	$(ADDMOD) $(PSD)func -oper zfunc
	$(ADDMOD) $(PSD)func -functiontype 0
	$(ADDMOD) $(PSD)func -include $(GLD)funclib

$(PSOBJ)zfunc.$(OBJ) : $(PSSRC)zfunc.c $(OP) $(memory__h)\
 $(gscdefs_h) $(gsfunc_h) $(gsstruct_h)\
 $(ialloc_h) $(idict_h) $(idparam_h) $(ifunc_h) $(store_h)
	$(PSCC) $(PSO_)zfunc.$(OBJ) $(C_) $(PSSRC)zfunc.c

$(PSOBJ)zfunc0.$(OBJ) : $(PSSRC)zfunc0.c $(OP) $(memory__h)\
 $(gsdsrc_h) $(gsfunc_h) $(gsfunc0_h)\
 $(stream_h)\
 $(files_h) $(ialloc_h) $(idict_h) $(idparam_h) $(ifunc_h)
	$(PSCC) $(PSO_)zfunc0.$(OBJ) $(C_) $(PSSRC)zfunc0.c

# ---------------- zlib/Flate filters ---------------- #

fzlib_=$(PSOBJ)zfzlib.$(OBJ)
$(PSD)fzlib.dev : $(INT_MAK) $(ECHOGS_XE) $(fzlib_)\
 $(GLD)szlibe.dev $(GLD)szlibd.dev
	$(SETMOD) $(PSD)fzlib -include $(GLD)szlibe $(GLD)szlibd
	$(ADDMOD) $(PSD)fzlib -obj $(fzlib_)
	$(ADDMOD) $(PSD)fzlib -oper zfzlib

$(PSOBJ)zfzlib.$(OBJ) : $(PSSRC)zfzlib.c $(OP)\
 $(idict_h) $(idparam_h) $(ifilter_h) $(ifrpred_h) $(ifwpred_h)\
 $(spdiffx_h) $(spngpx_h) $(strimpl_h) $(szlibx_h)
	$(PSCC) $(PSO_)zfzlib.$(OBJ) $(C_) $(PSSRC)zfzlib.c

# ---------------- ReusableStreamDecode filter ---------------- #
# This is also used by the implementation of CIDFontType 0 fonts.

$(PSD)frsd.dev : $(INT_MAK) $(ECHOGS_XE) $(PSD)zfrsd.dev
	$(SETMOD) $(PSD)frsd -include $(PSD)zfrsd
	$(ADDMOD) $(PSD)frsd -ps gs_lev2 gs_res gs_frsd

zfrsd_=$(PSOBJ)zfrsd.$(OBJ)
$(PSD)zfrsd.dev : $(INT_MAK) $(ECHOGS_XE) $(zfrsd_)
	$(SETMOD) $(PSD)zfrsd $(zfrsd_)
	$(ADDMOD) $(PSD)zfrsd -oper zfrsd

$(PSOBJ)zfrsd.$(OBJ) : $(PSSRC)zfrsd.c $(OP) $(memory__h)\
 $(gxiodev_h)\
 $(sfilter_h) $(stream_h) $(strimpl_h)\
 $(files_h) $(idict_h) $(idparam_h) $(iname_h) $(store_h)
	$(PSCC) $(PSO_)zfrsd.$(OBJ) $(C_) $(PSSRC)zfrsd.c

# ======================== PostScript Level 2 ======================== #

# We keep the old name for backward compatibility.
$(PSD)level2.dev : $(PSD)psl2.dev
	$(CP_) $(PSD)psl2.dev $(PSD)level2.dev

$(PSD)psl2.dev : $(INT_MAK) $(ECHOGS_XE)\
 $(PSD)cidfont.dev $(PSD)cie.dev $(PSD)cmapread.dev $(PSD)compfont.dev\
 $(PSD)dct.dev $(PSD)dpsand2.dev\
 $(PSD)filter.dev $(PSD)iodevice.dev $(PSD)pagedev.dev $(PSD)pattern.dev\
 $(PSD)psl1.dev $(GLD)psl2lib.dev $(PSD)psl2read.dev\
 $(PSD)sepr.dev $(PSD)type32.dev $(PSD)type42.dev
	$(SETMOD) $(PSD)psl2 -include $(PSD)cidfont $(PSD)cie $(PSD)cmapread $(PSD)compfont
	$(ADDMOD) $(PSD)psl2 -include $(PSD)dct $(PSD)dpsand2 $(PSD)filter $(PSD)iodevice
	$(ADDMOD) $(PSD)psl2 -include $(PSD)pagedev $(PSD)pattern $(PSD)psl1 $(GLD)psl2lib $(PSD)psl2read
	$(ADDMOD) $(PSD)psl2 -include $(PSD)sepr $(PSD)type32 $(PSD)type42
	$(ADDMOD) $(PSD)psl2 -emulator PostScript PostScriptLevel2

# Define basic Level 2 language support.
# This is the minimum required for CMap and CIDFont support.

psl2int_=$(PSOBJ)iutil2.$(OBJ) $(PSOBJ)zmisc2.$(OBJ)
$(PSD)psl2int.dev : $(INT_MAK) $(ECHOGS_XE) $(psl2int_)\
 $(PSD)dps2int.dev $(PSD)usparam.dev
	$(SETMOD) $(PSD)psl2int $(psl2int_)
	$(ADDMOD) $(PSD)psl2int -include $(PSD)dps2int $(PSD)usparam
	$(ADDMOD) $(PSD)psl2int -oper zmisc2
	$(ADDMOD) $(PSD)psl2int -ps gs_lev2 gs_res

ivmem2_h=$(PSSRC)ivmem2.h

$(PSOBJ)iutil2.$(OBJ) : $(PSSRC)iutil2.c $(GH) $(memory__h) $(string__h)\
 $(gsparam_h) $(gsutil_h)\
 $(errors_h) $(idict_h) $(imemory_h) $(iutil_h) $(iutil2_h) $(opcheck_h)
	$(PSCC) $(PSO_)iutil2.$(OBJ) $(C_) $(PSSRC)iutil2.c

$(PSOBJ)zmisc2.$(OBJ) : $(PSSRC)zmisc2.c $(OP) $(memory__h) $(string__h)\
 $(iddict_h) $(idparam_h) $(iparam_h) $(dstack_h) $(estack_h)\
 $(ilevel_h) $(iname_h) $(iutil2_h) $(ivmspace_h) $(store_h)
	$(PSCC) $(PSO_)zmisc2.$(OBJ) $(C_) $(PSSRC)zmisc2.c

# Define support for user and system parameters.
# We make this a separate module only because it must have a default.

nousparm_=$(PSOBJ)inouparm.$(OBJ)
$(PSD)nousparm.dev : $(INT_MAK) $(ECHOGS_XE) $(nousparm_)
	$(SETMOD) $(PSD)nousparm $(nousparm_)

$(PSOBJ)inouparm.$(OBJ) : $(PSSRC)inouparm.c\
 $(ghost_h) $(icontext_h)
	$(PSCC) $(PSO_)inouparm.$(OBJ) $(C_) $(PSSRC)inouparm.c

usparam_=$(PSOBJ)zusparam.$(OBJ)
$(PSD)usparam.dev : $(INT_MAK) $(ECHOGS_XE) $(usparam_)
	$(SETMOD) $(PSD)usparam $(usparam_)
	$(ADDMOD) $(PSD)usparam -oper zusparam -replace $(PSD)nousparm

# Note that zusparam includes both Level 1 and Level 2 operators.
$(PSOBJ)zusparam.$(OBJ) : $(PSSRC)zusparam.c $(OP) $(memory__h) $(string__h)\
 $(gscdefs_h) $(gsfont_h) $(gsstruct_h) $(gsutil_h) $(gxht_h)\
 $(ialloc_h) $(icontext_h) $(idict_h) $(idparam_h) $(iparam_h)\
 $(iname_h) $(itoken_h) $(iutil2_h) $(ivmem2_h)\
 $(dstack_h) $(estack_h) $(store_h)
	$(PSCC) $(PSO_)zusparam.$(OBJ) $(C_) $(PSSRC)zusparam.c

# Define full Level 2 support.

iimage2_h=$(PSSRC)iimage2.h

psl2read_=$(PSOBJ)zcolor2.$(OBJ) $(PSOBJ)zcsindex.$(OBJ) $(PSOBJ)zht2.$(OBJ) $(PSOBJ)zimage2.$(OBJ)
# Note that zmisc2 includes both Level 1 and Level 2 operators.
$(PSD)psl2read.dev : $(INT_MAK) $(ECHOGS_XE) $(psl2read_)\
 $(PSD)psl2int.dev $(PSD)dps2read.dev
	$(SETMOD) $(PSD)psl2read $(psl2read_)
	$(ADDMOD) $(PSD)psl2read -include $(PSD)psl2int $(PSD)dps2read
	$(ADDMOD) $(PSD)psl2read -oper zcolor2_l2 zcsindex_l2
	$(ADDMOD) $(PSD)psl2read -oper zht2_l2 zimage2_l2

$(PSOBJ)zcolor2.$(OBJ) : $(PSSRC)zcolor2.c $(OP)\
 $(gscolor_h) $(gscssub_h) $(gsmatrix_h) $(gsstruct_h)\
 $(gxcolor2_h) $(gxcspace_h) $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h) $(gxfixed_h) $(gxpcolor_h)\
 $(estack_h) $(ialloc_h) $(idict_h) $(iname_h) $(idparam_h) $(igstate_h) $(istruct_h)\
 $(store_h)
	$(PSCC) $(PSO_)zcolor2.$(OBJ) $(C_) $(PSSRC)zcolor2.c

$(PSOBJ)zcsindex.$(OBJ) : $(PSSRC)zcsindex.c $(OP) $(memory__h)\
 $(gscolor_h) $(gsstruct_h) $(gxfixed_h) $(gxcolor2_h) $(gxcspace_h) $(gsmatrix_h)\
 $(ialloc_h) $(icsmap_h) $(estack_h) $(igstate_h) $(ivmspace_h) $(store_h)
	$(PSCC) $(PSO_)zcsindex.$(OBJ) $(C_) $(PSSRC)zcsindex.c

$(PSOBJ)zht2.$(OBJ) : $(PSSRC)zht2.c $(OP)\
 $(gsstruct_h) $(gxdevice_h) $(gzht_h)\
 $(estack_h) $(ialloc_h) $(icolor_h) $(iddict_h) $(idparam_h) $(igstate_h)\
 $(iht_h) $(store_h)
	$(PSCC) $(PSO_)zht2.$(OBJ) $(C_) $(PSSRC)zht2.c

$(PSOBJ)zimage2.$(OBJ) : $(PSSRC)zimage2.c $(OP) $(math__h) $(memory__h)\
 $(gscolor_h) $(gscolor2_h) $(gscspace_h) $(gsimage_h) $(gsmatrix_h)\
 $(gxfixed_h)\
 $(idict_h) $(idparam_h) $(iimage_h) $(iimage2_h) $(ilevel_h) $(igstate_h)
	$(PSCC) $(PSO_)zimage2.$(OBJ) $(C_) $(PSSRC)zimage2.c

# ---------------- setpagedevice ---------------- #

pagedev_=$(PSOBJ)zdevice2.$(OBJ) $(PSOBJ)zmedia2.$(OBJ)
$(PSD)pagedev.dev : $(INT_MAK) $(ECHOGS_XE) $(pagedev_)
	$(SETMOD) $(PSD)pagedev $(pagedev_)
	$(ADDMOD) $(PSD)pagedev -oper zdevice2_l2 zmedia2_l2
	$(ADDMOD) $(PSD)pagedev -ps gs_setpd

$(PSOBJ)zdevice2.$(OBJ) : $(PSSRC)zdevice2.c $(OP) $(math__h) $(memory__h)\
 $(dstack_h) $(estack_h)\
 $(idict_h) $(idparam_h) $(igstate_h) $(iname_h) $(iutil_h) $(store_h)\
 $(gxdevice_h) $(gsstate_h)
	$(PSCC) $(PSO_)zdevice2.$(OBJ) $(C_) $(PSSRC)zdevice2.c

$(PSOBJ)zmedia2.$(OBJ) : $(PSSRC)zmedia2.c $(OP) $(math__h) $(memory__h)\
 $(gsmatrix_h) $(idict_h) $(idparam_h) $(iname_h) $(store_h)
	$(PSCC) $(PSO_)zmedia2.$(OBJ) $(C_) $(PSSRC)zmedia2.c

# ---------------- IODevices ---------------- #

iodevice_=$(PSOBJ)ziodev2.$(OBJ) $(PSOBJ)zdevcal.$(OBJ)
$(PSD)iodevice.dev : $(INT_MAK) $(ECHOGS_XE) $(iodevice_)
	$(SETMOD) $(PSD)iodevice $(iodevice_)
	$(ADDMOD) $(PSD)iodevice -oper ziodev2_l2
	$(ADDMOD) $(PSD)iodevice -iodev null calendar

$(PSOBJ)ziodev2.$(OBJ) : $(PSSRC)ziodev2.c $(OP) $(string__h) $(gp_h)\
 $(gxiodev_h) $(stream_h)\
 $(dstack_h) $(files_h) $(iparam_h) $(iutil2_h) $(store_h)
	$(PSCC) $(PSO_)ziodev2.$(OBJ) $(C_) $(PSSRC)ziodev2.c

$(PSOBJ)zdevcal.$(OBJ) : $(PSSRC)zdevcal.c $(GH) $(time__h)\
 $(gxiodev_h) $(iparam_h) $(istack_h)
	$(PSCC) $(PSO_)zdevcal.$(OBJ) $(C_) $(PSSRC)zdevcal.c

# ---------------- Filters other than the ones in sfilter.c ---------------- #

# Standard Level 2 decoding filters only.  The PDF configuration uses this.
fdecode_=$(GLOBJ)scantab.$(OBJ) $(GLOBJ)scfparam.$(OBJ) $(GLOBJ)sfilter2.$(OBJ) $(PSOBJ)zfdecode.$(OBJ)
$(PSD)fdecode.dev : $(INT_MAK) $(ECHOGS_XE) $(fdecode_)\
 $(GLD)cfd.dev $(GLD)lzwd.dev $(GLD)pdiff.dev $(GLD)pngp.dev $(GLD)rld.dev
	$(SETMOD) $(PSD)fdecode $(fdecode_)
	$(ADDMOD) $(PSD)fdecode -include $(GLD)cfd $(GLD)lzwd $(GLD)pdiff $(GLD)pngp $(GLD)rld
	$(ADDMOD) $(PSD)fdecode -oper zfdecode

$(PSOBJ)zfdecode.$(OBJ) : $(PSSRC)zfdecode.c $(OP) $(memory__h)\
 $(gsparam_h) $(gsstruct_h)\
 $(ialloc_h) $(idict_h) $(idparam_h) $(ifilter_h) $(ifilter2_h) $(ifrpred_h)\
 $(ilevel_h) $(iparam_h)\
 $(sa85x_h) $(scf_h) $(scfx_h) $(sfilter_h) $(slzwx_h) $(spdiffx_h) $(spngpx_h)\
 $(store_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)zfdecode.$(OBJ) $(C_) $(PSSRC)zfdecode.c

# Complete Level 2 filter capability.
filter_=$(PSOBJ)zfilter2.$(OBJ)
$(PSD)filter.dev : $(INT_MAK) $(ECHOGS_XE) $(PSD)fdecode.dev $(filter_)\
 $(GLD)cfe.dev $(GLD)lzwe.dev $(GLD)rle.dev
	$(SETMOD) $(PSD)filter -include $(PSD)fdecode
	$(ADDMOD) $(PSD)filter -obj $(filter_)
	$(ADDMOD) $(PSD)filter -include $(GLD)cfe $(GLD)lzwe $(GLD)rle
	$(ADDMOD) $(PSD)filter -oper zfilter2

$(PSOBJ)zfilter2.$(OBJ) : $(PSSRC)zfilter2.c $(OP) $(memory__h)\
 $(gsstruct_h)\
 $(ialloc_h) $(idict_h) $(idparam_h) $(ifilter_h) $(ifilter2_h) $(ifwpred_h)\
 $(store_h)\
 $(sfilter_h) $(scfx_h) $(slzwx_h) $(spdiffx_h) $(spngpx_h) $(strimpl_h)
	$(PSCC) $(PSO_)zfilter2.$(OBJ) $(C_) $(PSSRC)zfilter2.c

# Extensions beyond Level 2 standard.
xfilter_=$(PSOBJ)sbhc.$(OBJ) $(PSOBJ)sbwbs.$(OBJ) $(PSOBJ)shcgen.$(OBJ)\
 $(PSOBJ)smtf.$(OBJ) $(PSOBJ)zfilterx.$(OBJ)
$(PSD)xfilter.dev : $(INT_MAK) $(ECHOGS_XE) $(xfilter_) $(GLD)pngp.dev
	$(SETMOD) $(PSD)xfilter $(xfilter_)
	$(ADDMOD) $(PSD)xfilter -include $(GLD)pngp
	$(ADDMOD) $(PSD)xfilter -oper zfilterx

$(PSOBJ)sbhc.$(OBJ) : $(PSSRC)sbhc.c $(AK) $(memory__h) $(stdio__h)\
 $(gdebug_h) $(sbhc_h) $(shcgen_h) $(strimpl_h)
	$(PSCC) $(PSO_)sbhc.$(OBJ) $(C_) $(PSSRC)sbhc.c

$(PSOBJ)sbwbs.$(OBJ) : $(PSSRC)sbwbs.c $(AK) $(stdio__h) $(memory__h)\
 $(gdebug_h) $(sbwbs_h) $(sfilter_h) $(strimpl_h)
	$(PSCC) $(PSO_)sbwbs.$(OBJ) $(C_) $(PSSRC)sbwbs.c

$(PSOBJ)shcgen.$(OBJ) : $(PSSRC)shcgen.c $(AK) $(memory__h) $(stdio__h)\
 $(gdebug_h) $(gserror_h) $(gserrors_h) $(gsmemory_h)\
 $(scommon_h) $(shc_h) $(shcgen_h)
	$(PSCC) $(PSO_)shcgen.$(OBJ) $(C_) $(PSSRC)shcgen.c

$(PSOBJ)smtf.$(OBJ) : $(PSSRC)smtf.c $(AK) $(stdio__h)\
 $(smtf_h) $(strimpl_h)
	$(PSCC) $(PSO_)smtf.$(OBJ) $(C_) $(PSSRC)smtf.c

$(PSOBJ)zfilterx.$(OBJ) : $(PSSRC)zfilterx.c $(OP) $(memory__h)\
 $(gsstruct_h) $(ialloc_h) $(idict_h) $(idparam_h) $(ifilter_h)\
 $(store_h) $(sfilter_h) $(sbhc_h) $(sbtx_h) $(sbwbs_h) $(shcgen_h)\
 $(smtf_h) $(strimpl_h)
	$(PSCC) $(PSO_)zfilterx.$(OBJ) $(C_) $(PSSRC)zfilterx.c

# MD5 digest filter
fmd5_=$(GLOBJ)zfmd5.$(OBJ)
$(PSD)fmd5.dev : $(INT_MAK) $(ECHOGS_XE) $(fmd5_) $(GLD)smd5.dev
	$(SETMOD) $(PSD)fmd5 $(fmd5_)
	$(ADDMOD) $(PSD)fmd5 -include $(GLD)smd5
	$(ADDMOD) $(PSD)fmd5 -oper zfmd5

$(PSOBJ)zfmd5.$(OBJ) : $(PSSRC)zfmd5.c $(OP) $(memory__h)\
 $(gsstruct_h) $(ialloc_h) $(ifilter_h)\
 $(smd5_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)zfmd5.$(OBJ) $(C_) $(PSSRC)zfmd5.c

# Arcfour cipher filter
farc4_=$(GLOBJ)zfarc4.$(OBJ)
$(PSD)farc4.dev : $(INT_MAK) $(ECHOGS_XE) $(farc4_) $(GLD)sarc4.dev
	$(SETMOD) $(PSD)farc4 $(farc4_)
	$(ADDMOD) $(PSD)farc4 -include $(GLD)sarc4
	$(ADDMOD) $(PSD)farc4 -oper zfarc4

$(PSOBJ)zfarc4.$(OBJ) : $(PSSRC)zfarc4.c $(OP) $(memory__h)\
 $(gsstruct_h) $(ialloc_h) $(ifilter_h)\
 $(sarc4_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)zfarc4.$(OBJ) $(C_) $(PSSRC)zfarc4.c

# ---------------- Binary tokens ---------------- #

nobtoken_=$(PSOBJ)inobtokn.$(OBJ)
$(PSD)nobtoken.dev : $(INT_MAK) $(ECHOGS_XE) $(nobtoken_)
	$(SETMOD) $(PSD)nobtoken $(nobtoken_)

$(PSOBJ)inobtokn.$(OBJ) : $(PSSRC)inobtokn.c $(GH)\
 $(stream_h) $(errors_h) $(iscan_h) $(iscanbin_h)
	$(PSCC) $(PSO_)inobtokn.$(OBJ) $(C_) $(PSSRC)inobtokn.c

btoken_=$(PSOBJ)iscanbin.$(OBJ) $(PSOBJ)zbseq.$(OBJ)
$(PSD)btoken.dev : $(INT_MAK) $(ECHOGS_XE) $(btoken_)
	$(SETMOD) $(PSD)btoken $(btoken_)
	$(ADDMOD) $(PSD)btoken -oper zbseq_l2 -replace $(PSD)nobtoken
	$(ADDMOD) $(PSD)btoken -ps gs_btokn

$(PSOBJ)iscanbin.$(OBJ) : $(PSSRC)iscanbin.c $(GH)\
 $(math__h) $(memory__h) $(errors_h)\
 $(gsutil_h) $(gxalloc_h) $(ialloc_h) $(ibnum_h) $(iddict_h) $(iname_h)\
 $(iscan_h) $(iscanbin_h) $(iutil_h) $(ivmspace_h)\
 $(btoken_h) $(dstack_h) $(ostack_h)\
 $(sfilter_h) $(store_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)iscanbin.$(OBJ) $(C_) $(PSSRC)iscanbin.c

$(PSOBJ)zbseq.$(OBJ) : $(PSSRC)zbseq.c $(OP) $(memory__h)\
 $(gxalloc_h)\
 $(btoken_h) $(ialloc_h) $(istruct_h) $(store_h)
	$(PSCC) $(PSO_)zbseq.$(OBJ) $(C_) $(PSSRC)zbseq.c

# ---------------- User paths & insideness testing ---------------- #

upath_=$(PSOBJ)zupath.$(OBJ) $(PSOBJ)ibnum.$(OBJ) $(GLOBJ)gdevhit.$(OBJ)
$(PSD)upath.dev : $(INT_MAK) $(ECHOGS_XE) $(upath_)
	$(SETMOD) $(PSD)upath $(upath_)
	$(ADDMOD) $(PSD)upath -oper zupath_l2

$(PSOBJ)zupath.$(OBJ) : $(PSSRC)zupath.c $(OP)\
 $(dstack_h) $(oparc_h) $(store_h)\
 $(ibnum_h) $(idict_h) $(igstate_h) $(iname_h) $(iutil_h) $(stream_h)\
 $(gscoord_h) $(gsmatrix_h) $(gspaint_h) $(gspath_h) $(gsstate_h)\
 $(gxfixed_h) $(gxdevice_h) $(gzpath_h) $(gzstate_h)
	$(PSCC) $(PSO_)zupath.$(OBJ) $(C_) $(PSSRC)zupath.c

# -------- Additions common to Display PostScript and Level 2 -------- #

$(PSD)dpsand2.dev : $(INT_MAK) $(ECHOGS_XE)\
 $(PSD)btoken.dev $(PSD)color.dev $(PSD)upath.dev $(GLD)dps2lib.dev $(PSD)dps2read.dev
	$(SETMOD) $(PSD)dpsand2 -include $(PSD)btoken $(PSD)color $(PSD)upath $(GLD)dps2lib $(PSD)dps2read

dps2int_=$(PSOBJ)zvmem2.$(OBJ) $(PSOBJ)zdps1.$(OBJ)
# Note that zvmem2 includes both Level 1 and Level 2 operators.
$(PSD)dps2int.dev : $(INT_MAK) $(ECHOGS_XE) $(dps2int_)
	$(SETMOD) $(PSD)dps2int $(dps2int_)
	$(ADDMOD) $(PSD)dps2int -oper zvmem2 zdps1_l2
	$(ADDMOD) $(PSD)dps2int -ps gs_dps1

dps2read_=$(PSOBJ)ibnum.$(OBJ) $(PSOBJ)zcharx.$(OBJ)
$(PSD)dps2read.dev : $(INT_MAK) $(ECHOGS_XE) $(dps2read_) $(PSD)dps2int.dev
	$(SETMOD) $(PSD)dps2read $(dps2read_)
	$(ADDMOD) $(PSD)dps2read -include $(PSD)dps2int
	$(ADDMOD) $(PSD)dps2read -oper ireclaim_l2 zcharx
	$(ADDMOD) $(PSD)dps2read -ps gs_dps2

$(PSOBJ)ibnum.$(OBJ) : $(PSSRC)ibnum.c $(GH) $(math__h) $(memory__h)\
 $(errors_h) $(stream_h) $(ibnum_h) $(imemory_h) $(iutil_h)
	$(PSCC) $(PSO_)ibnum.$(OBJ) $(C_) $(PSSRC)ibnum.c

$(PSOBJ)zcharx.$(OBJ) : $(PSSRC)zcharx.c $(OP)\
 $(gsmatrix_h) $(gstext_h) $(gxfixed_h) $(gxfont_h)\
 $(ialloc_h) $(ibnum_h) $(ichar_h) $(iname_h) $(igstate_h)
	$(PSCC) $(PSO_)zcharx.$(OBJ) $(C_) $(PSSRC)zcharx.c

$(PSOBJ)zdps1.$(OBJ) : $(PSSRC)zdps1.c $(OP)\
 $(gsmatrix_h) $(gspath_h) $(gspath2_h) $(gsstate_h)\
 $(ialloc_h) $(ivmspace_h) $(igstate_h) $(store_h) $(stream_h) $(ibnum_h)
	$(PSCC) $(PSO_)zdps1.$(OBJ) $(C_) $(PSSRC)zdps1.c

$(PSOBJ)zvmem2.$(OBJ) : $(PSSRC)zvmem2.c $(OP)\
 $(estack_h) $(ialloc_h) $(ivmspace_h) $(store_h) $(ivmem2_h)
	$(PSCC) $(PSO_)zvmem2.$(OBJ) $(C_) $(PSSRC)zvmem2.c

# -------- Composite (PostScript Type 0) font support -------- #

$(PSD)compfont.dev : $(INT_MAK) $(ECHOGS_XE)\
 $(GLD)psf0lib.dev $(PSD)psf0read.dev
	$(SETMOD) $(PSD)compfont -include $(GLD)psf0lib $(PSD)psf0read

# We always include cmapread because zfont0.c refers to it,
# and it's not worth the trouble to exclude.
psf0read_=$(PSOBJ)zcfont.$(OBJ) $(PSOBJ)zfont0.$(OBJ)
$(PSD)psf0read.dev : $(INT_MAK) $(ECHOGS_XE) $(psf0read_)
	$(SETMOD) $(PSD)psf0read $(psf0read_)
	$(ADDMOD) $(PSD)psf0read -oper zcfont zfont0
	$(ADDMOD) $(PSD)psf0read -include $(PSD)cmapread

$(PSOBJ)zcfont.$(OBJ) : $(PSSRC)zcfont.c $(OP)\
 $(gsmatrix_h)\
 $(gxfixed_h) $(gxfont_h) $(gxtext_h)\
 $(ichar_h) $(estack_h) $(ifont_h) $(igstate_h) $(store_h)
	$(PSCC) $(PSO_)zcfont.$(OBJ) $(C_) $(PSSRC)zcfont.c

$(PSOBJ)zfont0.$(OBJ) : $(PSSRC)zfont0.c $(OP)\
 $(gsstruct_h)\
 $(gxdevice_h) $(gxfcmap_h) $(gxfixed_h) $(gxfont_h) $(gxfont0_h) $(gxmatrix_h)\
 $(gzstate_h)\
 $(bfont_h) $(ialloc_h) $(iddict_h) $(idparam_h) $(igstate_h) $(iname_h)\
 $(store_h)
	$(PSCC) $(PSO_)zfont0.$(OBJ) $(C_) $(PSSRC)zfont0.c

# ---------------- CMap and CIDFont support ---------------- #
# Note that this requires at least minimal Level 2 support,
# because it requires findresource.

icid_h=$(PSSRC)icid.h
ifcid_h=$(PSSRC)ifcid.h

cmapread_=$(PSOBJ)zcid.$(OBJ) $(PSOBJ)zfcmap.$(OBJ)
$(PSD)cmapread.dev : $(INT_MAK) $(ECHOGS_XE) $(cmapread_)\
 $(GLD)cmaplib.dev $(PSD)psl2int.dev
	$(SETMOD) $(PSD)cmapread $(cmapread_)
	$(ADDMOD) $(PSD)cmapread -include $(GLD)cmaplib $(PSD)psl2int
	$(ADDMOD) $(PSD)cmapread -oper zfcmap
	$(ADDMOD) $(PSD)cmapread -ps gs_cmap

$(PSOBJ)zfcmap.$(OBJ) : $(PSSRC)zfcmap.c $(OP) $(memory__h)\
 $(gsmatrix_h) $(gsstruct_h) $(gsutil_h)\
 $(gdevpsf_h)\
 $(gxfcmap_h) $(gxfont_h)\
 $(files_h) $(ialloc_h) $(icid_h) $(iddict_h) $(idparam_h) $(ifont_h) $(iname_h)\
 $(store_h)\
 $(spprint_h) $(stream_h)
	$(PSCC) $(PSO_)zfcmap.$(OBJ) $(C_) $(PSSRC)zfcmap.c

cidread_=$(PSOBJ)zcid.$(OBJ) $(PSOBJ)zfcid.$(OBJ) $(PSOBJ)zfcid0.$(OBJ) $(PSOBJ)zfcid1.$(OBJ)
$(PSD)cidfont.dev : $(INT_MAK) $(ECHOGS_XE) $(cidread_)\
 $(PSD)psf1read.dev $(PSD)psl2int.dev $(PSD)type2.dev $(PSD)type42.dev\
 $(PSD)zfrsd.dev
	$(SETMOD) $(PSD)cidfont $(cidread_)
	$(ADDMOD) $(PSD)cidfont -include $(PSD)psf1read $(PSD)psl2int
	$(ADDMOD) $(PSD)cidfont -include $(PSD)type2 $(PSD)type42 $(PSD)zfrsd
	$(ADDMOD) $(PSD)cidfont -oper zfcid0 zfcid1
	$(ADDMOD) $(PSD)cidfont -ps gs_cidfn gs_cidcm

$(PSOBJ)zcid.$(OBJ) : $(PSSRC)zcid.c $(OP)\
 $(gxcid_h) $(errors_h) $(icid_h) $(idict_h) $(idparam_h)
	$(PSCC) $(PSO_)zcid.$(OBJ) $(C_) $(PSSRC)zcid.c

$(PSOBJ)zfcid.$(OBJ) : $(PSSRC)zfcid.c $(OP)\
 $(gsmatrix_h) $(gxfcid_h)\
 $(bfont_h) $(icid_h) $(idict_h) $(idparam_h) $(ifcid_h) $(store_h)
	$(PSCC) $(PSO_)zfcid.$(OBJ) $(C_) $(PSSRC)zfcid.c

$(PSOBJ)zfcid0.$(OBJ) : $(PSSRC)zfcid0.c $(OP) $(memory__h)\
 $(gsccode_h) $(gsmatrix_h) $(gsstruct_h) $(gxfcid_h) $(gxfont1_h)\
 $(gdevpsf_h)\
 $(stream_h)\
 $(bfont_h) $(files_h) $(ichar_h) $(ichar1_h) $(icid_h) $(idict_h) $(idparam_h)\
 $(ifcid_h) $(ifont1_h) $(ifont2_h) $(ifont42_h) $(store_h)
	$(PSCC) $(PSO_)zfcid0.$(OBJ) $(C_) $(PSSRC)zfcid0.c

$(PSOBJ)zfcid1.$(OBJ) : $(PSSRC)zfcid1.c $(OP) $(memory__h)\
 $(gsccode_h) $(gsmatrix_h) $(gsstruct_h) $(gxfcid_h)\
 $(bfont_h) $(icid_h) $(idict_h) $(idparam_h)\
 $(ifcid_h) $(ifont42_h) $(store_h)
	$(PSCC) $(PSO_)zfcid1.$(OBJ) $(C_) $(PSSRC)zfcid1.c

# ---------------- CIE color ---------------- #

cieread_=$(PSOBJ)zcie.$(OBJ) $(PSOBJ)zcrd.$(OBJ)
$(PSD)cie.dev : $(INT_MAK) $(ECHOGS_XE) $(cieread_) $(GLD)cielib.dev
	$(SETMOD) $(PSD)cie $(cieread_)
	$(ADDMOD) $(PSD)cie -oper zcie_l2 zcrd_l2
	$(ADDMOD) $(PSD)cie -include $(GLD)cielib

icie_h=$(PSSRC)icie.h

$(PSOBJ)zcie.$(OBJ) : $(PSSRC)zcie.c $(OP) $(math__h) $(memory__h)\
 $(gscolor2_h) $(gscie_h) $(gsstruct_h) $(gxcspace_h)\
 $(ialloc_h) $(icie_h) $(idict_h) $(idparam_h) $(estack_h)\
 $(isave_h) $(igstate_h) $(ivmspace_h) $(store_h)
	$(PSCC) $(PSO_)zcie.$(OBJ) $(C_) $(PSSRC)zcie.c

$(PSOBJ)zcrd.$(OBJ) : $(PSSRC)zcrd.c $(OP) $(math__h)\
 $(gscrd_h) $(gscrdp_h) $(gscspace_h) $(gscolor2_h) $(gsstruct_h)\
 $(estack_h) $(ialloc_h) $(icie_h) $(idict_h) $(idparam_h) $(igstate_h)\
 $(iparam_h) $(ivmspace_h) $(store_h)
	$(PSCC) $(PSO_)zcrd.$(OBJ) $(C_) $(PSSRC)zcrd.c

# ---------------- Pattern color ---------------- #

ipcolor_h=$(PSSRC)ipcolor.h

$(PSD)pattern.dev : $(INT_MAK) $(ECHOGS_XE) $(GLD)patlib.dev $(PSD)patread.dev
	$(SETMOD) $(PSD)pattern -include $(GLD)patlib $(PSD)patread

patread_=$(PSOBJ)zpcolor.$(OBJ)
$(PSD)patread.dev : $(INT_MAK) $(ECHOGS_XE) $(patread_)
	$(SETMOD) $(PSD)patread $(patread_)
	$(ADDMOD) $(PSD)patread -oper zpcolor_l2

$(PSOBJ)zpcolor.$(OBJ) : $(PSSRC)zpcolor.c $(OP)\
 $(gscolor_h) $(gsmatrix_h) $(gsstruct_h)\
 $(gxcolor2_h) $(gxcspace_h) $(gxdcolor_h) $(gxdevice_h) $(gxdevmem_h)\
 $(gxfixed_h) $(gxpcolor_h)\
 $(estack_h)\
 $(ialloc_h) $(icremap_h) $(idict_h) $(idparam_h) $(igstate_h)\
 $(ipcolor_h) $(istruct_h)\
 $(store_h)
	$(PSCC) $(PSO_)zpcolor.$(OBJ) $(C_) $(PSSRC)zpcolor.c

# ---------------- Separation color ---------------- #

seprread_=$(PSOBJ)zcssepr.$(OBJ) $(PSOBJ)zccube.$(OBJ)
$(PSD)sepr.dev : $(INT_MAK) $(ECHOGS_XE) $(seprread_)\
 $(PSD)func4.dev $(GLD)seprlib.dev
	$(SETMOD) $(PSD)sepr $(seprread_)
	$(ADDMOD) $(PSD)sepr -oper zcssepr_l2
	$(ADDMOD) $(PSD)sepr -oper zccube
	$(ADDMOD) $(PSD)sepr -include $(PSD)func4 $(GLD)seprlib

$(PSOBJ)zcssepr.$(OBJ) : $(PSSRC)zcssepr.c $(OP) $(memory__h)\
 $(gscolor_h) $(gscsepr_h) $(gsmatrix_h) $(gsstruct_h)\
 $(gxcolor2_h) $(gxcspace_h) $(gxfixed_h)\
 $(estack_h) $(ialloc_h) $(icsmap_h) $(ifunc_h) $(igstate_h) $(iname_h) $(ivmspace_h) $(store_h)
	$(PSCC) $(PSO_)zcssepr.$(OBJ) $(C_) $(PSSRC)zcssepr.c

$(PSOBJ)zccube.$(OBJ) : $(PSSRC)zccube.c $(OP) $(memory__h)\
 $(gxcspace_h) $(estack_h) $(ialloc_h) $(ifunc_h) $(ostack_h) $(gsfunc0_h)
	$(PSCC) $(PSO_)zccube.$(OBJ) $(C_) $(PSSRC)zccube.c

# ---------------- DCT filters ---------------- #
# The definitions for jpeg*.dev are in jpeg.mak.

$(PSD)dct.dev : $(INT_MAK) $(ECHOGS_XE) $(PSD)dcte.dev $(PSD)dctd.dev
	$(SETMOD) $(PSD)dct -include $(PSD)dcte $(PSD)dctd

# Encoding (compression)

dcte_=$(PSOBJ)zfdcte.$(OBJ)
$(PSD)dcte.dev : $(INT_MAK) $(ECHOGS_XE) $(GLD)sdcte.dev $(GLD)sdeparam.dev $(dcte_)
	$(SETMOD) $(PSD)dcte -include $(GLD)sdcte $(GLD)sdeparam
	$(ADDMOD) $(PSD)dcte -obj $(dcte_)
	$(ADDMOD) $(PSD)dcte -oper zfdcte

$(PSOBJ)zfdcte.$(OBJ) : $(PSSRC)zfdcte.c $(OP)\
 $(memory__h) $(stdio__h) $(jpeglib__h)\
 $(gsmalloc_h)\
 $(sdct_h) $(sjpeg_h) $(stream_h) $(strimpl_h)\
 $(files_h) $(ialloc_h) $(idict_h) $(idparam_h) $(ifilter_h) $(iparam_h)
	$(PSCC) $(PSO_)zfdcte.$(OBJ) $(C_) $(PSSRC)zfdcte.c

# Decoding (decompression)

dctd_=$(PSOBJ)zfdctd.$(OBJ)
$(PSD)dctd.dev : $(INT_MAK) $(ECHOGS_XE) $(GLD)sdctd.dev $(GLD)sddparam.dev $(dctd_)
	$(SETMOD) $(PSD)dctd -include $(GLD)sdctd $(GLD)sddparam
	$(ADDMOD) $(PSD)dctd -obj $(dctd_)
	$(ADDMOD) $(PSD)dctd -oper zfdctd

$(PSOBJ)zfdctd.$(OBJ) : $(PSSRC)zfdctd.c $(OP)\
 $(memory__h) $(stdio__h) $(jpeglib__h)\
 $(gsmalloc_h)\
 $(ialloc_h) $(ifilter_h) $(iparam_h) $(sdct_h) $(sjpeg_h) $(strimpl_h)
	$(PSCC) $(PSO_)zfdctd.$(OBJ) $(C_) $(PSSRC)zfdctd.c

# ================ Display PostScript ================ #

dps_=$(PSOBJ)zdps.$(OBJ) $(PSOBJ)zcontext.$(OBJ)
$(PSD)dps.dev : $(INT_MAK) $(ECHOGS_XE) $(GLD)dpslib.dev $(PSD)psl2.dev $(dps_)
	$(SETMOD) $(PSD)dps -include $(GLD)dpslib $(PSD)psl2
	$(ADDMOD) $(PSD)dps -obj $(dps_)
	$(ADDMOD) $(PSD)dps -oper zcontext1 zcontext2 zdps
	$(ADDMOD) $(PSD)dps -ps gs_dps

$(PSOBJ)zdps.$(OBJ) : $(PSSRC)zdps.c $(OP)\
 $(gsdps_h) $(gsimage_h) $(gsiparm2_h) $(gsstate_h)\
 $(gxalloc_h) $(gxfixed_h) $(gxpath_h)\
 $(btoken_h)\
 $(idparam_h) $(iddict_h) $(igstate_h) $(iimage2_h) $(iname_h) $(store_h)
	$(PSCC) $(PSO_)zdps.$(OBJ) $(C_) $(PSSRC)zdps.c

$(PSOBJ)zcontext.$(OBJ) : $(PSSRC)zcontext.c $(OP) $(gp_h) $(memory__h)\
 $(gsexit_h) $(gsgc_h) $(gsstruct_h) $(gsutil_h) $(gxalloc_h) $(gxstate_h)\
 $(icontext_h) $(idict_h) $(igstate_h) $(interp_h) $(isave_h) $(istruct_h)\
 $(dstack_h) $(estack_h) $(files_h) $(ostack_h) $(store_h) $(stream_h)
	$(PSCC) $(PSO_)zcontext.$(OBJ) $(C_) $(PSSRC)zcontext.c

# ---------------- NeXT Display PostScript ---------------- #

dpsnext_=$(PSOBJ)zdpnext.$(OBJ)
$(PSD)dpsnext.dev : $(INT_MAK) $(ECHOGS_XE) $(dpsnext_)\
 $(PSD)dps.dev $(GLD)dpnxtlib.dev
	$(SETMOD) $(PSD)dpsnext -include $(PSD)dps $(GLD)dpnxtlib
	$(ADDMOD) $(PSD)dpsnext -obj $(dpsnext_)
	$(ADDMOD) $(PSD)dpsnext -oper zdpnext
	$(ADDMOD) $(PSD)dpsnext -ps gs_dpnxt

$(PSOBJ)zdpnext.$(OBJ) : $(PSSRC)zdpnext.c $(math__h) $(OP)\
 $(gscoord_h) $(gscspace_h) $(gsdpnext_h)\
 $(gsiparam_h) $(gsiparm2_h) $(gsmatrix_h) $(gspath2_h)\
 $(gxcvalue_h) $(gxdevice_h) $(gxsample_h)\
 $(ialloc_h) $(igstate_h) $(iimage_h) $(iimage2_h) $(store_h)
	$(PSCC) $(PSO_)zdpnext.$(OBJ) $(C_) $(PSSRC)zdpnext.c

# ==================== PostScript LanguageLevel 3 ===================== #

# ---------------- DevicePixel color space ---------------- #

cspixint_=$(PSOBJ)zcspixel.$(OBJ)
$(PSD)cspixel.dev : $(INT_MAK) $(ECHOGS_XE) $(cspixint_) $(GLD)cspixlib.dev
	$(SETMOD) $(PSD)cspixel $(cspixint_)
	$(ADDMOD) $(PSD)cspixel -oper zcspixel
	$(ADDMOD) $(PSD)cspixel -include $(GLD)cspixlib

$(PSOBJ)zcspixel.$(OBJ) : $(PSSRC)zcspixel.c $(OP)\
 $(gscolor2_h) $(gscpixel_h) $(gscspace_h) $(gsmatrix_h)\
 $(igstate_h)
	$(PSCC) $(PSO_)zcspixel.$(OBJ) $(C_) $(PSSRC)zcspixel.c

# ---------------- Rest of LanguageLevel 3 ---------------- #

$(PSD)psl3.dev : $(INT_MAK) $(ECHOGS_XE)\
 $(PSD)psl2.dev $(PSD)cspixel.dev $(PSD)frsd.dev $(PSD)func.dev\
 $(GLD)psl3lib.dev $(PSD)psl3read.dev
	$(SETMOD) $(PSD)psl3 -include $(PSD)psl2 $(PSD)cspixel $(PSD)frsd $(PSD)func
	$(ADDMOD) $(PSD)psl3 -include $(GLD)psl3lib $(PSD)psl3read

$(PSOBJ)zcsdevn.$(OBJ) : $(PSSRC)zcsdevn.c $(OP) $(memory__h)\
 $(gscolor2_h) $(gscdevn_h) $(gxcdevn_h) $(gxcspace_h)\
 $(estack_h) $(ialloc_h) $(icremap_h) $(ifunc_h) $(igstate_h) $(iname_h)
	$(PSCC) $(PSO_)zcsdevn.$(OBJ) $(C_) $(PSSRC)zcsdevn.c

$(PSOBJ)zfunc3.$(OBJ) : $(PSSRC)zfunc3.c $(memory__h) $(OP)\
 $(gsfunc3_h) $(gsstruct_h)\
 $(files_h) $(ialloc_h) $(idict_h) $(idparam_h) $(ifunc_h)\
 $(store_h) $(stream_h)
	$(PSCC) $(PSO_)zfunc3.$(OBJ) $(C_) $(PSSRC)zfunc3.c

# FunctionType 4 functions are not a PostScript feature, but they
# are used in the implementation of Separation and DeviceN color spaces.

func4read_=$(PSOBJ)zfunc4.$(OBJ)
$(PSD)func4.dev : $(INT_MAK) $(ECHOGS_XE) $(func4read_)\
 $(PSD)func.dev $(GLD)func4lib.dev
	$(SETMOD) $(PSD)func4 $(func4read_)
	$(ADDMOD) $(PSD)func4 -functiontype 4
	$(ADDMOD) $(PSD)func4 -include $(PSD)func $(GLD)func4lib

$(PSOBJ)zfunc4.$(OBJ) : $(PSSRC)zfunc4.c $(memory__h) $(OP)\
 $(gsfunc_h) $(gsfunc4_h) $(gsutil_h)\
 $(idict_h) $(ifunc_h) $(iname_h)\
 $(opextern_h) $(dstack_h)
	$(PSCC) $(PSO_)zfunc4.$(OBJ) $(C_) $(PSSRC)zfunc4.c

$(PSOBJ)zimage3.$(OBJ) : $(PSSRC)zimage3.c $(OP) $(memory__h)\
 $(gscolor2_h) $(gsiparm3_h) $(gsiparm4_h) $(gscspace_h) $(gxiparam_h)\
 $(idparam_h) $(idict_h) $(igstate_h) $(iimage_h) $(iimage2_h)
	$(PSCC) $(PSO_)zimage3.$(OBJ) $(C_) $(PSSRC)zimage3.c

$(PSOBJ)zmisc3.$(OBJ) : $(PSSRC)zmisc3.c $(GH)\
 $(gsclipsr_h) $(gscolor2_h) $(gscspace_h) $(gscssub_h) $(gsmatrix_h)\
 $(igstate_h) $(oper_h) $(store_h)
	$(PSCC) $(PSO_)zmisc3.$(OBJ) $(C_) $(PSSRC)zmisc3.c

$(PSOBJ)zshade.$(OBJ) : $(PSSRC)zshade.c $(memory__h) $(OP)\
 $(gscolor2_h) $(gscolor3_h) $(gscspace_h) $(gsfunc3_h)\
 $(gsptype2_h) $(gsshade_h) $(gsstruct_h) $(gsuid_h)\
 $(stream_h)\
 $(files_h)\
 $(ialloc_h) $(idict_h) $(idparam_h) $(ifunc_h) $(igstate_h) $(ipcolor_h)\
 $(store_h)
	$(PSCC) $(PSO_)zshade.$(OBJ) $(C_) $(PSSRC)zshade.c

psl3read_1=$(PSOBJ)zcsdevn.$(OBJ) $(PSOBJ)zfunc3.$(OBJ) $(PSOBJ)zccube.$(OBJ)
psl3read_2=$(PSOBJ)zimage3.$(OBJ) $(PSOBJ)zmisc3.$(OBJ) $(PSOBJ)zshade.$(OBJ)
psl3read_=$(psl3read_1) $(psl3read_2)

# Note: we need the ReusableStreamDecode filter for shadings.
$(PSD)psl3read.dev : $(INT_MAK) $(ECHOGS_XE) $(psl3read_)\
 $(PSD)frsd.dev $(PSD)fzlib.dev
	$(SETMOD) $(PSD)psl3read $(psl3read_1)
	$(ADDMOD) $(PSD)psl3read $(psl3read_2)
	$(ADDMOD) $(PSD)psl3read -oper zcsdevn
	$(ADDMOD) $(PSD)psl3read -oper zccube
	$(ADDMOD) $(PSD)psl3read -oper zimage3 zmisc3 zshade
	$(ADDMOD) $(PSD)psl3read -functiontype 2 3
	$(ADDMOD) $(PSD)psl3read -ps gs_ll3
	$(ADDMOD) $(PSD)psl3read -include $(PSD)frsd $(PSD)fzlib

# ---------------- Trapping ---------------- #

trapread_=$(PSOBJ)ztrap.$(OBJ)
$(PSD)trapread.dev : $(INT_MAK) $(ECHOGS_XE) $(trapread_)
	$(SETMOD) $(PSD)trapread $(trapread_)
	$(ADDMOD) $(PSD)trapread -oper ztrap
	$(ADDMOD) $(PSD)trapread -ps gs_trap

$(PSOBJ)ztrap.$(OBJ) : $(PSSRC)ztrap.c $(OP)\
 $(gstrap_h)\
 $(ialloc_h) $(iparam_h)
	$(PSCC) $(PSO_)ztrap.$(OBJ) $(C_) $(PSSRC)ztrap.c

$(PSD)trapping.dev : $(INT_MAK) $(ECHOGS_XE) $(GLD)traplib.dev $(PSD)trapread.dev
	$(SETMOD) $(PSD)trapping -include $(GLD)traplib $(PSD)trapread

# ---------------- Transparency ---------------- #

transread_=$(PSOBJ)ztrans.$(OBJ)
$(PSD)transpar.dev : $(INT_MAK) $(ECHOGS_XE)\
 $(PSD)psl2read.dev $(GLD)translib.dev $(transread_)
	$(SETMOD) $(PSD)transpar $(transread_)
	$(ADDMOD) $(PSD)transpar -oper ztrans
	$(ADDMOD) $(PSD)transpar -include $(PSD)psl2read $(GLD)translib

$(PSOBJ)ztrans.$(OBJ) : $(PSSRC)ztrans.c $(OP) $(string__h)\
 $(gscolor2_h) $(gscspace_h) $(gsipar3x_h) $(gstrans_h) $(gxiparam_h)\
 $(idict_h) $(idparam_h) $(ifunc_h) $(igstate_h) $(iimage_h) $(iimage2_h) $(iname_h)\
 $(store_h)
	$(PSCC) $(PSO_)ztrans.$(OBJ) $(C_) $(PSSRC)ztrans.c

# ---------------- ICCBased color spaces ---------------- #

iccread_=$(PSOBJ)zicc.$(OBJ)
$(PSD)icc.dev : $(INT_MAK) $(ECHOGS_XE) $(PSD)cie.dev $(iccread_) \
                $(GLD)sicclib.dev
	$(SETMOD) $(PSD)icc $(iccread_)
	$(ADDMOD) $(PSD)icc -oper zicc_ll3
	$(ADDMOD) $(PSD)icc -ps gs_icc
	$(ADDMOD) $(PSD)icc -include $(GLD)sicclib $(PSD)cie

$(PSOBJ)zicc.$(OBJ) : $(PSSRC)zicc.c  $(OP) $(math__h) $(memory__h)\
 $(gsstruct_h) $(gxcspace_h) $(stream_h) $(files_h) $(gscolor2_h)\
 $(gsicc_h) $(estack_h) $(idict_h) $(idparam_h) $(igstate_h) $(icie_h)
	$(PSCC) $(PSO_)zicc.$(OBJ) $(C_) $(PSSRC)zicc.c



# ================================ PDF ================================ #

# We need nearly all of the PostScript LanguageLevel 3 interpreter for PDF,
# but not all of it: we could do without the arc operators (Level 1),
# the Encode filters (Level 2), and some LL3 features (clipsave/cliprestore,
# UseCIEColor, IdiomSets).  However, we've decided it isn't worth the
# trouble to do the fine-grain factoring to enable this, since code size
# is not at a premium for PDF interpreters.

# Because of the way the PDF encodings are defined, they must get loaded
# before we install the Level 2 resource machinery.
# On the other hand, the PDF .ps files must get loaded after
# level2dict is defined.
$(PSD)pdf.dev : $(INT_MAK) $(ECHOGS_XE)\
 $(PSD)psbase.dev $(GLD)dps2lib.dev $(PSD)dps2read.dev\
 $(PSD)pdffonts.dev $(PSD)psl3.dev $(PSD)pdfread.dev $(PSD)cff.dev\
 $(PSD)fmd5.dev $(PSD)farc4.dev $(PSD)ttfont.dev $(PSD)type2.dev $(PSD)icc.dev
	$(SETMOD) $(PSD)pdf -include $(PSD)psbase $(GLD)dps2lib
	$(ADDMOD) $(PSD)pdf -include $(PSD)dps2read $(PSD)pdffonts $(PSD)psl3
	$(ADDMOD) $(PSD)pdf -include $(GLD)psl2lib $(PSD)pdfread $(PSD)cff
	$(ADDMOD) $(PSD)pdf -include $(PSD)fmd5 $(PSD)farc4 $(PSD)ttfont $(PSD)type2
	$(ADDMOD) $(PSD)pdf -include $(PSD)icc
	$(ADDMOD) $(PSD)pdf -functiontype 4
	$(ADDMOD) $(PSD)pdf -emulator PDF

# Reader only

$(PSD)pdffonts.dev : $(INT_MAK) $(ECHOGS_XE)
	$(SETMOD) $(PSD)pdffonts -ps gs_mex_e gs_mro_e gs_pdf_e gs_wan_e

$(PSD)pdfread.dev : $(INT_MAK) $(ECHOGS_XE)\
 $(PSD)frsd.dev $(PSD)func4.dev $(PSD)fzlib.dev $(PSD)transpar.dev
	$(SETMOD) $(PSD)pdfread -include $(PSD)frsd $(PSD)func4 $(PSD)fzlib
	$(SETMOD) $(PSD)pdfread -include $(PSD)transpar
	$(ADDMOD) $(PSD)pdfread -ps pdf_ops gs_l2img
	$(ADDMOD) $(PSD)pdfread -ps pdf_base pdf_draw pdf_font pdf_main pdf_sec

# ================ Dependencies for auxiliary programs ================ #

GENINIT_DEPS=$(stdpre_h)

# ============================= Main program ============================== #

$(PSOBJ)gs.$(OBJ) : $(PSSRC)gs.c $(GH)\
 $(imain_h) $(imainarg_h) $(iminst_h)
	$(PSCC) $(PSO_)gs.$(OBJ) $(C_) $(PSSRC)gs.c

$(PSOBJ)iapi.$(OBJ) : $(PSSRC)iapi.c $(GH)\
 $(string__h) $(errors_h) $(gsargs_h) $(gscdefs_h) $(gstypes_h) $(iapi_h)\
 $(iref_h) $(imain_h) $(imainarg_h) $(iminst_h)
	$(PSCC) $(PSO_)iapi.$(OBJ) $(C_) $(PSSRC)iapi.c

$(PSOBJ)icontext.$(OBJ) : $(PSSRC)icontext.c $(GH)\
 $(gsstruct_h) $(gxalloc_h)\
 $(dstack_h) $(errors_h) $(estack_h) $(files_h)\
 $(icontext_h) $(idict_h) $(igstate_h) $(interp_h) $(isave_h) $(store_h)\
 $(stream_h)
	$(PSCC) $(PSO_)icontext.$(OBJ) $(C_) $(PSSRC)icontext.c

gdevdsp_h=$(GLSRC)gdevdsp.h
gdevdsp2_h=$(GLSRC)gdevdsp2.h

$(PSOBJ)idisp.$(OBJ) : $(PSSRC)idisp.c\
 $(iapi_h) $(ghost_h) $(gp_h)\
 $(imain_h) $(iminst_h) $(idisp_h)\
 $(gx_h) $(gxdevice_h) $(gxdevmem_h) $(gdevdsp_h) $(gdevdsp2_h)
	$(PSCC) $(PSO_)idisp.$(OBJ) $(C_) $(PSSRC)idisp.c

$(PSOBJ)imainarg.$(OBJ) : $(PSSRC)imainarg.c $(GH)\
 $(ctype__h) $(memory__h) $(string__h)\
 $(gp_h)\
 $(gsargs_h) $(gscdefs_h) $(gsdevice_h) $(gsmalloc_h) $(gsmdebug_h)\
 $(gxdevice_h) $(gxdevmem_h)\
 $(errors_h) $(estack_h) $(files_h)\
 $(ialloc_h) $(iconf_h) $(imain_h) $(imainarg_h) $(iminst_h)\
 $(iname_h) $(interp_h) $(iscan_h) $(iutil_h) $(ivmspace_h)\
 $(ostack_h) $(sfilter_h) $(store_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)imainarg.$(OBJ) $(C_) $(PSSRC)imainarg.c

$(PSOBJ)imain.$(OBJ) : $(PSSRC)imain.c $(GH) $(memory__h) $(string__h)\
 $(gp_h) $(gscdefs_h) $(gslib_h) $(gsmatrix_h) $(gsutil_h) $(gxdevice_h)\
 $(dstack_h) $(errors_h) $(estack_h) $(files_h)\
 $(ialloc_h) $(iconf_h) $(idebug_h) $(idict_h) $(idisp_h) $(iinit_h)\
 $(iname_h) $(interp_h) $(isave_h) $(iscan_h) $(ivmspace_h)\
 $(main_h) $(oper_h) $(ostack_h)\
 $(sfilter_h) $(store_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)imain.$(OBJ) $(C_) $(PSSRC)imain.c

#****** $(CCINT) interp.c
$(PSOBJ)interp.$(OBJ) : $(PSSRC)interp.c $(GH) $(memory__h) $(string__h)\
 $(gsstruct_h)\
 $(dstack_h) $(errors_h) $(estack_h) $(files_h)\
 $(ialloc_h) $(iastruct_h) $(icontext_h) $(icremap_h) $(iddict_h) $(igstate_h)\
 $(iname_h) $(inamedef_h) $(interp_h) $(ipacked_h)\
 $(isave_h) $(iscan_h) $(istack_h) $(itoken_h) $(iutil_h) $(ivmspace_h)\
 $(oper_h) $(ostack_h) $(sfilter_h) $(store_h) $(stream_h) $(strimpl_h)
	$(PSCC) $(PSO_)interp.$(OBJ) $(C_) $(PSSRC)interp.c

$(PSOBJ)ireclaim.$(OBJ) : $(PSSRC)ireclaim.c $(GH)\
 $(gsstruct_h)\
 $(iastate_h) $(icontext_h) $(interp_h) $(isave_h) $(isstate_h)\
 $(dstack_h) $(errors_h) $(estack_h) $(opdef_h) $(ostack_h) $(store_h)
	$(PSCC) $(PSO_)ireclaim.$(OBJ) $(C_) $(PSSRC)ireclaim.c
