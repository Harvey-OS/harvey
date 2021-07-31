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

# makefile for DesqView/X/gcc/X11 configuration.
# Note: this makefile assumes you are using gcc in ANSI mode.

# ------------------------------- Options ------------------------------- #

####### The following are the only parts of the file you should need to edit.

# ------ Generic options ------ #

# Define the installation commands and target directories for
# executables and files.  Only relevant to `make install'.

INSTALL = install -c
INSTALL_PROGRAM = $(INSTALL) -m 775
INSTALL_DATA = $(INSTALL) -m 664

prefix = c:/bin
bindir = c:/bin
gsdatadir = c:/gs
gsfontdir = c:/gsfonts

docdir=$(gsdatadir)/doc
exdir=$(gsdatadir)/examples
GS_DOCDIR=$(docdir)

# Define the default directory/ies for the runtime
# initialization and font files.  Separate multiple directories with a ;.

GS_LIB_DEFAULT=".;$(gsdatadir);$(gsfontdir)"

# Define the name of the interpreter initialization file.
# (There is no reason to change this.)

GS_INIT=gs_init.ps

# Choose generic configuration options.

# -DDEBUG
#	includes debugging features (-Z switch) in the code.
#	  Code runs substantially slower even if no debugging switches
#	  are set.
# -DNOPRIVATE
#	makes private (static) procedures and variables public,
#	  so they are visible to the debugger and profiler.
#	  No execution time or space penalty.

GENOPT=

# Define the name of the executable file.

GS=gs

# Define the directory where the IJG JPEG library sources are stored.
# You may have to change this if the IJG library version changes.
# See jpeg.mak for more information.

JSRCDIR=jpeg-5

# ------ Platform-specific options ------ #

# Define the name of the C compiler.

CC=gcc

# Define the other compilation flags.
# Add -DBSD4_2 for 4.2bsd systems.
# Add -DSYSV for System V or DG/UX.
# Add -DSYSV -D__SVR3 for SCO ODT, ISC Unix 2.2 or before,
#   or any System III Unix, or System V release 3-or-older Unix.
# Add -DSVR4 (not -DSYSV) for System V release 4.
# XCFLAGS can be set from the command line.
# We don't include -ansi, because this gets in the way of the platform-
#   specific stuff that <math.h> typically needs; nevertheless, we expect
#   gcc to accept ANSI-style function prototypes and function definitions.

# Under DJGPP, since we strip the executable by default, we may as
# well *not* use '-g'

# CFLAGS=-g -O $(XCFLAGS)
CFLAGS=-O $(XCFLAGS)

# Define platform flags for ld.
# Ultrix wants -x.
# SunOS 4.n may need -Bstatic.
# XLDFLAGS can be set from the command line.

LDFLAGS=$(XLDFLAGS)

# Define any extra libraries to link into the executable.
# ISC Unix 2.2 wants -linet.
# SCO Unix needs -lsocket if you aren't including the X11 driver.
# (Libraries required by individual drivers are handled automatically.)

EXTRALIBS=-lsys -lc

# Define the include switch(es) for the X11 header files.
# This can be null if handled in some other way (e.g., the files are
# in /usr/include, or the directory is supplied by an environment variable);
# in particular, SCO Xenix, Unix, and ODT just want
#XINCLUDE=
# Note that x_.h expects to find the header files in $(XINCLUDE)/X11,
# not in $(XINCLUDE).

XINCLUDE=

# Define the directory/ies and library names for the X11 library files.
# The former can be null if these files are in the default linker search path.
# Unfortunately, Quarterdeck's old libraries did not conform to the
# X11 conventions for naming, in that the main Xlib library was called
# libx.a, not libx11.a.  To make things worse, both are provided in
# the v2.00 library.  Creation dates indicate that 'libx.a' is left
# over from a previous build (or this could just be on my system, but
# others who have upgraded from the early version will have the same
# problem---SJT).  Thus I will make the default to look for
# 'libx11.a', since v1.0x does *not* have it and the linker will
# complain.  With the reverse default, the linker will find to the
# obsolete library on some systems.

XLIBDIRS=
# reverse the comments if you have QDDVX10x or the prerelease version
# of QDLIB200 (still available on some Simtel mirrors, unfortunately)
# XLIBS=Xt Xext X
XLIBS=Xt Xext X11

# Define whether this platform has floating point hardware:
#	FPU_TYPE=2 means floating point is faster than fixed point.
# (This is the case on some RISCs with multiple instruction dispatch.)
#	FPU_TYPE=1 means floating point is at worst only slightly slower
# than fixed point.
#	FPU_TYPE=0 means that floating point may be considerably slower.
#	FPU_TYPE=-1 means that floating point is always much slower than
# fixed point.

FPU_TYPE=1

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.

FEATURE_DEVS=level2.dev

# Choose the device(s) to include.  See devs.mak for details.

DEVICE_DEVS=x11.dev
DEVICE_DEVS3=deskjet.dev djet500.dev laserjet.dev ljetplus.dev ljet2p.dev ljet3.dev ljet4.dev
DEVICE_DEVS4=cdeskjet.dev cdjcolor.dev cdjmono.dev cdj550.dev pj.dev pjxl.dev pjxl300.dev
DEVICE_DEVS5=paintjet.dev pjetxl.dev
DEVICE_DEVS7=gif8.dev tiffg3.dev tiffg4.dev
DEVICE_DEVS9=pbm.dev pbmraw.dev pgm.dev pgmraw.dev ppm.dev ppmraw.dev bit.dev bitrgb.dev bitcmyk.dev

# ---------------------------- End of options --------------------------- #

# Define the name of the makefile -- used in dependencies.

MAKEFILE=dvx-gcc.mak

# Define the ANSI-to-K&R dependency.  (gcc accepts ANSI syntax.)

AK=

# Define the compilation rules and flags.

CCC=$(CC) $(CCFLAGS) -c

# --------------------------- Generic makefile ---------------------------- #

# The remainder of the makefile (unixhead.mak, gs.mak, devs.mak, unixtail.mak)
# is generic.  tar_cat concatenates all these together.
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

# Partial makefile, common to all Desqview/X configurations.

# This part of the makefile gets inserted after the compiler-specific part
# (xxx-head.mak) and before gs.mak and devs.mak.

# ----------------------------- Generic stuff ----------------------------- #

# Define the platform name.

PLATFORM=dvx_

# Define the syntax for command, object, and executable files.

CMD=.bat
O=-o ./
OBJ=o
XE=.exe

# Define the current directory prefix and shell invocations.

D=\\
EXP=
SHELL=
SH=
SHP=

# Define the arguments for genconf.

CONFILES=-p -pl &-l%%s -ol ld.tr

# Define the compilation rules and flags.

CCFLAGS=$(GENOPT) $(CFLAGS)

.c.o: $(AK)
	$(CCC) $*.c

CCCF=$(CCC)
CCD=$(CCC)
CCINT=$(CCC)
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
#    Copyright (C) 1989, 1992, 1993, 1994, 1995 Aladdin Enterprises.  All rights reserved.
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

# makefile for device drivers.

# -------------------------------- Catalog ------------------------------- #

# It is possible to build configurations with an arbitrary collection of
# device drivers, although some drivers are supported only on a subset
# of the target platforms.  The currently available drivers are:

# MS-DOS displays (note: not usable with Desqview/X):
#   MS-DOS EGA and VGA:
#	ega	EGA (640x350, 16-color)
#	vga	VGA (640x480, 16-color)
#   MS-DOS SuperVGA:
# +	atiw	ATI Wonder SuperVGA, 256-color modes
# +	s3vga	SuperVGA with S3 86C911 chip (e.g., Diamond Stealth board)
#	svga16	Generic SuperVGA in 800x600, 16-color mode
#		  (replaces atiw16, tseng16, and tvga16 -- see below)
#	tseng	SuperVGA using Tseng Labs ET3000/4000 chips, 256-color modes
#	tseng8	Tseng Labs SuperVGA using only 8 colors (for halftone testing)
# +	tvga	Trident SuperVGA, 256-color modes
#   ****** NOTE: The vesa device does not work with the Watcom (32-bit MS-DOS)
#   ****** compiler or executable.
#	vesa	SuperVGA with VESA standard API driver
#   MS-DOS other:
#	bgi	Borland Graphics Interface (CGA)  [MS-DOS only]
# *	herc	Hercules Graphics display   [MS-DOS only]
# *	pe	Private Eye display
# Other displays:
#   MS Windows:
#	mswin	Microsoft Windows 3.0, 3.1   [MS Windows only]
#	mswindll  Microsoft Windows 3.1 DLL    [MS Windows only]
#	mswinprn  Microsoft Windows 3.0, 3.1 printer   [MS Windows only]
#   OS/2:
# *	os2pm	OS/2 Presentation Manager   [OS/2 only]
# *	os2dll	OS/2 DLL bitmap             [OS/2 only]
#   Unix and VMS:
#   ****** NOTE: For direct frame buffer addressing under SCO Unix or Xenix,
#   ****** edit the definition of EGAVGA below.
# *	att3b1	AT&T 3b1/Unixpc monochrome display   [3b1 only]
# *	lvga256  Linux vgalib, 256-color VGA modes  [Linux only]
# *	sonyfb	Sony Microsystems monochrome display   [Sony only]
# *	sunview  SunView window system   [SunOS only]
# +	vgalib	Linux PC with VGALIB   [Linux only]
#	x11	X Windows version 11, release >=4   [Unix and VMS only]
#	x11alpha  X Windows masquerading as a device with alpha capability
#	x11cmyk  X Windows masquerading as a 1-bit-per-plane CMYK device
#	x11mono  X Windows masquerading as a black-and-white device
#   Platform-independent:
# *	sxlcrt	CRT sixels, e.g. for VT240-like terminals
# Printers:
# *	ap3250	Epson AP3250 printer
# *	appledmp  Apple Dot Matrix Printer (should also work with Imagewriter)
#	bj10e	Canon BubbleJet BJ10e
# *	bj200	Canon BubbleJet BJ200
# *	cdeskjet  H-P DeskJet 500C with 1 bit/pixel color
# *	cdjcolor  H-P DeskJet 500C with 24 bit/pixel color and
#		high-quality color (Floyd-Steinberg) dithering;
#		also good for DeskJet 540C
# *	cdjmono  H-P DeskJet 500C printing black only;
#		also good for DeskJet 510, 520, and 540C (black only)
# *	cdj500	H-P DeskJet 500C (same as cdjcolor)
# *	cdj550	H-P DeskJet 550C;
#		also good for DeskJet 560C
# *	cp50	Mitsubishi CP50 color printer
# *	declj250  alternate DEC LJ250 driver
# +	deskjet  H-P DeskJet and DeskJet Plus
#	djet500  H-P DeskJet 500
# *	djet500c  H-P DeskJet 500C alternate driver (does not work on 550C)
# *	dnj650c  H-P DesignJet 650C
#	epson	Epson-compatible dot matrix printers (9- or 24-pin)
# *	eps9mid  Epson-compatible 9-pin, interleaved lines
#		(intermediate resolution)
# *	eps9high  Epson-compatible 9-pin, interleaved lines
#		(triple resolution)
# *	epsonc	Epson LQ-2550 and Fujitsu 3400/2400/1200 color printers
# *	ibmpro  IBM 9-pin Proprinter
# *	imagen	Imagen ImPress printers
# *	iwhi	Apple Imagewriter in high-resolution mode
# *	iwlo	Apple Imagewriter in low-resolution mode
# *	iwlq	Apple Imagewriter LQ in 320 x 216 dpi mode
# *	jetp3852  IBM Jetprinter ink-jet color printer (Model #3852)
# +	laserjet  H-P LaserJet
# *	la50	DEC LA50 printer
# *	la70	DEC LA70 printer
# *	la70t	DEC LA70 printer with low-resolution text enhancement
# *	la75	DEC LA75 printer
# *	la75plus DEC LA75plus printer
# *	lbp8	Canon LBP-8II laser printer
# *	lips3	Canon LIPS III laser printer in English (CaPSL) mode
# *	ln03	DEC LN03 printer
# *	lj250	DEC LJ250 Companion color printer
# +	ljet2p	H-P LaserJet IId/IIp/III* with TIFF compression
# +	ljet3	H-P LaserJet III* with Delta Row compression
# +	ljet4	H-P LaserJet 4 (defaults to 600 dpi)
# +	lj4dith  H-P LaserJet 4 with Floyd-Steinberg dithering
# +	ljetplus  H-P LaserJet Plus
# *	lp2563	H-P 2563B line printer
# *	m8510	C.Itoh M8510 printer
# *	necp6	NEC P6/P6+/P60 printers at 360 x 360 DPI resolution
# *	nwp533  Sony Microsystems NWP533 laser printer   [Sony only]
# *	oki182	Okidata MicroLine 182
# *	paintjet  alternate H-P PaintJet color printer
# *	pj	H-P PaintJet XL driver 
# *	pjetxl	alternate H-P PaintJet XL driver
# *	pjxl	H-P PaintJet XL color printer
# *	pjxl300  H-P PaintJet XL300 color printer;
#		also good for PaintJet 1200C
# *	r4081	Ricoh 4081 laser printer
# *	sj48	StarJet 48 inkjet printer
# *	sparc	SPARCprinter
# *	st800	Epson Stylus 800 printer
# *	t4693d2  Tektronix 4693d color printer, 2 bits per R/G/B component
# *	t4693d4  Tektronix 4693d color printer, 4 bits per R/G/B component
# *	t4693d8  Tektronix 4693d color printer, 8 bits per R/G/B component
# *	tek4696  Tektronix 4695/4696 inkjet plotter
# *	xes	Xerox XES printers (2700, 3700, 4045, etc.)
# Fax systems:
# *	dfaxhigh  DigiBoard, Inc.'s DigiFAX software format (high resolution)
# *	dfaxlow  DigiFAX low (normal) resolution
# Fax file format:
#   ****** NOTE: all of these drivers adjust the page size to match
#   ****** one of the three CCITT standard sizes (U.S. letter with A4 width,
#   ****** A4, or B4).
#	faxg3	Group 3 fax, with EOLs but no header or EOD
#	faxg32d  Group 3 2-D fax, with EOLs but no header or EOD
#	faxg4	Group 4 fax, with EOLs but no header or EOD
#	tiffg3	Group 3 fax TIFF
#	tiffg32d  Group 3 2-D fax TIFF
#	tiffg4	Group 4 fax TIFF
# File formats and others:
#	bit	Plain bits, monochrome
#	bitrgb	Plain bits, RGB
#	bitcmyk  Plain bits, CMYK
#	bmpmono	Monochrome MS Windows .BMP file format
#	bmp16	4-bit (EGA/VGA) .BMP file format
#	bmp256	8-bit (256-color) .BMP file format
#	bmp16m	24-bit .BMP file format
# *	cif	CIF file format for VLSI
#	gifmono	Monochrome GIF file format
#	gif8	8-bit color GIF file format
# *	mgrmono  1-bit monochrome MGR devices
# *	mgrgray2  2-bit gray scale MGR devices
# *	mgrgray4  4-bit gray scale MGR devices
# *	mgrgray8  8-bit gray scale MGR devices
# *	mgr4	4-bit (VGA) color MGR devices
# *	mgr8	8-bit color MGR devices
#	pcxmono	Monochrome PCX file format
#	pcxgray	8-bit gray scale PCX file format
#	pcx16	4-bit planar (EGA/VGA) color PCX file format
#	pcx256	8-bit chunky color PCX file format
#	pcx24b	24-bit color PCX file format, 3 8-bit planes
#	psmono	Monochrome PostScript (Level 1) image
#	pbm	Portable Bitmap (plain format)
#	pbmraw	Portable Bitmap (raw format)
#	pgm	Portable Graymap (plain format)
#	pgmraw	Portable Graymap (raw format)
#	ppm	Portable Pixmap (plain format) (RGB)
#	ppmraw	Portable Pixmap (raw format) (RGB)

# User-contributed drivers marked with * require hardware or software
# that is not available to Aladdin Enterprises.  Please contact the
# original contributors, not Aladdin Enterprises, if you have questions.
# Contact information appears in the driver entry below.
#
# Drivers marked with a + are maintained by Aladdin Enterprises with
# the assistance of users, since Aladdin Enterprises doesn't have access to
# the hardware for these either.

# If you add drivers, it would be nice if you kept each list
# in alphabetical order.

# ---------------------------- End of catalog ---------------------------- #

# As noted in gs.mak, DEVICE_DEVS and DEVICE_DEVS1..9 select the devices
# that should be included in a given configuration.  By convention,
# these are used as follows:
#	DEVICE_DEVS - the default device, and any display devices.
#	DEVICE_DEVS1 - additional display devices if needed.
#	DEVICE_DEVS2 - dot matrix printers.
#	DEVICE_DEVS3 - H-P monochrome printers.
#	DEVICE_DEVS4 - H-P color printers.
#	DEVICE_DEVS5 - additional H-P printers if needed.
#	DEVICE_DEVS6 - other ink-jet and laser printers.
#	DEVICE_DEVS7 - GIF, TIFF, and fax file formats.
#	DEVICE_DEVS8 - BMP and PCX file formats.
#	DEVICE_DEVS9 - PBM/PGM/PPM and bit file formats.
# Feel free to disregard this convention if it gets in your way.

# If you want to add a new device driver, the examples below should be
# enough of a guide to the correct form for the makefile rules.

# All device drivers depend on the following:
GDEV=$(AK) echogs$(XE) $(gserrors_h) $(gx_h) $(gxdevice_h)

# Define the header files for device drivers.  Every header file used by
# more than one device driver must be listed here.
gdev8bcm_h=gdev8bcm.h
gdevpccm_h=gdevpccm.h
gdevpcfb_h=gdevpcfb.h $(dos__h)
gdevpcl_h=gdevpcl.h
gdevsvga_h=gdevsvga.h
gdevx_h=gdevx.h

###### ----------------------- Device support ----------------------- ######

# Provide a mapping between StandardEncoding and ISOLatin1Encoding.
gdevemap.$(OBJ): gdevemap.c $(AK) $(std_h)

# Implement dynamic color management for 8-bit mapped color displays.
gdev8bcm.$(OBJ): gdev8bcm.c $(AK) \
  $(gx_h) $(gxdevice_h) $(gdev8bcm_h)

###### ------------------- MS-DOS display devices ------------------- ######

# There are really only three drivers: an EGA/VGA driver (4 bit-planes,
# plane-addressed), a SuperVGA driver (8 bit-planes, byte addressed),
# and a special driver for the S3 chip.

# PC display color mapping
gdevpccm.$(OBJ): gdevpccm.c $(AK) \
  $(gx_h) $(gsmatrix_h) $(gxdevice_h) $(gdevpccm_h)

### ----------------------- EGA and VGA displays ----------------------- ###

gdevegaa.$(OBJ): gdevegaa.asm

ETEST=ega.$(OBJ) $(ega_) gdevpcfb.$(OBJ) gdevpccm.$(OBJ) gdevegaa.$(OBJ)
ega.exe: $(ETEST) libc$(MM).tr
	$(COMPDIR)\tlink $(LCT) $(LO) $(LIBDIR)\c0$(MM) @ega.tr @libc$(MM).tr

ega.$(OBJ): ega.c $(GDEV)
	$(CCC) -v ega.c

# The shared MS-DOS makefile defines PCFBASM as either gdevegaa.$(OBJ)
# or an empty string.

# NOTE: for direct frame buffer addressing under SCO Unix or Xenix,
# change gdevevga to gdevsco in the following line.  Also, since
# SCO's /bin/as does not support the "out" instructions, you must build
# the gnu assembler and have it on your path as "as".
EGAVGA=gdevevga.$(OBJ) gdevpcfb.$(OBJ) gdevpccm.$(OBJ) $(PCFBASM)
#EGAVGA=gdevsco.$(OBJ) gdevpcfb.$(OBJ) gdevpccm.$(OBJ) $(PCFBASM)

gdevevga.$(OBJ): gdevevga.c $(GDEV) $(gdevpcfb_h)
	$(CCD) gdevevga.c

gdevsco.$(OBJ): gdevsco.c $(GDEV) $(gdevpcfb_h)

# Common code for MS-DOS and SCO.
gdevpcfb.$(OBJ): gdevpcfb.c $(GDEV) $(MAKEFILE) $(gconfigv_h) \
  $(gdevpccm_h) $(gdevpcfb_h) $(gsparam_h)
	$(CCD) gdevpcfb.c

# The EGA/VGA family includes EGA and VGA.  Many SuperVGAs in 800x600,
# 16-color mode can share the same code; see the next section below.

ega.dev: $(EGAVGA)
	$(SETDEV) ega $(EGAVGA)

vga.dev: $(EGAVGA)
	$(SETDEV) vga $(EGAVGA)

### ------------------------- SuperVGA displays ------------------------ ###

# SuperVGA displays in 16-color, 800x600 mode are really just slightly
# glorified VGA's, so we can handle them all with a single driver.
# The way to select them on the command line is with
#	-sDEVICE=svga16 -dDisplayMode=NNN
# where NNN is the display mode in decimal.  See use.doc for the modes
# for some popular display chipsets.

svga16.dev: $(EGAVGA)
	$(SETDEV) svga16 $(EGAVGA)

# More capable SuperVGAs have a wide variety of slightly differing
# interfaces, so we need a separate driver for each one.

SVGA=gdevsvga.$(OBJ) gdevpccm.$(OBJ) $(PCFBASM)

gdevsvga.$(OBJ): gdevsvga.c $(GDEV) $(MAKEFILE) $(gconfigv_h) \
  $(gxarith_h) $(gdevpccm_h) $(gdevpcfb_h) $(gdevsvga_h)
	$(CCD) gdevsvga.c

# The SuperVGA family includes: ATI Wonder, S3, Trident, Tseng ET3000/4000,
# and VESA.

atiw.dev: $(SVGA)
	$(SETDEV) atiw $(SVGA)

tseng.dev: $(SVGA)
	$(SETDEV) tseng $(SVGA)

tseng8.dev: $(SVGA)
	$(SETDEV) tseng8 $(SVGA)

tvga.dev: $(SVGA)
	$(SETDEV) tvga $(SVGA)

vesa.dev: $(SVGA)
	$(SETDEV) vesa $(SVGA)

# The S3 driver doesn't share much code with the others.

s3vga_=$(SVGA) gdevs3ga.$(OBJ) gdevsvga.$(OBJ) gdevpccm.$(OBJ)
s3vga.dev: $(s3vga_)
	$(SETDEV) s3vga $(s3vga_)

gdevs3ga.$(OBJ): gdevs3ga.c $(GDEV) $(MAKEFILE) $(gdevpcfb_h) $(gdevsvga_h)
	$(CCD) gdevs3ga.c

### ------------ The BGI (Borland Graphics Interface) device ----------- ###

cgaf.$(OBJ): $(BGIDIR)\cga.bgi
	$(BGIDIR)\bgiobj /F $(BGIDIR)\cga

egavgaf.$(OBJ): $(BGIDIR)\egavga.bgi
	$(BGIDIR)\bgiobj /F $(BGIDIR)\egavga

# Include egavgaf.$(OBJ) for debugging only.
bgi_=gdevbgi.$(OBJ) cgaf.$(OBJ)
bgi.dev: $(bgi_)
	$(SETDEV) bgi $(bgi_)
	$(ADDMOD) bgi -lib $(LIBDIR)\graphics

gdevbgi.$(OBJ): gdevbgi.c $(GDEV) $(MAKEFILE) $(gxxfont_h)
	$(CCC) -DBGI_LIB="$(BGIDIRSTR)" gdevbgi.c

### ------------------- The Hercules Graphics display ------------------- ###

herc_=gdevherc.$(OBJ)
herc.dev: $(herc_)
	$(SETDEV) herc $(herc_)

gdevherc.$(OBJ): gdevherc.c $(GDEV)
	$(CCC) gdevherc.c

###### ----------------------- Other displays ------------------------ ######

### ---------------------- The Private Eye display ---------------------- ###
### Note: this driver was contributed by a user:                          ###
###   please contact narf@media-lab.media.mit.edu if you have questions.  ###

pe_=gdevpe.$(OBJ)
pe.dev: $(pe_)
	$(SETDEV) pe $(pe_)

gdevpe.$(OBJ): gdevpe.c $(GDEV)

### -------------------- The MS-Windows 3.n display --------------------- ###

gdevmswn_h=gdevmswn.h $(GDEV) gp_mswin.h

# Choose one of gdevwddb or gdevwdib here.
mswin_=gdevmswn.$(OBJ) gdevmsxf.$(OBJ) gdevwdib.$(OBJ) \
  gdevemap.$(OBJ) gdevpccm.$(OBJ)
mswin.dev: $(mswin_)
	$(SETDEV) mswin $(mswin_)

gdevmswn.$(OBJ): gdevmswn.c $(gdevmswn_h) $(gp_h) $(gpcheck_h) \
  $(gsparam_h) $(gdevpccm_h)

gdevmsxf.$(OBJ): gdevmsxf.c $(ctype__h) $(math__h) $(memory__h) \
  $(gdevmswn_h) $(gsstruct_h) $(gsutil_h) $(gxxfont_h)

# An implementation using a device-dependent bitmap.
gdevwddb.$(OBJ): gdevwddb.c $(gdevmswn_h)

# An implementation using a DIB filled by an image device.
gdevwdib.$(OBJ): gdevwdib.c $(dos__h) $(gdevmswn_h)

### -------------------- The MS-Windows 3.n DLL ------------------------- ###

# Must use gdevwdib.  mswin device must use the same.
mswindll_=gdevmswn.$(OBJ) gdevmsxf.$(OBJ) gdevwdib.$(OBJ) \
  gdevemap.$(OBJ) gdevpccm.$(OBJ)
mswindll.dev: $(mswindll_)
	$(SETDEV) mswindll $(mswindll_)

### -------------------- The MS-Windows 3.n printer --------------------- ###

mswinprn_=gdevwprn.$(OBJ) gdevmsxf.$(OBJ)
mswinprn.dev: $(mswinprn_)
	$(SETDEV) mswinprn $(mswinprn_)

gdevwprn.$(OBJ): gdevwprn.c $(gdevmswn_h) $(gp_h)

### ------------------ OS/2 Presentation Manager device ----------------- ###

os2pm_=gdevpm.$(OBJ) gdevpccm.$(OBJ)
os2pm.dev: $(os2pm_)
	$(SETDEV) os2pm $(os2pm_)

os2dll_=gdevpm.$(OBJ) gdevpccm.$(OBJ)
os2dll.dev: $(os2dll_)
	$(SETDEV) os2dll $(os2dll_)

gdevpm.$(OBJ): gdevpm.c $(gp_h) $(gpcheck_h) \
  $(gsexit_h) $(gsparam_h) $(gdevpccm_h)

### -------------- The AT&T 3b1 Unixpc monochrome display --------------- ###
### Note: this driver was contributed by a user: please contact           ###
###       Andy Fyfe (andy@cs.caltech.edu) if you have questions.          ###

att3b1_=gdev3b1.$(OBJ)
att3b1.dev: $(att3b1_)
	$(SETDEV) att3b1 $(att3b1_)

gdev3b1.$(OBJ): gdev3b1.c

### ------------------------- DEC sixel displays ------------------------ ###
### Note: this driver was contributed by a user: please contact           ###
###   Phil Keegstra (keegstra@tonga.gsfc.nasa.gov) if you have questions. ###

# This is a "printer" device, but it probably shouldn't be.
# I don't know why the implementor chose to do it this way.
sxlcrt_=gdevln03.$(OBJ) gdevprn.$(OBJ)
sxlcrt.dev: $(sxlcrt_)
	$(SETDEV) sxlcrt $(sxlcrt_)

###### --------------- Memory-buffered printer devices --------------- ######

PDEVH=$(GDEV) $(gdevprn_h)

gdevprn.$(OBJ): gdevprn.c $(PDEVH) $(gp_h) $(gsparam_h)

### --------------------- The Apple printer devices --------------------- ###
### Note: these drivers were contributed by users.                        ###
###   If you have questions about the DMP driver, please contact          ###
###	Mark Wedel (master@cats.ucsc.edu).                                ###
###   If you have questions about the Imagewriter drivers, please contact ###
###	Jonathan Luckey (luckey@rtfm.mlb.fl.us).                          ###
###   If you have questions about the Imagewriter LQ driver, please       ###
###	contact Scott Barker (barkers@cuug.ab.ca).                        ###

appledmp_=gdevadmp.$(OBJ) gdevprn.$(OBJ)

gdevadmp.$(OBJ): gdevadmp.c $(GDEV) $(gdevprn_h)

appledmp.dev: $(appledmp_)
	$(SETDEV) appledmp $(appledmp_)

iwhi.dev: $(appledmp_)
	$(SETDEV) iwhi $(appledmp_)

iwlo.dev: $(appledmp_)
	$(SETDEV) iwlo $(appledmp_)

iwlq.dev: $(appledmp_)
	$(SETDEV) iwlq $(appledmp_)

### ------------ The Canon BubbleJet BJ10e and BJ200 devices ------------ ###

bj10e_=gdevbj10.$(OBJ) gdevprn.$(OBJ)

bj10e.dev: $(bj10e_)
	$(SETDEV) bj10e $(bj10e_)

bj200.dev: $(bj10e_)
	$(SETDEV) bj200 $(bj10e_)

gdevbj10.$(OBJ): gdevbj10.c $(PDEVH)

### ----------- The H-P DeskJet and LaserJet printer devices ----------- ###

### These are essentially the same device.
### NOTE: printing at full resolution (300 DPI) requires a printer
###   with at least 1.5 Mb of memory.  150 DPI only requires .5 Mb.
### Note that the lj4dith driver is included with the H-P color printer
###   drivers below.

HPPCL=gdevprn.$(OBJ) gdevpcl.$(OBJ)
HPMONO=gdevdjet.$(OBJ) $(HPPCL)

gdevpcl.$(OBJ): gdevpcl.c $(PDEVH) $(gdevpcl_h)

gdevdjet.$(OBJ): gdevdjet.c $(PDEVH) $(gdevpcl_h)

deskjet.dev: $(HPMONO)
	$(SETDEV) deskjet $(HPMONO)

djet500.dev: $(HPMONO)
	$(SETDEV) djet500 $(HPMONO)

laserjet.dev: $(HPMONO)
	$(SETDEV) laserjet $(HPMONO)

ljetplus.dev: $(HPMONO)
	$(SETDEV) ljetplus $(HPMONO)

### Selecting ljet2p provides TIFF (mode 2) compression on LaserJet III,
### IIIp, IIId, IIIsi, IId, and IIp. 

ljet2p.dev: $(HPMONO)
	$(SETDEV) ljet2p $(HPMONO)

### Selecting ljet3 provides Delta Row (mode 3) compression on LaserJet III,
### IIIp, IIId, IIIsi.

ljet3.dev: $(HPMONO)
	$(SETDEV) ljet3 $(HPMONO)

### Selecting ljet4 also provides Delta Row compression on LaserJet IV series.

ljet4.dev: $(HPMONO)
	$(SETDEV) ljet4 $(HPMONO)

lp2563.dev: $(HPMONO)
	$(SETDEV) lp2563 $(HPMONO)

### The H-P DeskJet, PaintJet, and DesignJet family color printer devices.###
### Note: there are two different 500C drivers, both contributed by users.###
###   If you have questions about the djet500c driver,                    ###
###       please contact AKayser@et.tudelft.nl.                           ###
###   If you have questions about the cdj* drivers,                       ###
###       please contact g.cameron@biomed.abdn.ac.uk.                     ###
###   If you have questions about the dnj560c driver,                     ###
###       please contact koert@zen.cais.com.                              ###
###   If you have questions about the lj4dith driver,                     ###
###       please contact Eckhard.Rueggeberg@ts.go.dlr.de.                 ###

cdeskjet_=gdevcdj.$(OBJ) $(HPPCL)

cdeskjet.dev: $(cdeskjet_)
	$(SETDEV) cdeskjet $(cdeskjet_)

cdjcolor.dev: $(cdeskjet_)
	$(SETDEV) cdjcolor $(cdeskjet_)

cdjmono.dev: $(cdeskjet_)
	$(SETDEV) cdjmono $(cdeskjet_)

cdj500.dev: $(cdeskjet_)
	$(SETDEV) cdj500 $(cdeskjet_)

cdj550.dev: $(cdeskjet_)
	$(SETDEV) cdj550 $(cdeskjet_)

declj250.dev: $(cdeskjet_)
	$(SETDEV) declj250 $(cdeskjet_)

dnj650c.dev: $(cdeskjet_)
	$(SETDEV) dnj650c $(cdeskjet_)

lj4dith.dev: $(cdeskjet_)
	$(SETDEV) lj4dith $(cdeskjet_)

pj.dev: $(cdeskjet_)
	$(SETDEV) pj $(cdeskjet_)

pjxl.dev: $(cdeskjet_)
	$(SETDEV) pjxl $(cdeskjet_)

pjxl300.dev: $(cdeskjet_)
	$(SETDEV) pjxl300 $(cdeskjet_)

# NB: you can also customise the build if required, using
# -DBitsPerPixel=<number> if you wish the default to be other than 24
# for the generic drivers (cdj500, cdj550, pjxl300, pjtest, pjxltest).
gdevcdj.$(OBJ): gdevcdj.c $(PDEVH) $(gdevpcl_h)
	$(CCC) gdevcdj.c

djet500c_=gdevdjtc.$(OBJ) $(HPPCL)
djet500c.dev: $(djet500c_)
	$(SETDEV) djet500c $(djet500c_)

gdevdjtc.$(OBJ): gdevdjtc.c $(PDEVH) $(gdevpcl_h)

### -------------------- The Mitsubishi CP50 printer -------------------- ###
### Note: this driver was contributed by a user: please contact           ###
###       Michael Hu (michael@ximage.com) if you have questions.          ###

cp50_=gdevcp50.$(OBJ) gdevprn.$(OBJ)
cp50.dev: $(cp50_)
	$(SETDEV) cp50 $(cp50_)

gdevcp50.$(OBJ): gdevcp50.c $(PDEVH)

### ----------------- The generic Epson printer device ----------------- ###
### Note: most of this code was contributed by users.  Please contact    ###
###       the following people if you have questions:                    ###
###   eps9mid - Guenther Thomsen (thomsen@cs.tu-berlin.de)               ###
###   eps9high - David Wexelblat (dwex@mtgzfs3.att.com)                  ###
###   ibmpro - James W. Birdsall (jwbirdsa@picarefy.picarefy.com)        ###

epson_=gdevepsn.$(OBJ) gdevprn.$(OBJ)

epson.dev: $(epson_)
	$(SETDEV) epson $(epson_)

eps9mid.dev: $(epson_)
	$(SETDEV) eps9mid $(epson_)

eps9high.dev: $(epson_)
	$(SETDEV) eps9high $(epson_)

gdevepsn.$(OBJ): gdevepsn.c $(PDEVH)

### ----------------- The IBM Proprinter printer device ---------------- ###

ibmpro.dev: $(epson_)
	$(SETDEV) ibmpro $(epson_)

### -------------- The Epson LQ-2550 color printer device -------------- ###
### Note: this driver was contributed by users: please contact           ###
###       Dave St. Clair (dave@exlog.com) if you have questions.         ###

epsonc_=gdevepsc.$(OBJ) gdevprn.$(OBJ)
epsonc.dev: $(epsonc_)
	$(SETDEV) epsonc $(epsonc_)

gdevepsc.$(OBJ): gdevepsc.c $(PDEVH)

### ------------ The Epson ESC/P 2 language printer devices ------------ ###
### Note: these drivers were contributed by a user: please contact       ###
###       Richard Brown (rab@tauon.ph.unimelb.edu.au) with any questions.###
### This driver supports the Stylus 800 and AP3250 printers.             ###

ESCP2=gdevescp.$(OBJ) gdevprn.$(OBJ)

gdevescp.$(OBJ): gdevescp.c $(PDEVH)

ap3250.dev: $(ESCP2)
	$(SETDEV) ap3250 $(ESCP2)

st800.dev: $(ESCP2)
	$(SETDEV) st800 $(ESCP2)

### ------------ The H-P PaintJet color printer device ----------------- ###
### Note: this driver also supports the DEC LJ250 color printer, which   ###
###       has a PaintJet-compatible mode, and the PaintJet XL.           ###
### If you have questions about the XL, please contact Rob Reiss         ###
###       (rob@moray.berkeley.edu).                                      ###

PJET=gdevpjet.$(OBJ) $(HPPCL)

gdevpjet.$(OBJ): gdevpjet.c $(PDEVH) $(gdevpcl_h)

lj250.dev: $(PJET)
	$(SETDEV) lj250 $(PJET)

paintjet.dev: $(PJET)
	$(SETDEV) paintjet $(PJET)

pjetxl.dev: $(PJET)
	$(SETDEV) pjetxl $(PJET)

### -------------- Imagen ImPress Laser Printer device ----------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Alan Millar (AMillar@bolis.sf-bay.org) if you have questions.  ###
### Set USE_BYTE_STREAM if using parallel interface;                     ###
### Don't set it if using 'ipr' spooler (default).                       ###
### You may also add -DA4 if needed for A4 paper.			 ###

imagen_=gdevimgn.$(OBJ) gdevprn.$(OBJ)
imagen.dev: $(imagen_)
	$(SHP)gssetdev imagen $(imagen_)

gdevimgn.$(OBJ): gdevimgn.c $(PDEVH)
	$(CCC) gdevimgn.c			# for ipr spooler
#	$(CCC) -DUSE_BYTE_STREAM gdevimgn.c	# for parallel

### ------- The IBM 3852 JetPrinter color inkjet printer device -------- ###
### Note: this driver was contributed by users: please contact           ###
###       Kevin Gift (kgift@draper.com) if you have questions.           ###
### Note that the paper size that can be addressed by the graphics mode  ###
###   used in this driver is fixed at 7-1/2 inches wide (the printable   ###
###   width of the jetprinter itself.)                                   ###

jetp3852_=gdev3852.$(OBJ) gdevprn.$(OBJ)
jetp3852.dev: $(jetp3852_)
	$(SETDEV) jetp3852 $(jetp3852_)

gdev3852.$(OBJ): gdev3852.c $(PDEVH) $(gdevpcl_h)

### ---------- The Canon LBP-8II and LIPS III printer devices ---------- ###
### Note: these drivers were contributed by users.                       ###
### For questions about the LBP8 driver, please contact                  ###
###       Tom Quinn (trq@prg.oxford.ac.uk).                              ###
### For questions about the LIPS III driver, please contact              ###
###       Kenji Okamoto (okamoto@okamoto.cias.osakafu-u.ac.jp).          ###

lbp8_=gdevlbp8.$(OBJ) gdevprn.$(OBJ)
lbp8.dev: $(lbp8_)
	$(SETDEV) lbp8 $(lbp8_)

lips3.dev: $(lbp8_)
	$(SETDEV) lips3 $(lips3_)

gdevlbp8.$(OBJ): gdevlbp8.c $(PDEVH)

### ----------- The DEC LN03/LA50/LA70/LA75 printer devices ------------ ###
### Note: this driver was contributed by users: please contact           ###
###       Ulrich Mueller (ulm@vsnhd1.cern.ch) if you have questions.     ###
### For questions about LA50 and LA75, please contact                    ###
###       Ian MacPhedran (macphed@dvinci.USask.CA).                      ###
### For questions about the LA70, please contact                         ###
###       Bruce Lowekamp (lowekamp@csugrad.cs.vt.edu).                   ###
### For questions about the LA75plus, please contact                     ###
###       Andre' Beck (Andre_Beck@IRS.Inf.TU-Dresden.de).                ###

ln03_=gdevln03.$(OBJ) gdevprn.$(OBJ)
ln03.dev: $(ln03_)
	$(SETDEV) ln03 $(ln03_)

la50.dev: $(ln03_)
	$(SETDEV) la50 $(ln03_)

la70.dev: $(ln03_)
	$(SETDEV) la70 $(ln03_)

la75.dev: $(ln03_)
	$(SETDEV) la75 $(ln03_)

la75plus.dev: $(ln03_)
	$(SETDEV) la75plus $(ln03_)

gdevln03.$(OBJ): gdevln03.c $(PDEVH)

# LA70 driver with low-resolution text enhancement.

la70t_=gdevla7t.$(OBJ) gdevprn.$(OBJ)
la70t.dev: $(la70t_)
	$(SETDEV) la70t $(la70t_)

gdevla7t.$(OBJ): gdevla7t.c $(PDEVH)

### -------------- The C.Itoh M8510 printer device --------------------- ###
### Note: this driver was contributed by a user: please contact Bob      ###
###       Smith <bob@snuffy.penfield.ny.us> if you have questions.       ###

m8510_=gdev8510.$(OBJ) gdevprn.$(OBJ)
m8510.dev: $(m8510_)
	$(SETDEV) m8510 $(m8510_)

gdev8510.$(OBJ): gdev8510.c $(PDEVH)

### --------------------- The NEC P6 family devices -------------------- ###

necp6_=gdevnp6.$(OBJ) gdevprn.$(OBJ)
necp6.dev: $(necp6_)
	$(SETDEV) necp6 $(necp6_)

gdevnp6.$(OBJ): gdevnp6.c $(PDEVH)

### ----------------- The Okidata MicroLine 182 device ----------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Maarten Koning (smeg@bnr.ca) if you have questions.            ###

oki182_=gdevo182.$(OBJ) gdevprn.$(OBJ)
oki182.dev: $(oki182_)
	$(SETDEV) oki182 $(oki182_)

gdevo182.$(OBJ): gdevo182.c $(PDEVH)

### ------------- The Ricoh 4081 laser printer device ------------------ ###
### Note: this driver was contributed by users:                          ###
###       please contact kdw@oasis.icl.co.uk if you have questions.      ###

r4081_=gdev4081.$(OBJ) gdevprn.$(OBJ)
r4081.dev: $(r4081_)
	$(SETDEV) r4081 $(r4081_)

gdev4081.$(OBJ): gdev4081.c $(PDEVH)

###### ------------------------ Sony devices ------------------------ ######
### Note: these drivers were contributed by users: please contact        ###
###       Mike Smolenski (mike@intertech.com) if you have questions.     ###

### ------------------- Sony NeWS frame buffer device ------------------ ###

sonyfb_=gdevsnfb.$(OBJ) gdevprn.$(OBJ)
sonyfb.dev: $(sonyfb_)
	$(SETDEV) sonyfb $(sonyfb_)

gdevsnfb.$(OBJ): gdevsnfb.c $(PDEVH)

### -------------------- Sony NWP533 printer device -------------------- ###
### Note: this driver was contributed by a user: please contact Tero     ###
###       Kivinen (kivinen@joker.cs.hut.fi) if you have questions.       ###

nwp533_=gdevn533.$(OBJ) gdevprn.$(OBJ)
nwp533.dev: $(nwp533_)
	$(SETDEV) nwp533 $(nwp533_)

gdevn533.$(OBJ): gdevn533.c $(PDEVH)

### ------------------------- The SPARCprinter ------------------------- ###
### Note: this driver was contributed by users: please contact Martin    ###
###       Schulte (schulte@thp.uni-koeln.de) if you have questions.      ###
###       He would also like to hear from anyone using the driver.       ###
### Please consult the source code for additional documentation.         ###

sparc_=gdevsppr.$(OBJ) gdevprn.$(OBJ)
sparc.dev: $(sparc_)
	$(SETDEV) sparc $(sparc_)

gdevsppr.$(OBJ): gdevsppr.c $(PDEVH)

### ----------------- The StarJet SJ48 device -------------------------- ###
### Note: this driver was contributed by a user: if you have questions,  ###
###	                      .                                          ###
###       please contact Mats Akerblom (f86ma@dd.chalmers.se).           ###

sj48_=gdevsj48.$(OBJ) gdevprn.$(OBJ)
sj48.dev: $(sj48_)
	$(SETDEV) sj48 $(sj48_)

gdevsj48.$(OBJ): gdevsj48.c $(PDEVH)

###### --------------------- The SunView device --------------------- ######
### Note: this driver is maintained by a user: if you have questions,    ###
###       please contact Andreas Stolcke (stolcke@icsi.berkeley.edu).    ###

sunview_=gdevsun.$(OBJ)
sunview.dev: $(sunview_)
	$(SETDEV) sunview $(sunview_)
	$(ADDMOD) sunview -lib suntool sunwindow pixrect

gdevsun.$(OBJ): gdevsun.c $(GDEV) $(malloc__h) $(gscdefs_h) $(gsmatrix_h)

### ----------------- Tektronix 4396d color printer -------------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Karl Hakimian (hakimian@haney.eecs.wsu.edu)                    ###
###       if you have questions.                                         ###

t4693d_=gdev4693.$(OBJ) gdevprn.$(OBJ)
t4693d2.dev: $(t4693d_)
	$(SETDEV) t4693d2 $(t4693d_)

t4693d4.dev: $(t4693d_)
	$(SETDEV) t4693d4 $(t4693d_)

t4693d8.dev: $(t4693d_)
	$(SETDEV) t4693d8 $(t4693d_)

gdev4693.$(OBJ): gdev4693.c $(GDEV)

### -------------------- Tektronix ink-jet printers -------------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Karsten Spang (spang@nbivax.nbi.dk) if you have questions.     ###

tek4696_=gdevtknk.$(OBJ) gdevprn.$(OBJ)
tek4696.dev: $(tek4696_)
	$(SETDEV) tek4696 $(tek4696_)

gdevtknk.$(OBJ): gdevtknk.c $(PDEVH)

###### ------------------------- Fax devices ------------------------- ######

### --------------- Generic PostScript system compatible fax ------------ ###

# This code doesn't work yet.  Don't even think about using it.

PSFAX=gdevpfax.$(OBJ) gdevprn.$(OBJ)

psfax_=$(PSFAX)
psfax.dev: $(psfax_)
	$(SETDEV) psfax $(psfax_)
	$(ADDMOD) psfax -iodev Fax

gdevpfax.$(OBJ): gdevpfax.c $(PDEVH) $(gxiodev_h)

### ------------------------- The DigiFAX device ------------------------ ###
###    This driver outputs images in a format suitable for use with       ###
###    DigiBoard, Inc.'s DigiFAX software.  Use -sDEVICE=dfaxhigh for     ###
###    high resolution output, -sDEVICE=dfaxlow for normal output.        ###
### Note: this driver was contributed by a user: please contact           ###
###       Rick Richardson (rick@digibd.com) if you have questions.        ###

dfax_=gdevdfax.$(OBJ) gdevtfax.$(OBJ) gdevprn.$(OBJ) $(scfe_)

dfaxlow.dev: $(dfax_)
	$(SETDEV) dfaxlow $(dfax_)

dfaxhigh.dev: $(dfax_)
	$(SETDEV) dfaxhigh $(dfax_)

gdevdfax.$(OBJ): gdevdfax.c $(GDEV) $(gdevprn_h)

###### ------------------- Linux PC with vgalib ---------------------- ######
### Note: these drivers were contributed by users.                        ###
### For questions about the lvga256 driver, please contact                ###
###       Ludger Kunz (ludger.kunz@fernuni-hagen.de).                     ###
### For questions about the vgalib driver, please contact                 ###
###       Erik Talvola (talvola@gnu.ai.mit.edu).                          ###

lvga256_=gdevl256.$(OBJ)
lvga256.dev: $(lvga256_)
	$(SETDEV) lvga256 $(lvga256_)
	$(ADDMOD) lvga256 -lib vga vgagl

gdevl256.$(OBJ): gdevl256.c $(GDEV) $(MAKEFILE) $(gxxfont_h)

vgalib_=gdevvglb.$(OBJ)
vgalib.dev: $(vgalib_)
	$(SETDEV) vgalib $(vgalib_)
	$(ADDMOD) vgalib -lib vga

gdevvglb.$(OBJ): gdevvglb.c $(GDEV)

###### ----------------------- The X11 device ----------------------- ######

# Aladdin Enterprises does not support Ghostview.  For more information
# about Ghostview, please contact Tim Theisen (ghostview@cs.wisc.edu).

# See the main makefile for the definition of XLIBS.
x11_=gdevx.$(OBJ) gdevxini.$(OBJ) gdevxxf.$(OBJ) gdevemap.$(OBJ)
x11.dev: $(x11_)
	$(SETDEV) x11 $(x11_)
	$(ADDMOD) x11 -lib $(XLIBS)

# See the main makefile for the definition of XINCLUDE.
GDEVX=$(GDEV) x_.h gdevx.h $(MAKEFILE)
gdevx.$(OBJ): gdevx.c $(GDEVX) $(gsparam_h)
	$(CCC) $(XINCLUDE) gdevx.c

gdevxini.$(OBJ): gdevxini.c $(GDEVX) $(ctype__h)
	$(CCC) $(XINCLUDE) gdevxini.c

gdevxxf.$(OBJ): gdevxxf.c $(GDEVX) $(gsstruct_h) $(gsutil_h) $(gxxfont_h)
	$(CCC) $(XINCLUDE) gdevxxf.c

# Alternate X11-based devices to help debug other drivers.
# x11alpha pretends to have 4 bits of alpha channel.
# x11cmyk pretends to be a CMYK device with 1 bit each of C,M,Y,K.
# x11mono pretends to be a black-and-white device.
x11alt_=$(x11_) gdevxalt.$(OBJ)
x11alpha.dev: $(x11alt_)
	$(SETDEV) x11alpha $(x11alt_)
	$(ADDMOD) x11alpha -lib $(XLIBS)

x11cmyk.dev: $(x11alt_)
	$(SETDEV) x11cmyk $(x11alt_)
	$(ADDMOD) x11cmyk -lib $(XLIBS)

x11mono.dev: $(x11alt_)
	$(SETDEV) x11mono $(x11alt_)
	$(ADDMOD) x11mono -lib $(XLIBS)

gdevxalt.$(OBJ): gdevxalt.c $(GDEVX) $(PDEVH)
	$(CCC) $(XINCLUDE) gdevxalt.c

### ----------------- The Xerox XES printer device --------------------- ###
### Note: this driver was contributed by users: please contact           ###
###       Peter Flass (flass@lbdrscs.bitnet) if you have questions.      ###

xes_=gdevxes.$(OBJ) gdevprn.$(OBJ)
xes.dev: $(xes_)
	$(SETDEV) xes $(xes_)

gdevxes.$(OBJ): gdevxes.c $(GDEV)

### --------------------- The "plain bits" devices ---------------------- ###

bit_=gdevbit.$(OBJ) gdevprn.$(OBJ)

bit.dev: $(bit_)
	$(SETDEV) bit $(bit_)

bitrgb.dev: $(bit_)
	$(SETDEV) bitrgb $(bit_)

bitcmyk.dev: $(bit_)
	$(SETDEV) bitcmyk $(bit_)

gdevbit.$(OBJ): gdevbit.c $(PDEVH) $(gsparam_h) $(gxlum_h)

###### ----------------------- PC file formats ----------------------- ######

### ------------------------- .BMP file formats ------------------------- ###

bmp_=gdevbmp.$(OBJ) gdevpccm.$(OBJ) gdevprn.$(OBJ)

gdevbmp.$(OBJ): gdevbmp.c $(PDEVH) $(gdevpccm_h)

bmpmono.dev: $(bmp_)
	$(SETDEV) bmpmono $(bmp_)

bmp16.dev: $(bmp_)
	$(SETDEV) bmp16 $(bmp_)

bmp256.dev: $(bmp_)
	$(SETDEV) bmp256 $(bmp_)

bmp16m.dev: $(bmp_)
	$(SETDEV) bmp16m $(bmp_)

### -------------------- The CIF file format for VLSI ------------------ ###
### Note: this driver was contributed by a user: please contact          ###
###       Frederic Petrot (petrot@masi.ibp.fr) if you have questions.    ###

cif_=gdevcif.$(OBJ) gdevprn.$(OBJ)
cif.dev: $(cif_)
	$(SETDEV) cif $(cif_)

gdevcif.$(OBJ): gdevcif.c $(PDEVH)

### ------------------------- GIF file formats ------------------------- ###

GIF=gdevgif.$(OBJ) gdevpccm.$(OBJ) gdevprn.$(OBJ)

gdevgif.$(OBJ): gdevgif.c $(PDEVH) $(gdevpccm_h)

gifmono.dev: $(GIF)
	$(SETDEV) gifmono $(GIF)

gif8.dev: $(GIF)
	$(SETDEV) gif8 $(GIF)

### --------------------------- MGR devices ---------------------------- ###
### Note: these drivers were contributed by a user: please contact       ###
###       Carsten Emde (carsten@ce.pr.net.ch) if you have questions.     ###

MGR=gdevmgr.$(OBJ) gdevpccm.$(OBJ) gdevprn.$(OBJ)

gdevmgr.$(OBJ): gdevmgr.c $(PDEVH) $(gdevpccm_h) gdevmgr.h

mgrmono.dev: $(MGR)
	$(SETDEV) mgrmono $(MGR)

mgrgray2.dev: $(MGR)
	$(SETDEV) mgrgray2 $(MGR)

mgrgray4.dev: $(MGR)
	$(SETDEV) mgrgray4 $(MGR)

mgrgray8.dev: $(MGR)
	$(SETDEV) mgrgray8 $(MGR)

mgr4.dev: $(MGR)
	$(SETDEV) mgr4 $(MGR)

mgr8.dev: $(MGR)
	$(SETDEV) mgr8 $(MGR)

### ------------------------- PCX file formats ------------------------- ###

pcx_=gdevpcx.$(OBJ) gdevpccm.$(OBJ) gdevprn.$(OBJ)

gdevpcx.$(OBJ): gdevpcx.c $(PDEVH) $(gdevpccm_h) $(gxlum_h)

pcxmono.dev: $(pcx_)
	$(SETDEV) pcxmono $(pcx_)

pcxgray.dev: $(pcx_)
	$(SETDEV) pcxgray $(pcx_)

pcx16.dev: $(pcx_)
	$(SETDEV) pcx16 $(pcx_)

pcx256.dev: $(pcx_)
	$(SETDEV) pcx256 $(pcx_)

pcx24b.dev: $(pcx_)
	$(SETDEV) pcx24b $(pcx_)

###### ---------------- Portable Bitmap file formats ---------------- ######
### For more information, see the pbm(5), pgm(5), and ppm(5) man pages.  ###

pxm_=gdevpbm.$(OBJ) gdevprn.$(OBJ)

gdevpbm.$(OBJ): gdevpbm.c $(PDEVH) $(gscdefs_h) $(gxlum_h)

### Portable Bitmap (PBM, plain or raw format, magic numbers "P1" or "P4")

pbm.dev: $(pxm_)
	$(SETDEV) pbm $(pxm_)

pbmraw.dev: $(pxm_)
	$(SETDEV) pbmraw $(pxm_)

### Portable Graymap (PGM, plain or raw format, magic numbers "P2" or "P5")

pgm.dev: $(pxm_)
	$(SETDEV) pgm $(pxm_)

pgmraw.dev: $(pxm_)
	$(SETDEV) pgmraw $(pxm_)

### Portable Pixmap (PPM, plain or raw format, magic numbers "P3" or "P6")

ppm.dev: $(pxm_)
	$(SETDEV) ppm $(pxm_)

ppmraw.dev: $(pxm_)
	$(SETDEV) ppmraw $(pxm_)

### ---------------------- PostScript image format ---------------------- ###
### These devices make it possible to print Level 2 files on a Level 1    ###
###   printer, by converting them to a bitmap in PostScript format.       ###

ps_=gdevpsim.$(OBJ) gdevprn.$(OBJ)

gdevpsim.$(OBJ): gdevpsim.c $(GDEV) $(gdevprn_h)

psmono.dev: $(ps_)
	$(SETDEV) psmono $(ps_)

# Someday there will be RGB and CMYK variants....

### -------------------- Plain or TIFF fax encoding --------------------- ###
###    Use -sDEVICE=tiffg3 or tiffg4 and				  ###
###	  -r204x98 for low resolution output, or			  ###
###	  -r204x196 for high resolution output				  ###
###    These drivers recognize 3 page sizes: letter, A4, and B4.	  ###

tfax_=gdevtfax.$(OBJ) gdevprn.$(OBJ) $(scfe_)

gdevtfax.$(OBJ): gdevtfax.c $(GDEV) $(gdevprn_h) \
  $(time__h) $(gscdefs_h) gdevtifs.h

### Plain G3/G4 fax with no header

faxg3.dev: $(tfax_)
	$(SETDEV) faxg3 $(tfax_)

faxg32d.dev: $(tfax_)
	$(SETDEV) faxg32d $(tfax_)

faxg4.dev: $(tfax_)
	$(SETDEV) faxg4 $(tfax_)

### G3/G4 fax TIFF

tiffg3.dev: $(tfax_)
	$(SETDEV) tiffg3 $(tfax_)

tiffg32d.dev: $(tfax_)
	$(SETDEV) tiffg32d $(tfax_)

tiffg4.dev: $(tfax_)
	$(SETDEV) tiffg4 $(tfax_)
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

# Partial makefile, common to all Desqview/X configurations.

# This is the last part of the makefile for Desqview/X configurations.
# Since Unix make doesn't have an 'include' facility, we concatenate
# the various parts of the makefile together by brute force (in tar_cat).

# The following prevents GNU make from constructing argument lists that
# include all environment variables, which can easily be longer than
# brain-damaged system V allows.

.NOEXPORT:

# -------------------------------- Library -------------------------------- #

## The Desqview/X platform

dvx__=gp_nofb.$(OBJ) gp_dvx.$(OBJ) gp_unifs.$(OBJ) gp_dosfs.$(OBJ)
dvx_.dev: $(dvx__)
	$(SETMOD) dvx_ $(dvx__)

gp_dvx.$(OBJ): gp_dvx.c $(AK) $(string__h) $(gx_h) $(gsexit_h) $(gp_h) \
  $(time__h) $(dos__h)
	$(CCC) -D__DVX__ gp_dvx.c

# -------------------------- Auxiliary programs --------------------------- #

ansi2knr$(XE): ansi2knr.c $(stdio__h) $(string__h) $(malloc__h)
	$(CC) -o ansi2knr$(XE) $(CFLAGS) ansi2knr.c

echogs$(XE): echogs.c
	$(CC) -o echogs $(CFLAGS) echogs.c
	strip echogs
	coff2exe echogs
	del echogs

genarch$(XE): genarch.c
	$(CC) -o genarch genarch.c
	strip genarch
	coff2exe genarch
	del genarch

genconf$(XE): genconf.c
	$(CC) -o genconf genconf.c
	strip genconf
	coff2exe genconf
	del genconf

# We need to query the environment to construct gconfig_.h.
INCLUDE=/usr/include
gconfig_.h: $(MAKEFILE) echogs$(XE)
	echogs -w gconfig_.h -x 2f2a -s This file was generated automatically. -s -x 2a2f
	echogs -a gconfig_.h -x 23 define SYSTIME_H;
	echogs -a gconfig_.h -x 23 define DIRENT_H;

# ----------------------------- Main program ------------------------------ #

BEGINFILES=
CCBEGIN=$(CCC) *.c

# Interpreter main program

$(GS)$(XE): ld.tr echogs$(XE) gs.$(OBJ) $(INT_ALL) $(LIB_ALL) $(DEVS_ALL)
	cp ld.tr _temp_
	echo $(EXTRALIBS) -lm >>_temp_
	$(CC) $(LDFLAGS) $(XLIBDIRS) -o $(GS) gs.$(OBJ) @_temp_
	strip $(GS)
	coff2exe $(GS)  
	del $(GS)  
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

# Partial makefile common to all Unix and Desqview/X configurations.

# This is the very last part of the makefile for these configurations.
# Since Unix make doesn't have an 'include' facility, we concatenate
# the various parts of the makefile together by brute force (in tar_cat).

# Installation

TAGS:
	etags -t *.c *.h

install: install-exec install-data

install-exec: $(GS)
	-mkdir $(bindir)
	$(INSTALL_PROGRAM) $(GS) $(bindir)/$(GS)
	-mkdir $(scriptdir)
	for f in gsbj gsdj gsdj500 gslj gslp gsnd bdftops font2c \
ps2ascii ps2epsi ;\
	do $(INSTALL_PROGRAM) $$f $(scriptdir)/$$f ;\
	done

install-data: gs.1
	-mkdir $(mandir)
	-mkdir $(man1dir)
	for f in gs ps2epsi ;\
	do $(INSTALL_DATA) $$f.1 $(man1dir)/$$f.$(manext) ;\
	done
	-mkdir $(datadir)
	-mkdir $(gsdir)
	-mkdir $(gsdatadir)
	for f in gslp.ps gs_init.ps gs_btokn.ps gs_ccfnt.ps gs_dps1.ps \
gs_fonts.ps gs_kanji.ps gs_lev2.ps gs_pfile.ps gs_res.ps \
gs_setpd.ps gs_statd.ps gs_type0.ps gs_type1.ps \
gs_dbt_e.ps gs_iso_e.ps gs_ksb_e.ps gs_std_e.ps gs_sym_e.ps \
quit.ps Fontmap bdftops.ps decrypt.ps font2c.ps impath.ps landscap.ps \
level1.ps packfile.ps prfont.ps printafm.ps ps2ascii.ps ps2epsi.ps \
ps2image.ps pstoppm.ps showpage.ps type1enc.ps type1ops.ps wrfont.ps ;\
	do $(INSTALL_DATA) $$f $(gsdatadir)/$$f ;\
	done
	-mkdir $(docdir)
	for f in COPYING NEWS PUBLIC README current.doc devices.doc \
drivers.doc fonts.doc gs.1 hershey.doc history1.doc history2.doc humor.doc \
language.doc lib.doc make.doc ps2epsi.1 ps2epsi.doc psfiles.doc use.doc \
xfonts.doc ;\
	do $(INSTALL_DATA) $$f $(docdir)/$$f ;\
	done
	-mkdir $(exdir)
	for f in chess.ps cheq.ps colorcir.ps golfer.ps escher.ps \
snowflak.ps tiger.ps ;\
	do $(INSTALL_DATA) $$f $(exdir)/$$f ;\
	done
