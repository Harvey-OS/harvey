#    Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
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

# Generic makefile, common to all platforms.
# The platform-specific makefiles `include' this file.
# They define the following symbols:
#	GS - the name of the executable (without the extension, if any).
#	GS_LIB_DEFAULT - the default directory/ies for searching for the
#	    initialization and font files at run time.
#	GS_DOCDIR - the directory where documentation will be available
#	    at run time.
#	JSRCDIR - the directory where the IJG JPEG library source code
#	    is stored (at compilation time).
#	DEVICE_DEVS - the devices to include in the executable.
#	    See devs.mak for details.
#	DEVICE_DEVS1...DEVICE_DEVS9 - additional devices, if the definition of
#	    DEVICE_DEVS doesn't fit on one line.  See devs.mak for details.
#	FEATURE_DEVS - what features to include in the executable.
#	    Normally this is one of:
#		    level1 - a standard PostScript Level 1 language
#			interpreter.
#		    level2 - a standard PostScript Level 2 language
#			interpreter.
#		    pdf - a PDF-capable interpreter.
#	    The following features may be added to either of the standard
#		configurations:
#		    ccfonts - precompile fonts into C, and link them
#			with the executable.  See fonts.doc for details.
#		    ccinit - precompile the initialization files into
#			C data, and link them with the executable.
#	    The remaining features are of interest primarily to developers
#		who want to "mix and match" features to create custom
#		configurations:
#		    dps - (partial) support for Display PostScript extensions:
#			see language.doc for details.
#		    btoken - support for binary token encodings.
#			Included automatically in the dps and level2 features.
#		    color - support for the Level 1 CMYK color extensions.
#			Included automatically in the dps and level2 features.
#		    compfont - support for composite (type 0) fonts.
#			Included automatically in the level2 feature.
#		    dct - support for DCTEncode/Decode filters.
#			Included automatically in the level2 feature.
#		    filter - support for Level 2 filters (other than eexec,
#			ASCIIHexEncode/Decode, NullEncode, PFBDecode,
#			RLEncode/Decode, and SubFileDecode, which are
#			always included, and DCTEncode/Decode,
#			which are separate).
#			Included automatically in the level2 feature.
#		    type1 - support for Type 1 fonts and eexec;
#			normally included automatically in all configurations.
#		There are quite a number of other sub-features that can be
#		selectively included in or excluded from a configuration,
#		but the above are the ones that are most likely to be of
#		interest.
#
# It is very unlikely that anyone would want to edit the remaining
#   symbols, but we describe them here for completeness:
#	GS_INIT - the name of the initialization file for the interpreter,
#		normally gs_init.ps.
#	PLATFORM - a "device" name for the platform, so that platforms can
#		add various kinds of resources like devices and features.
#	CMD - the suffix for shell command files (e.g., null or .bat).
#		(This is only needed in a few places.)
#	D - the directory separator character (\ for MS-DOS, / for Unix).
#	O - the string for specifying the output file from the C compiler
#		(-o for MS-DOS, -o ./ for Unix).
#	OBJ - the extension for relocatable object files (e.g., o or obj).
#	XE - the extension for executable files (e.g., null or .exe).
#	BEGINFILES - the list of files that `make begin' and `make clean'
#		should delete.
#	CCAUX - the C invocation for auxiliary programs (ansi2knr, echogs,
#		genarch, genconf).
#	CCBEGIN - the compilation command for `make begin', normally
#		$(CCC) *.c.
#	CCC - the C invocation for normal compilation.
#	CCD - the C invocation for files that store into frame buffers or
#		device registers.  Needed because some optimizing compilers
#		will eliminate necessary stores.
#	CCCF - the C invocation for compiled fonts and other large,
#		self-contained data modules.  Needed because MS-DOS
#		requires using the 'huge' memory model for these.
#	CCINT - the C invocation for compiling the main interpreter module,
#		normally the same as CCC: this is needed because the
#		Borland compiler generates *worse* code for this module
#		(but only this module) when optimization (-O) is turned on.
#	AK - if source files must be converted from ANSI to K&R syntax,
#		this is ansi2knr$(XE); if not, it is null.
#		If a particular platform requires other utility programs
#		to be built, AK must include them too.
#	SHP - the prefix for invoking a shell script in the current directory
#		(null for MS-DOS, $(SH) ./ for Unix).
#	EXPP, EXP - the prefix for invoking an executable program in the
#		current directory (null for MS-DOS, ./ for Unix).
#	SH - the shell for scripts (null on MS-DOS, sh on Unix).
#	CONFILES - the arguments for genconf to generate the appropriate
#		linker control files (various).
#
# The platform-specific makefiles must also include rules for creating
# certain dynamically generated files:
#	gconfig_.h - this indicates the presence or absence of
#	    certain system header files that are located in different
#	    places on different systems.  (It could be generated by
#	    the GNU `configure' program.)

all default: $(GS)$(XE)

distclean realclean: clean
	rm -f makefile

clean mostlyclean:
	rm -f *.$(OBJ) *.a core gmon.out
	rm -f *.dev *.d_* arch.h devs.tr gconfig*.h j*.h o*.tr l*.tr
	rm -f t _temp_* _temp_*.* *.map *.sym
	rm -f ansi2knr$(XE) echogs$(XE) genarch$(XE) genconf$(XE)
	rm -f $(GS)$(XE) $(BEGINFILES)

# A rule to do a quick and dirty compilation attempt when first installing
# the interpreter.  Many of the compilations will fail:
# follow this with 'make'.

begin:
	rm -f arch.h gconfig*.h genarch$(XE) $(GS)$(XE) $(BEGINFILES)
	make arch.h gconfigv.h
	- $(CCBEGIN)
	rm -f gconfig.$(OBJ) gdev*.$(OBJ) gp_*.$(OBJ) gsmisc.$(OBJ)
	rm -f iccfont.$(OBJ) iinit.$(OBJ) interp.$(OBJ) ziodev.$(OBJ)

# Auxiliary programs

arch.h: genarch$(XE)
	$(EXPP) $(EXP)genarch arch.h

# Macros for constructing the *.dev files that describe features and
# devices.  We may replace these with echogs variants someday....
SETDEV=$(SHP)gssetdev$(CMD)
SETMOD=$(SHP)gssetmod$(CMD)
ADDMOD=$(SHP)gsaddmod$(CMD)

# -------------------------------- Library -------------------------------- #

# Define the inter-dependencies of the .h files.
# Since not all versions of `make' defer expansion of macros,
# we must list these in bottom-to-top order.

# Generic files

arch_h=arch.h
stdpre_h=stdpre.h
std_h=std.h $(arch_h) $(stdpre_h)

# Platform interfaces

gp_h=gp.h
gpcheck_h=gpcheck.h

# Configuration definitions

# gconfig*.h are generated dynamically.
gconfig__h=gconfig_.h
gconfigv_h=gconfigv.h
gscdefs_h=gscdefs.h

# C library interfaces

# Because of variations in the "standard" header files between systems, and
# because we must include std.h before any file that includes sys/types.h,
# we define local include files named *_.h to substitute for <*.h>.

vmsmath_h=vmsmath.h

dos__h=dos_.h
ctype__h=ctype_.h $(std_h)
dirent__h=dirent_.h $(std_h) $(gconfig__h)
errno__h=errno_.h
malloc__h=malloc_.h $(std_h)
math__h=math_.h $(std_h) $(vmsmath_h)
memory__h=memory_.h $(std_h)
stat__h=stat_.h $(std_h)
stdio__h=stdio_.h $(std_h)
string__h=string_.h $(std_h)
time__h=time_.h $(std_h) $(gconfig__h)
windows__h=windows_.h

# Miscellaneous

gconfig_h=gconfig.h
gdebug_h=gdebug.h
gserror_h=gserror.h
gserrors_h=gserrors.h
gsexit_h=gsexit.h
gsio_h=gsio.h
gsmemory_h=gsmemory.h
gsrefct_h=gsrefct.h
gsstruct_h=gsstruct.h
gstypes_h=gstypes.h
gx_h=gx.h $(stdio__h) $(gdebug_h) $(gserror_h) $(gsio_h) $(gsmemory_h) $(gstypes_h)

GX=$(AK) $(gx_h)
GXERR=$(GX) $(gserrors_h)

###### Support

### Include files

gsbitops_h=gsbitops.h
gsuid_h=gsuid.h
gsutil_h=gsutil.h

### Executable code

gsbitops.$(OBJ): gsbitops.c $(AK) $(std_h) $(memory__h) $(gsbitops_h)

gsmemory.$(OBJ): gsmemory.c $(GX) \
  $(gsrefct_h) $(gsstruct_h)

gsmisc.$(OBJ): gsmisc.c $(GXERR) $(gconfigv_h) \
  $(memory__h) $(gxfixed_h)

gsutil.$(OBJ): gsutil.c $(AK) $(gconfigv_h) \
  $(std_h) $(gsuid_h) $(gsutil_h)


###### Low-level facilities and utilities

### Include files

gsccode_h=gsccode.h
gsccolor_h=gsccolor.h $(gsstruct_h)
gschar_h=gschar.h $(gsccode_h)
gscie_h=gscie.h $(gsrefct_h)
gscolor1_h=gscolor.h
gscoord_h=gscoord.h
gsdevice_h=gsdevice.h
gsfont_h=gsfont.h
gshsb_h=gshsb.h
gsht_h=gsht.h
gsht1_h=gsht1.h $(gsht_h)
gsimage_h=gsimage.h
gsiscale_h=gsiscale.h
gsjconf_h=gsjconf.h $(std_h)
gsline_h=gsline.h
gsmatrix_h=gsmatrix.h
gspaint_h=gspaint.h
gsparam_h=gsparam.h
gspath_h=gspath.h
gspath2_h=gspath2.h
gsxfont_h=gsxfont.h
# Out of order
gscolor2_h=gscolor2.h $(gsccolor_h) $(gsuid_h)

gxarith_h=gxarith.h
gxbitmap_h=gxbitmap.h
gxchar_h=gxchar.h $(gschar_h)
gxclist_h=gxclist.h
gxclip2_h=gxclip2.h
gxcolor2_h=gxcolor2.h $(gscolor2_h) $(gsrefct_h) $(gxbitmap_h)
gxcoord_h=gxcoord.h $(gscoord_h)
gxcpath_h=gxcpath.h
gxdcolor_h=gxdcolor.h $(gsrefct_h) $(gsstruct_h) $(gxbitmap_h)
gxdevice_h=gxdevice.h $(gsmatrix_h) $(gsxfont_h) $(gxbitmap_h) $(gxdcolor_h)
gxdevmem_h=gxdevmem.h
gxdither_h=gxdither.h
gxdraw_h=gxdraw.h
gxfarith_h=gxfarith.h $(gconfigv_h) $(gxarith_h)
gxfcache_h=gxfcache.h $(gsuid_h) $(gsxfont_h)
gxfixed_h=gxfixed.h
gxfont_h=gxfont.h $(gsfont_h) $(gsuid_h) $(gsstruct_h)
gxfont0_h=gxfont0.h
gxfrac_h=gxfrac.h
gximage_h=gximage.h $(gscspace_h) $(gsimage_h) $(gsiscale_h)
gxiodev_h=gxiodev.h $(stat__h)
gxlum_h=gxlum.h
gxmatrix_h=gxmatrix.h $(gsmatrix_h)
gxpaint_h=gxpaint.h
gxpath_h=gxpath.h $(gspath_h)
gxpcolor_h=gxpcolor.h
gxtmap_h=gxtmap.h
gxxfont_h=gxxfont.h $(gsccode_h) $(gsmatrix_h) $(gsuid_h) $(gsxfont_h)
# The following are out of order because they include other files.
gscspace_h=gscspace.h $(gsccolor_h) $(gsstruct_h) $(gxfrac_h)
gxcldev_h=gxcldev.h $(gxclist_h)
gxdcconv_h=gxdcconv.h $(gxfrac_h) $(gsccolor_h)
gxfmap_h=gxfmap.h $(gsrefct_h) $(gxfrac_h) $(gxtmap_h)
gxcmap_h=gxcmap.h $(gxfmap_h)
gxht_h=gxht.h $(gsht1_h) $(gxtmap_h)
gscolor_h=gscolor.h $(gxtmap_h)
gsstate_h=gsstate.h $(gscolor_h) $(gsdevice_h) $(gsht_h) $(gsline_h)

gzcpath_h=gzcpath.h $(gxcpath_h)
gzht_h=gzht.h $(gxfmap_h) $(gxht_h)
gzline_h=gzline.h $(gsline_h)
gzpath_h=gzpath.h $(gsstruct_h) $(gxpath_h)
gzstate_h=gzstate.h $(gsstate_h) $(gxdcolor_h) $(gxfixed_h) $(gxmatrix_h) $(gxdevice_h) $(gxtmap_h)

gdevprn_h=gdevprn.h $(memory__h) $(string__h) $(gx_h) \
  $(gserrors_h) $(gsmatrix_h) $(gsutil_h) \
  $(gxdevice_h) $(gxdevmem_h) $(gxclist_h)

### Executable code

gxacpath.$(OBJ): gxacpath.c $(GXERR) \
  $(gsstruct_h) $(gxdevice_h) $(gxcpath_h) $(gxfixed_h) $(gxpaint_h) \
  $(gzcpath_h) $(gzpath_h)

gxccache.$(OBJ): gxccache.c $(GXERR) $(gpcheck_h) \
  $(gscspace_h) $(gsimage_h) $(gsstruct_h) \
  $(gxchar_h) $(gxdevmem_h) $(gxfcache_h) \
  $(gxfixed_h) $(gxfont_h) $(gxmatrix_h) $(gxxfont_h) \
  $(gzstate_h) $(gzpath_h) $(gzcpath_h) 

gxccman.$(OBJ): gxccman.c $(GXERR) $(gpcheck_h) \
  $(gsbitops_h) $(gsstruct_h) $(gsutil_h) $(gxfixed_h) $(gxmatrix_h) \
  $(gxdevmem_h) $(gxfont_h) $(gxfcache_h) $(gxchar_h) \
  $(gxxfont_h) $(gzstate_h) $(gzpath_h)

gxclist.$(OBJ): gxclist.c $(GXERR) $(gpcheck_h) \
  $(gsmatrix_h) $(gxbitmap_h) $(gxcldev_h) $(gxdevice_h) $(gxdevmem_h) \
  $(srlx_h) $(strimpl_h)

gxclread.$(OBJ): gxclread.c $(GXERR) $(gpcheck_h) \
  $(gsdevice_h) $(gsmatrix_h) \
  $(gxbitmap_h) $(gxcldev_h) $(gxdevice_h) $(gxdevmem_h) \
  $(srlx_h) $(strimpl_h)

gxcht.$(OBJ): gxcht.c $(GXERR) \
  $(gxcmap_h) $(gxdevice_h) $(gxfixed_h) $(gxmatrix_h) $(gzht_h) $(gzstate_h)

gxcmap.$(OBJ): gxcmap.c $(GXERR) \
  $(gsccolor_h) $(gscspace_h) \
  $(gxcmap_h) $(gxdcconv_h) $(gxdevice_h) $(gxdither_h) \
  $(gxfarith_h) $(gxfrac_h) $(gxlum_h) $(gzstate_h)

gxcpath.$(OBJ): gxcpath.c $(GXERR) \
  $(gsstruct_h) $(gxdevice_h) $(gxfixed_h) $(gzpath_h) $(gzcpath_h)

gxdcconv.$(OBJ): gxdcconv.c $(GX) \
  $(gxcmap_h) $(gxdcconv_h) $(gxdcolor_h) $(gxdevice_h) \
  $(gxfarith_h) $(gxlum_h) $(gzstate_h)

gxdither.$(OBJ): gxdither.c $(GX) \
  $(gxcmap_h) $(gxdither_h) $(gxfixed_h) $(gxlum_h) $(gxmatrix_h) \
  $(gzstate_h) $(gzht_h)

gxdraw.$(OBJ): gxdraw.c $(GXERR) $(gpcheck_h) \
  $(gxbitmap_h) $(gxdraw_h) $(gxfixed_h) $(gxmatrix_h) $(gzht_h) $(gzstate_h)

gxfill.$(OBJ): gxfill.c $(GXERR) \
  $(gsstruct_h) \
  $(gxdevice_h) $(gxdraw_h) $(gxfixed_h) $(gxmatrix_h) \
  $(gzcpath_h) $(gzpath_h) $(gzstate_h)

gxht.$(OBJ): gxht.c $(GXERR) \
  $(gsstruct_h) $(gsutil_h) $(gxfixed_h) $(gxdevice_h) $(gzstate_h) $(gzht_h)

gxpath.$(OBJ): gxpath.c $(GXERR) \
  $(gsstruct_h) $(gxfixed_h) $(gzpath_h)

gxpath2.$(OBJ): gxpath2.c $(GXERR) \
  $(gxfixed_h) $(gxarith_h) $(gzpath_h)

gxpcopy.$(OBJ): gxpcopy.c $(GXERR) \
  $(gsmatrix_h) $(gscoord_h) $(gxfixed_h) $(gxarith_h) $(gzline_h) $(gzpath_h)

gxstroke.$(OBJ): gxstroke.c $(GXERR) $(gpcheck_h) \
  $(gscoord_h) \
  $(gxdraw_h) $(gxfixed_h) $(gxarith_h) $(gxmatrix_h) $(gxpaint_h) \
  $(gzline_h) $(gzpath_h) $(gzstate_h)

###### High-level facilities

gschar.$(OBJ): gschar.c $(GXERR) \
  $(gsstruct_h) \
  $(gxfixed_h) $(gxarith_h) $(gxmatrix_h) $(gxcoord_h) $(gxdevmem_h) \
  $(gxfont_h) $(gxfont0_h) $(gxchar_h) $(gxfcache_h) $(gzpath_h) $(gzstate_h)

gscolor.$(OBJ): gscolor.c $(GXERR) \
  $(gsccolor_h) $(gscspace_h) $(gsstruct_h) \
  $(gxcmap_h) $(gxdcconv_h) $(gxdevice_h) $(gzstate_h)

gscoord.$(OBJ): gscoord.c $(GXERR) \
  $(gsccode_h) $(gxcoord_h) $(gxfarith_h) $(gxfixed_h) $(gxfont_h) \
  $(gxmatrix_h) $(gxpath_h) $(gzstate_h)

gsdevice.$(OBJ): gsdevice.c $(GXERR) \
  $(gscoord_h) $(gsmatrix_h) $(gspaint_h) $(gsparam_h) $(gspath_h) $(gsstruct_h) \
  $(gxarith_h) $(gxbitmap_h) $(gxcmap_h) $(gxdevmem_h) $(gzstate_h)

gsdparam.$(OBJ): gsdparam.c $(GXERR) \
  $(gsparam_h) $(gxdevice_h) $(gxfixed_h)

gsfont.$(OBJ): gsfont.c $(GXERR) \
  $(gsstruct_h) \
  $(gxdevice_h) $(gxfixed_h) $(gxmatrix_h) $(gxfont_h) $(gxfcache_h) \
  $(gzstate_h)

gsht.$(OBJ): gsht.c $(GXERR) \
  $(gsstruct_h) $(gxdevice_h) $(gzht_h) $(gzstate_h)

gshtscr.$(OBJ): gshtscr.c $(GXERR) \
  $(gsstruct_h) $(gxarith_h) $(gxdevice_h) $(gzht_h) $(gzstate_h)

gsimage.$(OBJ): gsimage.c $(GXERR) $(gpcheck_h) \
  $(gsccolor_h) $(gspaint_h) $(gsstruct_h) \
  $(gxfixed_h) $(gxfrac_h) $(gxarith_h) $(gxmatrix_h) \
  $(gxdevice_h) $(gzpath_h) $(gzstate_h) \
  $(gzcpath_h) $(gxdevmem_h) $(gximage_h)

gsimage1.$(OBJ): gsimage1.c $(GXERR) $(gpcheck_h) \
  $(gsccolor_h) $(gscspace_h) $(gspaint_h) \
  $(gxarith_h) $(gxcmap_h) $(gxcpath_h) $(gxdevmem_h) \
  $(gxdraw_h) $(gxfixed_h) $(gximage_h) $(gxmatrix_h) \
  $(gzht_h) $(gzpath_h) $(gzstate_h)

gsimage2.$(OBJ): gsimage2.c $(GXERR) $(gpcheck_h) \
  $(gsccolor_h) $(gscspace_h) $(gspaint_h) \
  $(gxarith_h) $(gxcmap_h) $(gxcpath_h) $(gxdevmem_h) \
  $(gxdraw_h) $(gxfixed_h) $(gximage_h) $(gxmatrix_h) \
  $(gzht_h) $(gzpath_h) $(gzstate_h)

gsimage3.$(OBJ): gsimage3.c $(GXERR) $(gpcheck_h) \
  $(gsccolor_h) $(gscspace_h) $(gspaint_h) \
  $(gxarith_h) $(gxcmap_h) $(gxcpath_h) $(gxdevmem_h) \
  $(gxdraw_h) $(gxfixed_h) $(gximage_h) $(gxmatrix_h) \
  $(gzht_h) $(gzpath_h) $(gzstate_h)

gsimpath.$(OBJ): gsimpath.c $(GXERR) \
  $(gsmatrix_h) $(gsstate_h) $(gspath_h)

gsiodev.$(OBJ): gsiodev.c $(GXERR) $(errno__h) $(string__h) \
  $(gp_h) $(gsparam_h) $(gxiodev_h)

# Filtered image scaling is a Level 2 feature, but it's too much work
# to break it out of the Level 1 library.
gsiscale.$(OBJ): gsiscale.c $(math__h) $(GXERR) $(gsiscale_h)

gsline.$(OBJ): gsline.c $(GXERR) \
  $(gxfixed_h) $(gxmatrix_h) $(gzstate_h) $(gzline_h)

gsmatrix.$(OBJ): gsmatrix.c $(GXERR) \
  $(gxfarith_h) $(gxfixed_h) $(gxmatrix_h)

gspaint.$(OBJ): gspaint.c $(GXERR) $(gpcheck_h) \
  $(gxfixed_h) $(gxmatrix_h) $(gspaint_h) $(gxpaint_h) \
  $(gzpath_h) $(gzstate_h) $(gxcpath_h) $(gxdevmem_h) $(gximage_h)

gspath.$(OBJ): gspath.c $(GXERR) \
  $(gscoord_h) $(gxfixed_h) $(gxmatrix_h) $(gxpath_h) $(gzstate_h)

gspath2.$(OBJ): gspath2.c $(GXERR) \
  $(gspath_h) $(gsstruct_h) $(gxfixed_h) $(gxmatrix_h) \
  $(gzstate_h) $(gzpath_h) $(gzcpath_h)

gsstate.$(OBJ): gsstate.c $(GXERR) \
  $(gscie_h) $(gscolor2_h) $(gscoord_h) $(gscspace_h) $(gsstruct_h) \
  $(gxcmap_h) \
  $(gzstate_h) $(gzht_h) $(gzline_h) $(gzpath_h) $(gzcpath_h)

gswppm.$(OBJ): gswppm.c $(GXERR) \
  $(gscdefs_h) $(gsmatrix_h) $(gxdevice_h) $(gxdevmem_h)

###### The internal devices

gdevmem_h=gdevmem.h $(gsbitops_h)

gdevabuf.$(OBJ): gdevabuf.c $(AK) \
  $(gx_h) $(gserrors_h) $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)

gdevmem.$(OBJ): gdevmem.c $(AK) \
  $(gx_h) $(gserrors_h) $(gsstruct_h) $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)

gdevm1.$(OBJ): gdevm1.c $(AK) \
  $(gx_h) $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)

gdevm2.$(OBJ): gdevm2.c $(AK) \
  $(gx_h) $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)

gdevm4.$(OBJ): gdevm4.c $(AK) \
  $(gx_h) $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)

gdevm8.$(OBJ): gdevm8.c $(AK) \
  $(gx_h) $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)

gdevm16.$(OBJ): gdevm16.c $(AK) \
  $(gx_h) $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)

gdevm24.$(OBJ): gdevm24.c $(AK) \
  $(gx_h) $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)

gdevm32.$(OBJ): gdevm32.c $(AK) \
  $(gx_h) $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)

gdevmpla.$(OBJ): gdevmpla.c $(AK) \
  $(gx_h) $(gxdevice_h) $(gxdevmem_h) $(gdevmem_h)

# Create a pseudo-"feature" for the entire (Level 1 base) library.

LIB1=gsbitops.$(OBJ) gschar.$(OBJ) gscolor.$(OBJ) gscoord.$(OBJ)
LIB2=gsdevice.$(OBJ) gsdparam.$(OBJ) gsfont.$(OBJ) gsht.$(OBJ) gshtscr.$(OBJ)
LIB3=gsimage.$(OBJ) gsimage1.$(OBJ) gsimage2.$(OBJ) gsimage3.$(OBJ)
LIB4=gsimpath.$(OBJ) gsiodev.$(OBJ) gsiscale.$(OBJ)
LIB5=gsline.$(OBJ) gsmatrix.$(OBJ) gsmemory.$(OBJ) gsmisc.$(OBJ)
LIB6=gspaint.$(OBJ) gspath.$(OBJ) gspath2.$(OBJ) gsstate.$(OBJ) gswppm.$(OBJ)
LIB7=gsutil.$(OBJ) gxacpath.$(OBJ) gxccache.$(OBJ) gxccman.$(OBJ)
LIB8=gxcht.$(OBJ) gxclist.$(OBJ) gxclread.$(OBJ) gxcmap.$(OBJ) gxcpath.$(OBJ)
LIB9=gxdcconv.$(OBJ) gxdither.$(OBJ) gxdraw.$(OBJ) gxfill.$(OBJ)
LIB10=gxht.$(OBJ) gxpath.$(OBJ) gxpath2.$(OBJ) gxpcopy.$(OBJ) gxstroke.$(OBJ)
LIB11=gdevabuf.$(OBJ) gdevmem.$(OBJ) gdevm1.$(OBJ) gdevm2.$(OBJ) gdevm4.$(OBJ)
LIB12=gdevm8.$(OBJ) gdevm16.$(OBJ) gdevm24.$(OBJ) gdevm32.$(OBJ) gdevmpla.$(OBJ)
LIB_ALL=$(LIB1) $(LIB2) $(LIB3) $(LIB4) $(LIB5) $(LIB6) $(LIB7) $(LIB8) \
  $(LIB9) $(LIB10) $(LIB11) $(LIB12)
gslib.dev: gs.mak echogs$(XE) $(LIB_ALL)
	$(EXP)echogs -w gslib.dev
	$(EXP)echogs -a gslib.dev $(LIB1)
	$(EXP)echogs -a gslib.dev $(LIB2)
	$(EXP)echogs -a gslib.dev $(LIB3)
	$(EXP)echogs -a gslib.dev $(LIB4)
	$(EXP)echogs -a gslib.dev $(LIB5)
	$(EXP)echogs -a gslib.dev $(LIB6)
	$(EXP)echogs -a gslib.dev $(LIB7)
	$(EXP)echogs -a gslib.dev $(LIB8)
	$(EXP)echogs -a gslib.dev $(LIB9)
	$(EXP)echogs -a gslib.dev $(LIB10)
	$(EXP)echogs -a gslib.dev $(LIB11)
	$(EXP)echogs -a gslib.dev $(LIB12)

# ---------------- Interpreter support ---------------- #

### Include files

errors_h=errors.h
idebug_h=idebug.h
idict_h=idict.h
idparam_h=idparam.h
igc_h=igc.h
ilevel_h=ilevel.h
iname_h=iname.h
ipacked_h=ipacked.h
iparam_h=iparam.h $(gsparam_h)
iref_h=iref.h
isave_h=isave.h
isstate_h=isstate.h
istack_h=istack.h
istruct_h=istruct.h $(gsstruct_h)
iutil_h=iutil.h
ivmspace_h=ivmspace.h
opcheck_h=opcheck.h
opdef_h=opdef.h
# Nested include files
dstack_h=dstack.h $(istack_h)
estack_h=estack.h $(istack_h)
ghost_h=ghost.h $(gx_h) $(iref_h)
imemory_h=imemory.h $(ivmspace_h)
ialloc_h=ialloc.h $(imemory_h)
iastruct_h=iastruct.h $(gxbitmap_h) $(ialloc_h)
iastate_h=iastate.h $(iastruct_h) $(istruct_h)
ostack_h=ostack.h $(istack_h)
oper_h=oper.h $(iutil_h) $(opcheck_h) $(opdef_h) $(ostack_h)
store_h=store.h $(ialloc_h)

GH=$(AK) $(ghost_h)

ialloc.$(OBJ): ialloc.c $(AK) $(gx_h) \
  $(errors_h) $(gsstruct_h) $(gxarith_h) $(iastate_h) $(iref_h) $(ivmspace_h) $(store_h)

idebug.$(OBJ): idebug.c $(GH) \
  $(ialloc_h) $(idebug_h) $(idict_h) $(iname_h) $(istack_h) $(iutil_h) $(ivmspace_h) \
  $(ostack_h) $(opdef_h) $(ipacked_h) $(store_h)

idict.$(OBJ): idict.c $(GH) $(errors_h) \
  $(ialloc_h) $(idebug_h) $(ivmspace_h) $(iname_h) $(ipacked_h) \
  $(isave_h) $(store_h) $(iutil_h) $(idict_h) $(dstack_h)

idparam.$(OBJ): idparam.c $(GH) $(errors_h) \
  $(gsmatrix_h) $(gsuid_h) \
  $(idict_h) $(idparam_h) $(ilevel_h) $(imemory_h) $(iname_h) $(iutil_h) \
  $(oper_h) $(store_h)

# igc.c, igcref.c, and igcstr.c should really be in the dpsand2 list,
# but since all the GC enumeration and relocation routines refer to them,
# it's too hard to separate them out from the Level 1 base.
igc.$(OBJ): igc.c $(GH) \
  $(gsexit_h) $(gsstruct_h) $(gsutil_h) \
  $(iastate_h) $(idict_h) $(igc_h) $(iname_h) $(isave_h) $(isstate_h) \
  $(dstack_h) $(errors_h) $(estack_h) $(opdef_h) $(ostack_h) $(ipacked_h) $(store_h)

igcref.$(OBJ): igcref.c $(GH)\
  $(gsexit_h) $(iastate_h) $(idebug_h) $(igc_h) $(iname_h) $(ipacked_h)

igcstr.$(OBJ): igcstr.c $(GH) \
  $(gsstruct_h) $(iastate_h) $(igc_h) $(isave_h) $(isstate_h)

iname.$(OBJ): iname.c $(GH) $(gsstruct_h) $(errors_h) $(iname_h) $(store_h)

iparam.$(OBJ): iparam.c $(GH) \
  $(ialloc_h) $(idict_h) $(iname_h) $(imemory_h) $(iparam_h) $(istack_h) $(iutil_h) $(ivmspace_h) \
  $(opcheck_h) $(store_h)

isave.$(OBJ): isave.c $(GH) \
  $(errors_h) $(gsexit_h) $(gsstruct_h) \
  $(iastate_h) $(iname_h) $(isave_h) $(isstate_h) $(ivmspace_h) \
  $(ipacked_h) $(store_h)

istack.$(OBJ): istack.c $(GH) $(memory__h) \
  $(errors_h) $(gsstruct_h) $(gsutil_h) \
  $(ialloc_h) $(istack_h) $(istruct_h) $(iutil_h) $(ivmspace_h) $(store_h)

iutil.$(OBJ): iutil.c $(OP) \
  $(errors_h) $(idict_h) $(imemory_h) $(iutil_h) $(ivmspace_h) \
  $(iname_h) $(ipacked_h) $(store_h) \
  $(gsmatrix_h) $(gsutil_h)

# -------------------------- Level 1 Interpreter -------------------------- #

###### Include files

files_h=files.h
fname_h=fname.h
ichar_h=ichar.h
icolor_h=icolor.h
icsmap_h=icsmap.h
ifont_h=ifont.h $(gsstruct_h)
interp_h=interp.h
iparray_h=iparray.h
iscan_h=iscan.h
iscannum_h=iscannum.h
istream_h=istream.h
main_h=main.h $(gsexit_h)
overlay_h=overlay.h
sbwbs_h=sbwbs.h
scanchar_h=scanchar.h
scommon_h=scommon.h $(gsmemory_h) $(gstypes_h) $(gsstruct_h)
sdct_h=sdct.h
sfilter_h=sfilter.h $(gstypes_h)
shc_h=shc.h
shcgen_h=shcgen.h
sjpeg_h=sjpeg.h
slzwx_h=slzwx.h
srlx_h=srlx.h
stream_h=stream.h $(scommon_h)
strimpl_h=strimpl.h $(scommon_h) $(gstypes_h) $(gsstruct_h)
# Nested include files
bfont_h=bfont.h $(ifont_h)
ifilter_h=ifilter.h $(istream_h) $(ivmspace_h)
igstate_h=igstate.h $(gsstate_h) $(istruct_h)
sbhc_h=sbhc.h $(shc_h)
scf_h=scf.h $(shc_h)
scfx_h=scfx.h $(shc_h)
# Include files for optional features
ibnum_h=ibnum.h

### Initialization and scanning

iinit.$(OBJ): iinit.c $(GH) $(gconfig_h) \
  $(gscdefs_h) $(gsexit_h) $(gsstruct_h) \
  $(ialloc_h) $(idict_h) $(dstack_h) $(errors_h) \
  $(ilevel_h) $(iname_h) $(interp_h) $(oper_h) \
  $(ipacked_h) $(iparray_h) $(ivmspace_h) $(store_h)

iscan.$(OBJ): iscan.c $(GH) $(ctype__h) \
  $(ialloc_h) $(idict_h) $(dstack_h) $(errors_h) $(files_h) \
  $(ilevel_h) $(iutil_h) $(iscan_h) $(iscannum_h) $(istruct_h) $(ivmspace_h) \
  $(iname_h) $(ipacked_h) $(iparray_h) $(istream_h) $(ostack_h) $(store_h) \
  $(stream_h) $(strimpl_h) $(sfilter_h) $(scanchar_h)

iscannum.$(OBJ): iscannum.c $(GH) \
  $(errors_h) $(iscannum_h) $(scanchar_h) $(store_h) $(stream_h)

iscantab.$(OBJ): iscantab.c $(AK) \
  $(stdpre_h) $(scommon_h) $(scanchar_h)

### Streams

sfile.$(OBJ): sfile.c $(AK) $(stdio__h) $(memory__h) \
  $(gdebug_h) $(gpcheck_h) $(stream_h) $(strimpl_h)

sfileno.$(OBJ): sfileno.c $(AK) $(stdio__h) $(errno__h) $(memory__h) \
  $(gdebug_h) $(gpcheck_h) $(stream_h) $(strimpl_h)

sfilter1.$(OBJ): sfilter1.c $(AK) $(stdio__h) $(memory__h) \
  $(sfilter_h) $(srlx_h) $(strimpl_h)

sstring.$(OBJ): sstring.c $(AK) $(stdio__h) $(memory__h) \
  $(scanchar_h) $(sfilter_h) $(strimpl_h)

stream.$(OBJ): stream.c $(AK) $(stdio__h) $(memory__h) \
  $(gdebug_h) $(gpcheck_h) $(stream_h) $(strimpl_h)

###### Operators

OP=$(GH) $(errors_h) $(oper_h)

### Non-graphics operators

zarith.$(OBJ): zarith.c $(OP) $(store_h)

zarray.$(OBJ): zarray.c $(OP) $(ialloc_h) $(ipacked_h) $(store_h)

zcontrol.$(OBJ): zcontrol.c $(OP) \
  $(estack_h) $(ipacked_h) $(iutil_h) $(store_h)

zdict.$(OBJ): zdict.c $(OP) \
  $(dstack_h) $(idict_h) $(ilevel_h) $(iname_h) $(ipacked_h) $(ivmspace_h) \
  $(store_h)

zfile.$(OBJ): zfile.c $(OP) $(stat__h) $(gp_h) \
  $(gsstruct_h) $(gxiodev_h) \
  $(ialloc_h) $(estack_h) $(files_h) $(fname_h) $(ilevel_h) $(interp_h) $(iutil_h) \
  $(isave_h) $(sfilter_h) $(stream_h) $(strimpl_h) $(store_h)

zfname.$(OBJ): zfname.c $(OP) \
  $(fname_h) $(gxiodev_h) $(ialloc_h) $(stream_h)

zfileio.$(OBJ): zfileio.c $(OP) $(gp_h) \
  $(files_h) $(ifilter_h) $(store_h) $(stream_h) $(strimpl_h) \
  $(gsmatrix_h) $(gxdevice_h) $(gxdevmem_h)

zfilter.$(OBJ): zfilter.c $(OP) \
  $(gsstruct_h) $(files_h) $(ialloc_h) $(ifilter_h) \
  $(sfilter_h) $(srlx_h) $(stream_h) $(strimpl_h)

zfproc.$(OBJ): zfproc.c $(GH) $(errors_h) $(oper_h) \
  $(estack_h) $(files_h) $(gsstruct_h) $(ialloc_h) $(ifilter_h) $(istruct_h) \
  $(store_h) $(stream_h) $(strimpl_h)

zgeneric.$(OBJ): zgeneric.c $(OP) \
  $(idict_h) $(estack_h) $(ivmspace_h) $(iname_h) $(ipacked_h) $(store_h)

ziodev.$(OBJ): ziodev.c $(OP) $(string__h) $(gp_h) $(gpcheck_h) $(gconfig_h) \
  $(gsparam_h) $(gsstruct_h) $(gxiodev_h) \
  $(files_h) $(ialloc_h) $(ivmspace_h) $(store_h) $(stream_h)

zmath.$(OBJ): zmath.c $(OP) $(store_h)

zmisc.$(OBJ): zmisc.c $(OP) $(gscdefs_h) $(gp_h) \
  $(errno__h) $(memory__h) $(string__h) \
  $(ialloc_h) $(idict_h) $(dstack_h) $(iname_h) $(ivmspace_h) $(ipacked_h) $(store_h)

zpacked.$(OBJ): zpacked.c $(OP) \
  $(ialloc_h) $(idict_h) $(ivmspace_h) $(iname_h) $(ipacked_h) $(iparray_h) \
  $(istack_h) $(store_h)

zrelbit.$(OBJ): zrelbit.c $(OP) $(gsutil_h) $(store_h) $(idict_h)

zstack.$(OBJ): zstack.c $(OP) $(ialloc_h) $(istack_h) $(store_h)

zstring.$(OBJ): zstring.c $(OP) $(gsutil_h) \
  $(ialloc_h) $(iname_h) $(ivmspace_h) $(store_h)

zsysvm.$(OBJ): zsysvm.c $(OP)

ztoken.$(OBJ): ztoken.c $(OP) \
  $(estack_h) $(files_h) $(gsstruct_h) $(iscan_h) \
  $(sfilter_h) $(store_h) $(stream_h) $(strimpl_h)

ztype.$(OBJ): ztype.c $(OP) \
  $(dstack_h) $(idict_h) $(imemory_h) $(iname_h) \
  $(iscannum_h) $(iutil_h) $(store_h)

zvmem.$(OBJ): zvmem.c $(OP) \
  $(ialloc_h) $(idict_h) $(dstack_h) $(estack_h) $(isave_h) $(igstate_h) $(store_h) \
  $(gsmatrix_h) $(gsstate_h) $(gsstruct_h)

### Graphics operators

zchar.$(OBJ): zchar.c $(OP) \
  $(gsstruct_h) $(gxarith_h) $(gxfixed_h) $(gxmatrix_h) \
  $(gschar_h) $(gxdevice_h) $(gxfont_h) $(gzpath_h) $(gzstate_h) \
  $(ialloc_h) $(ichar_h) $(idict_h) $(ifont_h) $(estack_h) $(ilevel_h) $(iname_h) $(igstate_h) $(store_h)

zcolor.$(OBJ): zcolor.c $(OP) \
  $(gxfixed_h) $(gxmatrix_h) $(gzstate_h) $(gxdevice_h) $(gxcmap_h) \
  $(ialloc_h) $(icolor_h) $(estack_h) $(iutil_h) $(igstate_h) $(store_h)

zdevice.$(OBJ): zdevice.c $(OP) \
  $(ialloc_h) $(idict_h) $(igstate_h) $(iname_h) $(interp_h) $(iparam_h) $(ivmspace_h) \
  $(gsmatrix_h) $(gsstate_h) $(gxdevice_h) $(store_h)

zfont.$(OBJ): zfont.c $(OP) \
  $(gsmatrix_h) $(gxdevice_h) $(gxfont_h) $(gxfcache_h) \
  $(ialloc_h) $(idict_h) $(igstate_h) $(iname_h) $(isave_h) $(ivmspace_h) \
  $(bfont_h) $(store_h)

zfont2.$(OBJ): zfont2.c $(OP) \
  $(gsmatrix_h) $(gxdevice_h) $(gschar_h) $(gxfixed_h) $(gxfont_h) \
  $(ialloc_h) $(bfont_h) $(idict_h) $(idparam_h) $(ilevel_h) $(iname_h) $(istruct_h) \
  $(ipacked_h) $(store_h)

zgstate.$(OBJ): zgstate.c $(OP) \
  $(gsmatrix_h) $(ialloc_h) $(igstate_h) $(istruct_h) $(store_h)

zht.$(OBJ): zht.c $(OP) \
  $(gsmatrix_h) $(gsstate_h) $(gsstruct_h) $(gxdevice_h) $(gzht_h) \
  $(ialloc_h) $(estack_h) $(igstate_h) $(store_h)

zmatrix.$(OBJ): zmatrix.c $(OP) $(gsmatrix_h) $(igstate_h) $(gscoord_h) $(store_h)

zpaint.$(OBJ): zpaint.c $(OP) \
  $(ialloc_h) $(estack_h) $(ilevel_h) $(igstate_h) $(store_h) $(stream_h) \
  $(gsimage_h) $(gsmatrix_h) $(gspaint_h) $(gsstruct_h)

zpath.$(OBJ): zpath.c $(OP) $(gsmatrix_h) $(gspath_h) $(igstate_h) $(store_h)

zpath2.$(OBJ): zpath2.c $(OP) \
  $(ialloc_h) $(estack_h) $(gspath_h) $(gsstruct_h) $(igstate_h) $(store_h)

# Create a pseudo-"feature" for the entire (Level 1 base) interpreter.

INT1=ialloc.$(OBJ) idebug.$(OBJ) idict.$(OBJ) idparam.$(OBJ) igc.$(OBJ) igcref.$(OBJ)
INT2=igcstr.$(OBJ) iinit.$(OBJ) iname.$(OBJ) interp.$(OBJ) iparam.$(OBJ)
INT3=isave.$(OBJ) iscan.$(OBJ) iscannum.$(OBJ) iscantab.$(OBJ) istack.$(OBJ) iutil.$(OBJ)
INT4=sfile.$(OBJ) sfilter1.$(OBJ) sstring.$(OBJ) stream.$(OBJ)
Z1=zarith.$(OBJ) zarray.$(OBJ) zcontrol.$(OBJ) zdict.$(OBJ)
Z1OPS=zarith zarray zcontrol zdict
Z2=zfile.$(OBJ) zfileio.$(OBJ) zfilter.$(OBJ) zfname.$(OBJ) zfproc.$(OBJ)
Z2OPS=zfile zfileio zfilter zfproc
Z3=zgeneric.$(OBJ) ziodev.$(OBJ) zmath.$(OBJ) zmisc.$(OBJ) zpacked.$(OBJ)
Z3OPS=zgeneric ziodev zmath zmisc zpacked
Z4=zrelbit.$(OBJ) zstack.$(OBJ) zstring.$(OBJ) zsysvm.$(OBJ)
Z4OPS=zrelbit zstack zstring zsysvm
Z5=ztoken.$(OBJ) ztype.$(OBJ) zvmem.$(OBJ)
Z5OPS=ztoken ztype zvmem
Z6=zchar.$(OBJ) zcolor.$(OBJ) zdevice.$(OBJ) zfont.$(OBJ) zfont2.$(OBJ)
Z6OPS=zchar zcolor zdevice zfont zfont2
Z7=zgstate.$(OBJ) zht.$(OBJ) zmatrix.$(OBJ) zpaint.$(OBJ) zpath.$(OBJ) zpath2.$(OBJ)
Z7OPS=zgstate zht zmatrix zpaint zpath zpath2
INT_ALL=gsmain.$(OBJ) gconfig.$(OBJ) \
  $(INT1) $(INT2) $(INT3) $(INT4) \
  $(Z1) $(Z2) $(Z3) $(Z4) $(Z5) $(Z6) $(Z7)
gs.dev: gs.mak
	$(SETMOD) gs gsmain.$(OBJ) gconfig.$(OBJ)
	$(ADDMOD) gs -obj $(INT1)
	$(ADDMOD) gs -obj $(INT2)
	$(ADDMOD) gs -obj $(INT3)
	$(ADDMOD) gs -obj $(INT4)
	$(ADDMOD) gs -obj $(Z1)
	$(ADDMOD) gs -oper $(Z1OPS)
	$(ADDMOD) gs -obj $(Z2)
	$(ADDMOD) gs -oper $(Z2OPS)
	$(ADDMOD) gs -obj $(Z3)
	$(ADDMOD) gs -oper $(Z3OPS)
	$(ADDMOD) gs -obj $(Z4)
	$(ADDMOD) gs -oper $(Z4OPS)
	$(ADDMOD) gs -obj $(Z5)
	$(ADDMOD) gs -oper $(Z5OPS)
	$(ADDMOD) gs -obj $(Z6)
	$(ADDMOD) gs -oper $(Z6OPS)
	$(ADDMOD) gs -obj $(Z7)
	$(ADDMOD) gs -oper $(Z7OPS)
	$(ADDMOD) gs -iodev stdin stdout stderr lineedit statementedit

# -------------------------- Feature definitions -------------------------- #

# ---------------- Standard Level 1 interpreter ---------------- #

level1.dev: gs.mak bcp.dev hsb.dev type1.dev
	$(SETMOD) level1 -include bcp hsb type1

# ---------------- HSB color ---------------- #

hsb_=gshsb.$(OBJ) zhsb.$(OBJ)
hsb.dev: gs.mak $(hsb_)
	$(SETMOD) hsb $(hsb_)
	$(ADDMOD) hsb -oper zhsb

gshsb.$(OBJ): gshsb.c $(GX) \
  $(gscolor_h) $(gshsb_h) $(gxfrac_h)

zhsb.$(OBJ): zhsb.c $(OP) \
  $(gshsb_h) $(igstate_h) $(store_h)

# ---------------- BCP filters ---------------- #

bcp_=sbcp.$(OBJ) zfbcp.$(OBJ)
bcp.dev: gs.mak $(bcp_)
	$(SETMOD) bcp $(bcp_)
	$(ADDMOD) bcp -oper zfbcp

sbcp.$(OBJ): sbcp.c $(AK) $(stdio__h) \
  $(sfilter_h) $(strimpl_h)

zfbcp.$(OBJ): zfbcp.c $(OP) \
  $(gsstruct_h) $(ialloc_h) $(ifilter_h) \
  $(sfilter_h) $(stream_h) $(strimpl_h)

# ---------------- PostScript Type 1 fonts ---------------- #

type1.dev: gs.mak psf1core.dev psf1read.dev
	$(SETMOD) type1 -include psf1core psf1read

# Core

psf1core_=gstype1.$(OBJ) gxhint1.$(OBJ) gxhint2.$(OBJ) gxhint3.$(OBJ)
psf1core.dev: gs.mak $(psf1core_)
	$(SETMOD) psf1core $(psf1core_)

gscrypt1_h=gscrypt1.h
gstype1_h=gstype1.h
gxfont1_h=gxfont1.h
gxop1_h=gxop1.h
gxtype1_h=gxtype1.h $(gscrypt1_h) $(gstype1_h)

gstype1.$(OBJ): gstype1.c $(GXERR) \
  $(gsstruct_h) \
  $(gxarith_h) $(gxcoord_h) $(gxfixed_h) $(gxmatrix_h) $(gxchar_h) $(gxdevmem_h) \
  $(gxfont_h) $(gxfont1_h) $(gxop1_h) $(gxtype1_h) \
  $(gzstate_h) $(gzpath_h)

gxhint1.$(OBJ): gxhint1.c $(GXERR) \
  $(gxarith_h) $(gxfixed_h) $(gxmatrix_h) $(gxdevmem_h) $(gxchar_h) \
  $(gxfont_h) $(gxfont1_h) $(gxtype1_h) \
  $(gzstate_h)

gxhint2.$(OBJ): gxhint2.c $(GXERR) \
  $(gxarith_h) $(gxfixed_h) $(gxmatrix_h) $(gxdevmem_h) $(gxchar_h) \
  $(gxfont_h) $(gxfont1_h) $(gxtype1_h) $(gxop1_h) \
  $(gzstate_h)

gxhint3.$(OBJ): gxhint3.c $(GXERR) \
  $(gxarith_h) $(gxfixed_h) $(gxmatrix_h) $(gxdevmem_h) $(gxchar_h) \
  $(gxfont_h) $(gxfont1_h) $(gxtype1_h) $(gxop1_h) \
  $(gxpath_h) $(gzstate_h)

# Reader

psf1read_=seexec.$(OBJ) zchar1.$(OBJ) zfont1.$(OBJ) zmisc1.$(OBJ)
psf1read.dev: gs.mak $(psf1read_)
	$(SETMOD) psf1read $(psf1read_)
	$(ADDMOD) psf1read -oper zchar1 zfont1 zmisc1
	$(ADDMOD) psf1read -ps gs_type1

comp1_h=comp1.h $(ghost_h) $(oper_h) $(gserrors_h) $(gxfixed_h) $(gxop1_h)

seexec.$(OBJ): seexec.c $(AK) $(stdio__h) \
  $(gscrypt1_h) $(scanchar_h) $(sfilter_h) $(strimpl_h)

zchar1.$(OBJ): zchar1.c $(OP) \
  $(gsstruct_h) $(gxfixed_h) $(gxmatrix_h) \
  $(gschar_h) $(gxdevice_h) $(gxfont_h) $(gxfont1_h) $(gxtype1_h) \
  $(ialloc_h) $(ichar_h) $(idict_h) $(estack_h) $(ifont_h) $(igstate_h) \
  $(store_h)

zfont1.$(OBJ): zfont1.c $(OP) \
  $(gsmatrix_h) $(gxdevice_h) $(gschar_h) $(gxfixed_h) $(gxfont_h) $(gxfont1_h) \
  $(bfont_h) $(ialloc_h) $(idict_h) $(idparam_h) $(store_h)

zmisc1.$(OBJ): zmisc1.c $(OP) \
  $(gscrypt1_h) $(ifilter_h) $(sfilter_h) $(stream_h) $(strimpl_h)

# -------- Level 1 color extensions (CMYK color and colorimage) -------- #

color.dev: gs.mak cmykcore.dev cmykread.dev
	$(SETMOD) color -include cmykcore cmykread

# Core

cmykcore_=gscolor1.$(OBJ) gsht1.$(OBJ)
cmykcore.dev: gs.mak $(cmykcore_)
	$(SETMOD) cmykcore $(cmykcore_)

gscolor1.$(OBJ): gscolor1.c $(GXERR) \
  $(gsccolor_h) $(gscolor1_h) $(gscspace_h) $(gsstruct_h) \
  $(gxcmap_h) $(gxdcconv_h) $(gxdevice_h) \
  $(gzstate_h)

gsht1.$(OBJ): gsht1.c $(GXERR) \
  $(gsstruct_h) $(gxdevice_h) $(gzht_h) $(gzstate_h)

# Reader

cmykread_=zcolor1.$(OBJ) zht1.$(OBJ)
cmykread.dev: gs.mak $(cmykread_)
	$(SETMOD) cmykread $(cmykread_)
	$(ADDMOD) cmykread -oper zcolor1 zht1

zcolor1.$(OBJ): zcolor1.c $(OP) \
  $(gscolor1_h) $(gscspace_h) $(gxfixed_h) $(gxmatrix_h) \
  $(gzstate_h) $(gxdevice_h) $(gxcmap_h) \
  $(ialloc_h) $(icolor_h) $(estack_h) $(iutil_h) $(igstate_h) $(store_h)

zht1.$(OBJ): zht1.c $(OP) \
  $(gsmatrix_h) $(gsstate_h) $(gsstruct_h) $(gxdevice_h) $(gzht_h) \
  $(ialloc_h) $(estack_h) $(igstate_h) $(store_h)

# ---------------- Binary tokens ---------------- #

# Reader only

btoken_=iscanbin.$(OBJ) zbseq.$(OBJ)
btoken.dev: gs.mak $(btoken_)
	$(SETMOD) btoken $(btoken_)
	$(ADDMOD) btoken -oper zbseq_l2
	$(ADDMOD) btoken -ps gs_btokn

bseq_h=bseq.h
btoken_h=btoken.h

iscanbin.$(OBJ): iscanbin.c $(GH) $(errors_h) \
  $(gsutil_h) $(ialloc_h) $(ibnum_h) $(idict_h) $(iname_h) \
  $(iscan_h) $(iutil_h) $(ivmspace_h) \
  $(bseq_h) $(btoken_h) $(dstack_h) $(ostack_h) \
  $(sfilter_h) $(store_h) $(stream_h) $(strimpl_h)

zbseq.$(OBJ): zbseq.c $(OP) \
  $(ialloc_h) $(idict_h) $(isave_h) $(sfilter_h) $(store_h) $(stream_h) $(strimpl_h) \
  $(iname_h) $(ibnum_h) $(btoken_h) $(bseq_h)

# ---------------- User paths & insideness testing ---------------- #

# Reader only

upath_=zupath.$(OBJ) ibnum.$(OBJ)
upath.dev: gs.mak $(upath_)
	$(SETMOD) upath $(upath_)
	$(ADDMOD) upath -oper zupath_l2

zupath.$(OBJ): zupath.c $(OP) \
  $(idict_h) $(dstack_h) $(iutil_h) $(igstate_h) $(store_h) $(stream_h) $(ibnum_h) \
  $(gscoord_h) $(gsmatrix_h) $(gspaint_h) $(gspath_h) $(gsstate_h) \
  $(gxfixed_h) $(gxdevice_h) $(gzpath_h) $(gzstate_h)

# -------- Additions common to Display PostScript and Level 2 -------- #

dpsand2.dev: gs.mak btoken.dev color.dev upath.dev dps2core.dev dps2read.dev
	$(SETMOD) dpsand2 -include btoken color upath dps2core dps2read

# Core

dps2core_=gsdps1.$(OBJ)
dps2core.dev: gs.mak $(dps2core_)
	$(SETMOD) dps2core $(dps2core_)

gsdps1.$(OBJ): gsdps1.c $(GXERR) \
  $(gscoord_h) $(gsmatrix_h) $(gspaint_h) $(gspath_h) $(gspath2_h) \
  $(gxfixed_h) $(gxmatrix_h) $(gzpath_h) $(gzstate_h)

# Reader

dps2read_=ibnum.$(OBJ) zchar2.$(OBJ) zdps1.$(OBJ) zvmem2.$(OBJ)
# Note that zvmem2 includes both Level 1 and Level 2 operators.
dps2read.dev: gs.mak $(dps2read_)
	$(SETMOD) dps2read $(dps2read_)
	$(ADDMOD) dps2read -oper zvmem2
	$(ADDMOD) dps2read -oper igc_l2 zchar2_l2 zdps1_l2
	$(ADDMOD) dps2read -ps gs_dps1

ibnum.$(OBJ): ibnum.c $(GH) $(errors_h) $(stream_h) $(ibnum_h)

zchar2.$(OBJ): zchar2.c $(OP) \
  $(gschar_h) $(gsmatrix_h) $(gsstruct_h) $(gxfixed_h) $(gxfont_h) \
  $(ialloc_h) $(ichar_h) $(estack_h) $(ifont_h) $(iname_h) $(igstate_h) $(store_h) $(stream_h) $(ibnum_h)

zdps1.$(OBJ): zdps1.c $(OP) \
  $(gsmatrix_h) $(gspath_h) $(gspath2_h) $(gsstate_h) \
  $(ialloc_h) $(ivmspace_h) $(igstate_h) $(store_h) $(stream_h) $(ibnum_h)

zvmem2.$(OBJ): zvmem2.c $(OP) \
  $(estack_h) $(ialloc_h) $(ivmspace_h) $(store_h)

# ---------------- Display PostScript ---------------- #

# We should include zcontext, but it isn't in good enough shape yet:
#	$(ADDMOD) dps -oper zcontext_l2
dps_=
dps.dev: gs.mak dpsand2.dev $(dps_)
	$(SETMOD) dps -include dpsand2
	$(ADDMOD) dps -obj $(dps_)

zcontext.$(OBJ): zcontext.c $(OP) \
  $(gsstruct_h) \
  $(idict_h) $(istruct_h) $(dstack_h) $(estack_h) $(igstate_h) $(store_h)

# -------- Composite (PostScript Type 0) font support -------- #

compfont.dev: gs.mak psf0core.dev psf0read.dev
	$(SETMOD) compfont -include psf0core psf0read

# Core

psf0core_=gschar0.$(OBJ) gsfont0.$(OBJ)
psf0core.dev: gs.mak $(psf0core_)
	$(SETMOD) psf0core $(psf0core_)

gschar0.$(OBJ): gschar0.c $(GXERR) \
  $(gsstruct_h) $(gxfixed_h) $(gxdevice_h) $(gxdevmem_h) \
  $(gxfont_h) $(gxfont0_h) $(gxchar_h)

gsfont0.$(OBJ): gsfont0.c $(GXERR) \
  $(gsmatrix_h) $(gsstruct_h) $(gxfixed_h) $(gxdevmem_h) $(gxfcache_h) \
  $(gxfont_h) $(gxfont0_h) $(gxchar_h) $(gxdevice_h)

# Reader

psf0read_=zchar2.$(OBJ) zfont0.$(OBJ)
psf0read.dev: gs.mak $(psf0read_)
	$(SETMOD) psf0read $(psf0read_)
	$(ADDMOD) psf0read -oper zfont0 zchar2
	$(ADDMOD) psf0read -ps gs_type0

zfont0.$(OBJ): zfont0.c $(OP) \
  $(gsmatrix_h) $(gsstruct_h) $(gxdevice_h) $(gxfont_h) $(gxfont0_h) \
  $(ialloc_h) $(bfont_h) $(idict_h) $(igstate_h) $(store_h)

# ---------------- CIE color ---------------- #

ciecore_=gscie.$(OBJ)
cieread_=zcie.$(OBJ)
cie_=$(ciecore_) $(cieread_)
cie.dev: gs.mak $(cie_)
	$(SETMOD) cie $(cie_)
	$(ADDMOD) cie -oper zcie_l2

gscie.$(OBJ): gscie.c $(GXERR) \
  $(gscspace_h) $(gscie_h) $(gscolor2_h) $(gsmatrix_h) $(gsstruct_h) \
  $(gxarith_h) $(gzstate_h)

zcie.$(OBJ): zcie.c $(OP) \
  $(gscspace_h) $(gscolor2_h) $(gscie_h) $(gsstruct_h) \
  $(ialloc_h) $(idict_h) $(idparam_h) $(estack_h) $(isave_h) $(igstate_h) $(ivmspace_h) $(store_h)

# ---------------- Pattern color ---------------- #

patcore_=gspcolor.$(OBJ) gxclip2.$(OBJ) gxpcmap.$(OBJ)
patread_=zpcolor.$(OBJ)
pattern_=$(patcore_) $(patread_)
pattern.dev: gs.mak $(pattern_)
	$(SETMOD) pattern $(pattern_)
	$(ADDMOD) pattern -oper zpcolor_l2

gspcolor.$(OBJ): gspcolor.c $(GXERR) $(math__h) \
  $(gscspace_h) $(gsstruct_h) $(gsutil_h) \
  $(gxarith_h) $(gxcolor2_h) $(gxcoord_h) $(gxclip2_h) $(gxdevice_h) $(gxdevmem_h) \
  $(gxfixed_h) $(gxmatrix_h) $(gxpath_h) $(gxpcolor_h) $(gzstate_h)

gxclip2.$(OBJ): gxclip2.c $(GXERR) $(memory__h) \
  $(gsstruct_h) $(gxclip2_h) $(gxdevice_h) $(gxdevmem_h)

gxpcmap.$(OBJ): gxpcmap.c $(GXERR) \
  $(gsstruct_h) $(gscspace_h) $(gsutil_h) \
  $(gxcolor2_h) $(gxdevice_h) $(gxdevmem_h) $(gxfixed_h) $(gxmatrix_h) $(gxpcolor_h) \
  $(gzstate_h)

zpcolor.$(OBJ): zpcolor.c $(OP) \
  $(gscolor_h) $(gscspace_h) $(gsmatrix_h) $(gsstruct_h) \
  $(gxcolor2_h) $(gxdevice_h) $(gxdevmem_h) $(gxfixed_h) $(gxpcolor_h) \
  $(estack_h) $(ialloc_h) $(idict_h) $(idparam_h) $(igstate_h) $(istruct_h) \
  $(store_h)

# ---------------- Separation color ---------------- #

seprcore_=gscsepr.$(OBJ)
seprread_=zcssepr.$(OBJ)
sepr_=$(seprcore_) $(seprread_)
sepr.dev: gs.mak $(sepr_)
	$(SETMOD) sepr $(sepr_)
	$(ADDMOD) sepr -oper zcssepr_l2

gscsepr.$(OBJ): gscsepr.c $(GXERR) \
  $(gscspace_h) $(gsmatrix_h) $(gsrefct_h) $(gxcolor2_h) $(gxfixed_h)

zcssepr.$(OBJ): zcssepr.c $(OP) \
  $(gscolor_h) $(gsstruct_h) $(gxfixed_h) $(gxcolor2_h) $(gscspace_h) \
  $(ialloc_h) $(icsmap_h) $(estack_h) $(igstate_h) $(ivmspace_h) $(store_h)

# ---------------- Device control ---------------- #

devctrl_=zdevice2.$(OBJ) ziodev2.$(OBJ)
devctrl.dev: gs.mak $(devctrl_)
	$(SETMOD) devctrl $(devctrl_)
	$(ADDMOD) devctrl -oper zdevice2_l2 ziodev2_l2
	$(ADDMOD) devctrl -iodev null ram
	$(ADDMOD) devctrl -ps gs_setpd

zdevice2.$(OBJ): zdevice2.c $(OP) \
  $(dstack_h) $(estack_h) $(idict_h) $(idparam_h) $(igstate_h) $(iname_h) $(store_h) \
  $(gxdevice_h) $(gsstate_h)

ziodev2.$(OBJ): ziodev2.c $(OP) \
  $(gxiodev_h) $(stream_h) $(files_h) $(iparam_h) $(iutil2_h) $(store_h)

# ---------------- PostScript Level 2 ---------------- #

###### Include files

iutil2_h=iutil2.h

level2.dev: gs.mak cie.dev compfont.dev dct.dev devctrl.dev dpsand2.dev filter.dev level1.dev pattern.dev psl2core.dev psl2read.dev sepr.dev
	$(SETMOD) level2 -include cie compfont dct devctrl dpsand2 filter
	$(ADDMOD) level2 -include level1 pattern psl2core psl2read sepr

# Core

psl2core_=gscolor2.$(OBJ)
psl2core.dev: gs.mak $(psl2core_)
	$(SETMOD) psl2core $(psl2core_)

gscolor2.$(OBJ): gscolor2.c $(GXERR) \
  $(gscspace_h) \
  $(gxarith_h) $(gxcolor2_h) $(gxfixed_h) $(gxmatrix_h) \
  $(gzstate_h)

# Reader

psl2read1_=iutil2.$(OBJ) zcolor2.$(OBJ) zcsindex.$(OBJ)
psl2read2_=zht2.$(OBJ) zimage2.$(OBJ) zmisc2.$(OBJ)
psl2read_=$(psl2read1_) $(psl2read2_)
# Note that zmisc2 includes both Level 1 and Level 2 operators.
psl2read.dev: gs.mak $(psl2read_)
	$(SETMOD) psl2read $(psl2read1_)
	$(ADDMOD) psl2read -obj $(psl2read2_)
	$(ADDMOD) psl2read -oper zmisc2
	$(ADDMOD) psl2read -oper zcolor2_l2 zcsindex_l2
	$(ADDMOD) psl2read -oper zht2_l2 zimage2_l2
	$(ADDMOD) psl2read -ps gs_lev2 gs_res

iutil2.$(OBJ): iutil2.c $(GXERR) $(memory__h) \
  $(gsparam_h) $(opcheck_h) $(idict_h) $(imemory_h) $(iutil_h) $(iutil2_h)

zcolor2.$(OBJ): zcolor2.c $(OP) \
  $(gscolor_h) $(gscspace_h) $(gsmatrix_h) $(gsstruct_h) \
  $(gxcolor2_h) $(gxdevice_h) $(gxdevmem_h) $(gxfixed_h) $(gxpcolor_h) \
  $(estack_h) $(ialloc_h) $(idict_h) $(idparam_h) $(igstate_h) $(istruct_h) \
  $(store_h)

zcsindex.$(OBJ): zcsindex.c $(OP) \
  $(gscolor_h) $(gsstruct_h) $(gxfixed_h) $(gxcolor2_h) $(gscspace_h) \
  $(ialloc_h) $(icsmap_h) $(estack_h) $(igstate_h) $(ivmspace_h) $(store_h)

zht2.$(OBJ): zht2.c $(OP) \
  $(gsstruct_h) $(gxdevice_h) $(gzht_h) \
  $(estack_h) $(ialloc_h) $(icolor_h) $(idict_h) $(idparam_h) $(igstate_h) \
  $(store_h)

zimage2.$(OBJ): zimage2.c $(OP) \
  $(gscolor_h) $(gscolor2_h) $(gscspace_h) $(gsmatrix_h) \
  $(idict_h) $(idparam_h) $(ilevel_h) $(igstate_h)

zmisc2.$(OBJ): zmisc2.c $(OP) \
  $(gscdefs_h) $(gsfont_h) $(gsstruct_h) $(gsutil_h) $(gxht_h) \
  $(ialloc_h) $(idict_h) $(idparam_h) $(iparam_h) $(dstack_h) $(estack_h) \
  $(ilevel_h) $(iname_h) $(iutil2_h) $(ivmspace_h) $(store_h)

# ---------------- Filters other than the ones in sfilter.c ---------------- #

# Standard Level 2 decoding filters only.  The PDF configuration uses this.
scfd_=scfd.$(OBJ) scfdtab.$(OBJ) scftab.$(OBJ) sbits.$(OBJ)
fdecode_=$(scfd_) slzwd.$(OBJ) sfilter2.$(OBJ) zfdecode.$(OBJ)
fdecode.dev: gs.mak $(fdecode_)
	$(SETMOD) fdecode $(fdecode_)
	$(ADDMOD) fdecode -oper zfdecode

# Full Level 2 filter capability, plus extensions.
scfe_=scfe.$(OBJ) scfetab.$(OBJ) scftab.$(OBJ) shc.$(OBJ) sbits.$(OBJ)
#slzwe_=slzwce.$(OBJ)
slzwe_=slzwe.$(OBJ)
xfilter_=sbhc.$(OBJ) sbwbs.$(OBJ) shcgen.$(OBJ)
filter_=$(scfe_) $(slzwe_) $(xfilter_) zfilter2.$(OBJ)
filter.dev: gs.mak fdecode.dev $(filter_)
	$(SETMOD) filter -include fdecode
	$(ADDMOD) filter -obj zfilter2.$(OBJ)
	$(ADDMOD) filter -obj $(scfe_) $(slzwe_)
	$(ADDMOD) filter -obj $(xfilter_)
	$(ADDMOD) filter -oper zfilter2

# Support

sbits.$(OBJ): sbits.c $(AK) $(std_h)

shc.$(OBJ): shc.c $(AK) $(std_h) $(shc_h)

# Filters

sbhc.$(OBJ): sbhc.c $(AK) $(stdio__h) \
  $(gdebug_h) $(sbhc_h) $(shcgen_h) $(strimpl_h)

sbwbs.$(OBJ): sbwbs.c $(AK) $(stdio__h) $(memory__h) \
  $(gdebug_h) $(sbwbs_h) $(sfilter_h) $(strimpl_h)

scfd.$(OBJ): scfd.c $(AK) $(stdio__h) $(gdebug_h)\
  $(scf_h) $(strimpl_h) $(scfx_h)

scfe.$(OBJ): scfe.c $(AK) $(stdio__h) $(gdebug_h)\
  $(scf_h) $(strimpl_h) $(scfx_h)

scfdtab.$(OBJ): scfdtab.c $(AK) $(std_h) $(scommon_h) $(scf_h)

scfetab.$(OBJ): scfetab.c $(AK) $(std_h) $(scommon_h) $(scf_h)

scftab.$(OBJ): scftab.c $(AK) $(std_h)

sfilter2.$(OBJ): sfilter2.c $(AK) $(stdio__h)\
  $(scanchar_h) $(sfilter_h) $(strimpl_h)

shcgen.$(OBJ): shcgen.c $(AK) $(stdio__h) \
  $(gdebug_h) $(gserror_h) $(gserrors_h) \
  $(scommon_h) $(shc_h) $(shcgen_h)

slzwce.$(OBJ): slzwce.c $(AK) $(stdio__h) $(gdebug_h)\
  $(slzwx_h) $(strimpl_h)

slzwd.$(OBJ): slzwd.c $(AK) $(stdio__h) $(gdebug_h)\
  $(slzwx_h) $(strimpl_h)

slzwe.$(OBJ): slzwe.c $(AK) $(stdio__h) $(gdebug_h)\
  $(slzwx_h) $(strimpl_h)

zfilter2.$(OBJ): zfilter2.c $(OP) \
  $(gsstruct_h) $(ialloc_h) $(idict_h) $(idparam_h) $(ifilter_h) $(store_h) \
  $(sfilter_h) $(sbhc_h) $(sbwbs_h) $(scfx_h) $(shcgen_h) $(slzwx_h) $(strimpl_h)

# ---------------- Pixel-difference filters ---------------- #

# These are designed for PDF, but may be useful in other contexts.

spdiff_=spdiff.$(OBJ) zfpdiff.$(OBJ)
spdiff.dev: gs.mak $(spdiff_)
	$(SETMOD) spdiff $(spdiff_)
	$(ADDMOD) spdiff -oper zfpdiff

spdiffx_h=spdiffx.h

spdiff.$(OBJ): spdiff.c $(AK) $(stdio__h)\
  $(spdiffx_h) $(strimpl_h)

zfpdiff.$(OBJ): zfpdiff.c $(OP) \
  $(gsstruct_h) $(idict_h) $(idparam_h) $(ifilter_h) \
  $(sfilter_h) $(spdiffx_h) $(strimpl_h)

# ---------------- PDF interpreter ---------------- #

# We need most of the Level 2 interpreter to do PDF, but not all of it.

# Because of the way the PDF encodings are defined, they must get loaded
# before we install the Level 2 resource machinery.
# On the other hand, the PDF .ps files must get loaded after
# level2dict is defined.
pdf.dev: gs.mak color.dev dctd.dev dps2core.dev dps2read.dev fdecode.dev level1.dev pdffonts.dev psl2core.dev psl2read.dev pdfread.dev
	$(SETMOD) pdf -include color dctd dps2core dps2read fdecode
	$(ADDMOD) pdf -include level1 pdffonts psl2core psl2read pdfread

# Reader only

pdffonts.dev: gs_pdf_e.ps
	$(SETMOD) pdffonts -ps gs_mex_e gs_mro_e gs_pdf_e gs_wan_e

# pdf_2ps must be the last .ps file loaded.
pdfread.dev: gs.mak spdiff.dev
	$(SETMOD) pdfread -include spdiff
	$(ADDMOD) pdfread -ps gs_pdf pdf_base pdf_draw pdf_font pdf_main pdf_2ps

# ---------------- DCT filters ---------------- #
# The definitions for jpeg*.dev are in jpeg.mak.

dct.dev: gs.mak dcte.dev dctd.dev
	$(SETMOD) dct -include dcte dctd

# Common code

dctc_=sdctc.$(OBJ) sjpegc.$(OBJ) zfdctc.$(OBJ)

sdctc.$(OBJ): sdctc.c $(AK) $(stdio__h) \
  $(sdct_h) $(strimpl_h) \
  jerror.h jpeglib.h

sjpegc.$(OBJ): sjpegc.c $(AK) $(stdio__h) $(gx_h)\
  $(gserrors_h) $(sjpeg_h) $(sdct_h) $(strimpl_h) \
  jerror.h jpeglib.h

zfdctc.$(OBJ): zfdctc.c $(GH) $(errors_h) $(opcheck_h) \
  $(idict_h) $(idparam_h) $(imemory_h) \
  $(ipacked_h) $(sdct_h) $(sjpeg_h) $(strimpl_h) \
  jpeglib.h

# Encoding (compression)

dcte_=$(dctc_) sdcte.$(OBJ) sjpege.$(OBJ) zfdcte.$(OBJ)
dcte.dev: gs.mak jpege.dev $(dcte_)
	$(SETMOD) dcte -include jpege
	$(ADDMOD) dcte -obj $(dcte_)
	$(ADDMOD) dcte -oper zfdcte

sdcte.$(OBJ): sdcte.c $(AK) $(memory__h) $(stdio__h) $(gdebug_h)\
  $(sdct_h) $(sjpeg_h) $(strimpl_h) \
  jerror.h jpeglib.h

sjpege.$(OBJ): sjpege.c $(AK) $(stdio__h) $(gx_h)\
  $(gserrors_h) $(sjpeg_h) $(sdct_h) $(strimpl_h) \
  jerror.h jpeglib.h

zfdcte.$(OBJ): zfdcte.c $(OP) \
  $(idict_h) $(idparam_h) $(sdct_h) $(sjpeg_h) $(strimpl_h) \
  jpeglib.h

# Decoding (decompression)

dctd_=$(dctc_) sdctd.$(OBJ) sjpegd.$(OBJ) zfdctd.$(OBJ)
dctd.dev: gs.mak jpegd.dev $(dctd_)
	$(SETMOD) dctd -include jpegd
	$(ADDMOD) dctd -obj $(dctd_)
	$(ADDMOD) dctd -oper zfdctd

sdctd.$(OBJ): sdctd.c $(AK) $(memory__h) $(stdio__h) $(gdebug_h)\
  $(sdct_h) $(sjpeg_h) $(strimpl_h) \
  jerror.h jpeglib.h

sjpegd.$(OBJ): sjpegd.c $(AK) $(stdio__h) $(gx_h)\
  $(gserrors_h) $(sjpeg_h) $(sdct_h) $(strimpl_h) \
  jerror.h jpeglib.h

zfdctd.$(OBJ): zfdctd.c $(OP) \
  $(sdct_h) $(sjpeg_h) $(strimpl_h) \
  jpeglib.h

# ---------------- Precompiled fonts ---------------- #
# See fonts.doc for more information.

ccfont_h=ccfont.h $(std_h) $(gsmemory_h) $(iref_h) $(store_h)

CCFONT=$(OP) $(ccfont_h)

# List the fonts we are going to compile.
# Because of intrinsic limitations in `make', we have to list
# the object file names and the font names separately.
# Because of limitations in the DOS shell, we have to break the fonts up
# into lists that will fit on a single line (120 characters).
# The rules for constructing the .c files from the fonts themselves,
# and for compiling the .c files, are in cfonts.mak, not here.
# For example, to compile the Helvetica fonts, you should invoke
#	make -f cfonts.mak Helvetica_o
ccfonts_ps=gs_ccfnt
#ccfonts1_=pagk.$(OBJ) pagko.$(OBJ) pagd.$(OBJ) pagdo.$(OBJ)
#ccfonts1=agk agko agd agdo
#ccfonts2_=pbkl.$(OBJ) pbkli.$(OBJ) pbkd.$(OBJ) pbkdi.$(OBJ)
#ccfonts2=bkl bkli bkd bkdi
#ccfonts3_=bchr.$(OBJ) bchri.$(OBJ) bchb.$(OBJ) bchbi.$(OBJ)
#ccfonts3=chr chri chb chbi
#ccfonts4_=ncrr.$(OBJ) ncri.$(OBJ) ncrb.$(OBJ) ncrbi.$(OBJ)
#ccfonts4=crr cri crb crbi
#ccfonts5_=phvr.$(OBJ) phvro.$(OBJ) phvb.$(OBJ) phvbo.$(OBJ) phvrrn.$(OBJ)
#ccfonts5=hvr hvro hvb hvbo hvrrn
#ccfonts6_=pncr.$(OBJ) pncri.$(OBJ) pncb.$(OBJ) pncbi.$(OBJ)
#ccfonts6=ncr ncri ncb ncbi
#ccfonts7_=pplr.$(OBJ) pplri.$(OBJ) pplb.$(OBJ) pplbi.$(OBJ)
#ccfonts7=plr plri plb plbi
#ccfonts8_=psyr.$(OBJ) ptmr.$(OBJ) ptmri.$(OBJ) ptmb.$(OBJ) ptmbi.$(OBJ)
#ccfonts8=syr tmr tmri tmb tmbi
#ccfonts9_=zcr.$(OBJ) zcro.$(OBJ) zcb.$(OBJ) pzdr.$(OBJ)
#ccfonts9=zcr zcro zcb zdr
# Add your own fonts here if desired.
ccfonts10_=
ccfonts10=
ccfonts11_=
ccfonts11=
ccfonts12_=
ccfonts12=
ccfonts13_=
ccfonts13=
ccfonts14_=
ccfonts14=
ccfonts15_=
ccfonts15=

# Strictly speaking, ccfonts shouldn't need to include type1,
# since one could choose to precompile only Type 0 fonts,
# but getting this exactly right would be too much work.
ccfonts.dev: $(MAKEFILE) gs.mak type1.dev iccfont.$(OBJ) \
  $(ccfonts1_) $(ccfonts2_) $(ccfonts3_) $(ccfonts4_) $(ccfonts5_) \
  $(ccfonts6_) $(ccfonts7_) $(ccfonts8_) $(ccfonts9_)
	$(SETMOD) ccfonts -include type1
	$(ADDMOD) ccfonts -obj iccfont.$(OBJ)
	$(ADDMOD) ccfonts -obj $(ccfonts1_)
	$(ADDMOD) ccfonts -obj $(ccfonts2_)
	$(ADDMOD) ccfonts -obj $(ccfonts3_)
	$(ADDMOD) ccfonts -obj $(ccfonts4_)
	$(ADDMOD) ccfonts -obj $(ccfonts5_)
	$(ADDMOD) ccfonts -obj $(ccfonts6_)
	$(ADDMOD) ccfonts -obj $(ccfonts7_)
	$(ADDMOD) ccfonts -obj $(ccfonts8_)
	$(ADDMOD) ccfonts -obj $(ccfonts9_)
	$(ADDMOD) ccfonts -obj $(ccfonts10_)
	$(ADDMOD) ccfonts -obj $(ccfonts11_)
	$(ADDMOD) ccfonts -obj $(ccfonts12_)
	$(ADDMOD) ccfonts -obj $(ccfonts13_)
	$(ADDMOD) ccfonts -obj $(ccfonts14_)
	$(ADDMOD) ccfonts -obj $(ccfonts15_)
	$(ADDMOD) ccfonts -oper ccfonts
	$(ADDMOD) ccfonts -ps $(ccfonts_ps)

gconfigf.h: $(MAKEFILE) gs.mak genconf$(XE)
	$(SETMOD) ccfonts_ -font $(ccfonts1)
	$(ADDMOD) ccfonts_ -font $(ccfonts2)
	$(ADDMOD) ccfonts_ -font $(ccfonts3)
	$(ADDMOD) ccfonts_ -font $(ccfonts4)
	$(ADDMOD) ccfonts_ -font $(ccfonts5)
	$(ADDMOD) ccfonts_ -font $(ccfonts6)
	$(ADDMOD) ccfonts_ -font $(ccfonts7)
	$(ADDMOD) ccfonts_ -font $(ccfonts8)
	$(ADDMOD) ccfonts_ -font $(ccfonts9)
	$(ADDMOD) ccfonts_ -font $(ccfonts10)
	$(ADDMOD) ccfonts_ -font $(ccfonts11)
	$(ADDMOD) ccfonts_ -font $(ccfonts12)
	$(ADDMOD) ccfonts_ -font $(ccfonts13)
	$(ADDMOD) ccfonts_ -font $(ccfonts14)
	$(ADDMOD) ccfonts_ -font $(ccfonts15)
	$(EXP)genconf ccfonts_.dev -f gconfigf.h

iccfont.$(OBJ): iccfont.c $(GH) gconfigf.h \
  $(ghost_h) $(gsstruct_h) $(ccfont_h) $(errors_h) \
  $(ialloc_h) $(idict_h) $(ifont_h) $(iname_h) $(isave_h) $(iutil_h) \
  $(oper_h) $(ostack_h) $(store_h) $(stream_h) $(strimpl_h) $(sfilter_h) $(iscan_h)

# ---------------- Precompiled initialization code ---------------- #
# Note that in order to generate this, you have to have a working copy
# of the interpreter already available.

ccinit.dev: gs.mak iccinit.$(OBJ) gs_init.$(OBJ)
	$(SETMOD) ccinit iccinit.$(OBJ) gs_init.$(OBJ)
	$(ADDMOD) ccinit -oper ccinit

iccinit.$(OBJ): iccinit.c $(GH) \
  $(errors_h) $(oper_h) $(store_h) $(stream_h) $(files_h)

# All the gs_*.ps files should be prerequisites of gs_init.c,
# but we don't have any convenient list of them.
# We can't make $(GS)$(XE) a prerequisite, because that would be circular.
gs_init.c: $(GS_INIT) mergeini.ps
	$(EXP)$(GS)$(XE) -dNODISPLAY -sCfile=gs_init.c mergeini.ps

gs_init.$(OBJ): gs_init.c $(stdpre_h)
	$(CCCF) gs_init.c

# --------------------- Configuration-dependent files --------------------- #

# gconfig.h shouldn't have to depend on DEVS_ALL, but that would
# involve rewriting gsconfig to only save the device name, not the
# contents of the <device>.dev files.
# We have to put gslib.dev last in the list because of problems
# with the Unix linker when gslib is really a library.

DEVS_ALL=gs.dev $(FEATURE_DEVS) $(PLATFORM).dev \
  $(DEVICE_DEVS) $(DEVICE_DEVS1) \
  $(DEVICE_DEVS2) $(DEVICE_DEVS3) $(DEVICE_DEVS4) $(DEVICE_DEVS5) \
  $(DEVICE_DEVS6) $(DEVICE_DEVS7) $(DEVICE_DEVS8) $(DEVICE_DEVS9) \
  gslib.dev

devs.tr: devs.mak $(MAKEFILE) echogs$(XE)
	$(EXP)echogs -w devs.tr - gs.dev $(PLATFORM).dev
	$(EXP)echogs -a devs.tr - $(FEATURE_DEVS)
	$(EXP)echogs -a devs.tr - $(DEVICE_DEVS)
	$(EXP)echogs -a devs.tr - $(DEVICE_DEVS1)
	$(EXP)echogs -a devs.tr - $(DEVICE_DEVS2)
	$(EXP)echogs -a devs.tr - $(DEVICE_DEVS3)
	$(EXP)echogs -a devs.tr - $(DEVICE_DEVS4)
	$(EXP)echogs -a devs.tr - $(DEVICE_DEVS5)
	$(EXP)echogs -a devs.tr - $(DEVICE_DEVS6)
	$(EXP)echogs -a devs.tr - $(DEVICE_DEVS7)
	$(EXP)echogs -a devs.tr - $(DEVICE_DEVS8)
	$(EXP)echogs -a devs.tr - $(DEVICE_DEVS9)
	$(EXP)echogs -a devs.tr - gslib.dev

# GCONFIG_EXTRAS can be set on the command line.
# Note that it consists of arguments for echogs, i.e.,
# it isn't just literal text.

gconfig.h obj.tr objw.tr ld.tr lib.tr: \
  $(MAKEFILE) genconf$(XE) echogs$(XE) devs.tr $(DEVS_ALL)
	$(EXP)genconf @devs.tr -h gconfig.h $(CONFILES)
	$(EXP)echogs -a gconfig.h -x 23 define GS_LIB_DEFAULT -x 2022 $(GS_LIB_DEFAULT) -x 22
	$(EXP)echogs -a gconfig.h -x 23 define GS_DOCDIR -x 2022 $(GS_DOCDIR) -x 22
	$(EXP)echogs -a gconfig.h -x 23 define GS_INIT -x 2022 $(GS_INIT) -x 22
	$(EXP)echogs -a gconfig.h $(GCONFIG_EXTRAS)

gconfigv.h: gs.mak $(MAKEFILE) echogs$(XE)
	$(EXP)echogs -w gconfigv.h -x 23 define USE_ASM -x 2028 -q $(USE_ASM)-0 -x 29
	$(EXP)echogs -a gconfigv.h -x 23 define USE_FPU -x 2028 -q $(FPU_TYPE)-0 -x 29

gconfig.$(OBJ): gconfig.c $(GX) \
  $(gscdefs_h) $(gconfig_h) $(gxdevice_h) $(gxiodev_h) $(MAKEFILE)

# ----------------------- Platform-specific modules ----------------------- #
# Platform-specific code doesn't really belong here: this is code that is
# shared among multiple platforms.

# Frame buffer implementations.

gp_nofb.$(OBJ): gp_nofb.c $(AK) \
  $(gx_h) $(gp_h) $(gxdevice_h)

gp_dosfb.$(OBJ): gp_dosfb.c $(AK) $(memory__h) \
  $(gx_h) $(gp_h) $(gserrors_h) $(gxdevice_h)

# MS-DOS file system, also used by Desqview/X.
gp_dosfs.$(OBJ): gp_dosfs.c $(AK) $(dos__h) $(gp_h) $(gx_h)

# MS-DOS file enumeration, *not* used by Desqview/X.
gp_dosfe.$(OBJ): gp_dosfe.c $(AK) $(stdio__h) $(memory__h) $(string__h) \
  $(dos__h) $(gstypes_h) $(gsmemory_h) $(gsstruct_h) $(gp_h) $(gsutil_h)

# Other MS-DOS facilities.
gp_msdos.$(OBJ): gp_msdos.c $(AK) $(dos__h) \
  $(gsmemory_h) $(gstypes_h) $(gp_h)

# Unix(-like) file system, also used by Desqview/X.
gp_unifs.$(OBJ): gp_unifs.c $(AK) $(memory__h) $(string__h) $(gx_h) $(gp_h) \
  $(gsstruct_h) $(gsutil_h) $(stat__h) $(dirent__h)

# Unix(-like) file name syntax, *not* used by Desqview/X.
gp_unifn.$(OBJ): gp_unifn.c $(AK) $(gx_h) $(gp_h)

# ----------------------------- Main program ------------------------------ #

# Interpreter main program

gs.$(OBJ): gs.c $(GH) $(ctype__h) \
  $(gscdefs_h) $(gsdevice_h) $(gxdevice_h) $(gxdevmem_h) \
  $(errors_h) $(estack_h) $(files_h) \
  $(ialloc_h) $(interp_h) $(iscan_h) $(iutil_h) $(ivmspace_h) \
  $(main_h) $(ostack_h) $(sfilter_h) $(store_h) $(stream_h) $(strimpl_h)

gsmain.$(OBJ): gsmain.c $(GH) \
  $(gp_h) $(gsmatrix_h) $(gxdevice_h) $(gserrors_h) \
  $(dstack_h) $(estack_h) $(files_h) $(ialloc_h) $(idict_h) $(iname_h) $(iscan_h) $(ivmspace_h) \
  $(main_h) $(oper_h) $(ostack_h) $(sfilter_h) $(store_h) $(strimpl_h)

interp.$(OBJ): interp.c $(GH) \
  $(dstack_h) $(errors_h) $(estack_h) $(files_h) $(oper_h) $(ostack_h) \
  $(ialloc_h) $(iastruct_h) $(iname_h) $(idict_h) $(interp_h) $(ipacked_h) \
  $(iscan_h) $(isave_h) $(istack_h) $(iutil_h) $(ivmspace_h) $(ostack_h) \
  $(sfilter_h) $(store_h) $(stream_h) $(strimpl_h)
	$(CCINT) interp.c
