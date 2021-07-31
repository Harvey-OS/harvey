#    Copyright (C) 1989, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
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

# $Id: gs.mak,v 1.5.2.2 2002/02/01 03:30:13 raph Exp $
# Generic makefile, common to all platforms, products, and configurations.
# The platform-specific makefiles `include' this file.

# Ghostscript makefiles cannot use default compilation rules, because
# they may place the output in (multiple) different directories.
# All compilation rules must have the form
#	<<compiler>> $(O_)<<output_file>> $(C_)<<input_file>>
# to cope with the divergent syntaxes of the various compilers.
# Spaces must appear where indicated, and nowhere else; in particular,
# there must be no space between $(O_) and the output file name.

# The platform-specific makefiles define the following symbols:
#	GS - the name of the executable (without the extension, if any).
#	GS_LIB_DEFAULT - the default directory/ies for searching for the
#	    initialization and font files at run time.
#	SEARCH_HERE_FIRST - the default setting of -P (whether or not to
#	    look for files in the current directory first).
#	GS_DOCDIR - the directory where documentation will be available
#	    at run time.
#	JSRCDIR - the directory where the IJG JPEG library source code
#	    is stored (at compilation time).
#	JVERSION - the major version number of the IJG JPEG library.
#	PSRCDIR, PVERSION - the same for libpng.
#	ZSRCDIR - the same for zlib.
#	SHARE_JPEG - normally 0; if set to 1, asks the linker to use
#	    an existing compiled libjpeg (-ljpeg) instead of compiling and
#	    linking libjpeg explicitly.  (We strongly recommend against
#	    doing this: see Make.htm details.)
#	JPEG_NAME - the name of the shared library, currently always
#	    jpeg (libjpeg, -lpjeg).
#	SHARE_LIBPNG - normally 0; if set to 1, asks the linker to use
#	    an existing compiled libpng (-lpng) instead of compiling and
#	    linking libpng explicitly.
#	LIBPNG_NAME, the name of the shared libpng, currently always
#	    png (libpng, -lpng).
#	SHARE_ZLIB - normally 0; if set to 1, asks the linker to use
#	    an existing compiled zlib (-lgz or -lz) instead of compiling
#	    and linking libgz/libz explicitly.
#	ZLIB_NAME - the name of the shared zlib, either gz (for libgz, -lgz)
#	    or z (for libz, -lz).
#	DEVICE_DEVS - the devices to include in the executable.
#	    See devs.mak for details.
#	DEVICE_DEVS1...DEVICE_DEVS20 - additional devices, if the definition
#	    of DEVICE_DEVS doesn't fit on one line.  See devs.mak for details.
#	FEATURE_DEVS - what features to include in the executable.
#	    Normally this is one of:
#		    $(PSD)psl1.dev - a PostScript Level 1 language interpreter.
#		    $(PSD)psl2.dev - a PostScript Level 2 language interpreter.
#		    $(PSD)psl3.dev - a PostScript LanguageLevel 3 language
#		      interpreter.
#	      and/or
#		    pdf - a PDF 1.2 interpreter.
#	    psl3 includes everything in psl2, and psl2 includes everything
#	      in psl1.  For backward compatibility, level1 is a synonym for
#	      psl1, and level2 is a synonym for psl2.  
#	    The following feature may be added to any of the standard
#	      configurations:
#		    ccfonts - precompile fonts into C, and link them
#			with the executable.  See Fonts.htm for details.
#	    The remaining features are of interest primarily to developers
#	      who want to "mix and match" features to create custom
#	      configurations:
#		    btoken - support for binary token encodings.
#			Included automatically in the dps and psl2 features.
#		    cidfont - (currently partial) support for CID-keyed fonts.
#		    color - support for the Level 1 CMYK color extensions.
#			Included automatically in the dps and psl2 features.
#		    compfont - support for composite (type 0) fonts.
#			Included automatically in the psl2 feature.
#		    dct - support for DCTEncode/Decode filters.
#			Included automatically in the psl2 feature.
#		    dps - (partial) support for Display PostScript extensions:
#			see Language.htm for details.
#		    dpsnext - (partial) support for Display PostScript
#			extensions with NeXT's additions.
#		    epsf - support for recognizing and skipping the binary
#			header of MS-DOS EPSF files.
#		    filter - support for Level 2 filters (other than eexec,
#			ASCIIHexEncode/Decode, NullEncode, PFBDecode,
#			RunLengthEncode/Decode, and SubFileDecode, which are
#			always included, and DCTEncode/Decode,
#			which are separate).
#			Included automatically in the psl2 feature.
#		    fzlib - support for zlibEncode/Decode filters.
#		    ttfont - support for TrueType fonts.
#		    type1 - support for Type 1 fonts and eexec;
#			normally included automatically in all configurations.
#		    type32 - support for Type 32 (downloaded bitmap) fonts.
#			Included automatically in the psl2 feature.
#		    type42 - support for Type 42 (embedded TrueType) fonts.
#			Included automatically in the psl2 feature.
#		There are quite a number of other sub-features that can be
#		selectively included in or excluded from a configuration,
#		but the above are the ones that are most likely to be of
#		interest.
#	COMPILE_INITS - normally 0; if set to 1, compiles the PostScript
#	    language initialization files (gs_init.ps et al) into the
#	    executable, eliminating the need for these files to be present
#	    at run time.
#	BAND_LIST_STORAGE - normally file; if set to memory, stores band
#	    lists in memory (with compression if needed).
#	BAND_LIST_COMPRESSOR - normally zlib: selects the compression method
#	    to use for band lists in memory.
#	FILE_IMPLEMENTATION - normally stdio; if set to fd, uses file
#	    descriptors instead of buffered stdio for file I/O; if set to
#	    both, provides both implementations with different procedure
#	    names for the fd-based implementation (see sfxfd.c for
#	    more information).
#	STDIO_IMPLEMENTATION - normally 'c' which uses callouts and 
#	    ziodevsc.c, but ghostscript library must use '' for file 
#	    based stdio in ziodevs.c. 
#           Callouts use procedure based streams and return back to
#           to gs_main_interpret() in imain.c whenever stdio is needed.
#	EXTEND_NAMES - a value N between 0 and 6, indicating that the name
#	    table should have a capacity of 2^(16+N) names.  This normally
#	    should be set to 0 (or left undefined), since non-zero values
#	    result in a larger fixed space overhead and slightly slower code.
#	SYSTEM_CONSTANTS_ARE_WRITABLE - normally 0 (or undefined); if set to
#	    1, makes the system configuration constants (buildtime, copyright,
#	    product, revision, revisiondate, serialnumber) writable.  Only
#	    one unusual application needs this.
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
#	O_ - the string for specifying the output file from the C compiler
#		(-o for MS-DOS, -o ./ for Unix).
#	OBJ - the extension for relocatable object files (e.g., o or obj).
#	XE - the extension for executable files (e.g., null or .exe).
#	XEAUX - the extension for the executable files (e.g., null or .exe)
#		for the utility programs (ansi2knr and those compiled with
#		CCAUX).
#	BEGINFILES - the list of additional files that `make clean' should
#		delete.
#	CCA2K - the C invocation for the ansi2knr program, which is the only
#		one that doesn't use ANSI C syntax.  (It is only needed if
#		the main C compiler also isn't an ANSI compiler.)
#	CCAUX - the C invocation for auxiliary programs (echogs, genarch,
#		genconf, gendev, genht, geninit).
#	CC_ - the C invocation for normal compilation.
#	CCD - the C invocation for files that store into frame buffers or
#		device registers.  Needed because some optimizing compilers
#		will eliminate necessary stores.
#	CCINT - the C invocation for compiling the main interpreter module,
#		normally the same as CC_: this is needed because the
#		Borland compiler generates *worse* code for this module
#		(but only this module) when optimization (-O) is turned on.
#	CCLEAF - the C invocation for compiling modules that contain only
#		leaf procedures, which don't need to build stack frames.
#		This is needed only because many compilers aren't able to
#		recognize leaf procedures on their own.
#	AK - if source files must be converted from ANSI to K&R syntax,
#		this is $(ANSI2KNR_XE); if not, it is null.
#		If a particular platform requires other utility programs
#		to be built, AK must include them too.
#	EXP - the prefix for invoking an executable program in a specified
#		directory (MCR on OpenVMS, null on all other platforms).
#	SH - the shell for scripts (null on MS-DOS, sh on Unix).
#	CONFILES - the arguments for genconf to generate the appropriate
#		linker control files (various).
#	CONFLDTR - the genconf switch for generating ld_tr.
#	CP_ - the command for copying one file to another.  Because of
#		limitations in the MS-DOS/MS Windows environment, the
#		second argument must be either '.' (in which case the
#		write date may be either preserved or set to the current
#		date) or a file name (in which case the write date is
#		always updated).
#	RM_ - the command for deleting (a) file(s) (including wild cards,
#		but limited to a single file or pattern).
#	RMN_ = the command for deleting multiple files / patterns.
#
# The platform-specific makefiles must also include rules for creating
# certain dynamically generated files:
#	gconfig_.h - this indicates the presence or absence of
#	    certain system header files that are located in different
#	    places on different systems.  (It could be generated by
#	    the GNU `configure' program.)
#	gconfigv.h - this indicates the status of certain machine-
#	    and configuration-specific features derived from definitions
#	    in the platform-specific makefile.

#**************** PATCHES
JGENDIR=$(GLGENDIR)
JOBJDIR=$(GLOBJDIR)
PNGGENDIR=$(GLGENDIR)
PNGOBJDIR=$(GLOBJDIR)
ZGENDIR=$(GLGENDIR)
ZOBJDIR=$(GLOBJDIR)
ICCGENDIR=$(GLGENDIR)
ICCOBJDIR=$(GLOBJDIR)
IJSGENDIR=$(GLGENDIR)
IJSOBJDIR=$(GLOBJDIR)
#**************** END PATCHES

GSGEN=$(GLGENDIR)$(D)
GSOBJ=$(GLOBJDIR)$(D)
# All top-level makefiles define DD.
#DD=$(GLGEN)

# Define the name of this makefile.
GS_MAK=$(GLSRCDIR)$(D)gs.mak

# Define the names of the executables.
GS_XE=$(BINDIR)$(D)$(GS)$(XE)
AUXGENDIR=$(GLGENDIR)
AUXGEN=$(AUXGENDIR)$(D)
ANSI2KNR_XE=$(AUXGEN)ansi2knr$(XEAUX)
ECHOGS_XE=$(AUXGEN)echogs$(XEAUX)
GENARCH_XE=$(AUXGEN)genarch$(XEAUX)
GENCONF_XE=$(AUXGEN)genconf$(XEAUX)
GENDEV_XE=$(AUXGEN)gendev$(XEAUX)
GENHT_XE=$(AUXGEN)genht$(XEAUX)
GENINIT_XE=$(AUXGEN)geninit$(XEAUX)

# Define the names of the generated header files.
# gconfig*.h and gconfx*.h are generated dynamically.
gconfig_h=$(GLGENDIR)$(D)gconfxx.h
gconfigf_h=$(GLGENDIR)$(D)gconfxc.h

all default : $(GS_XE)
	$(NO_OP)

#****** ON UNIX PLATFORMS, SHOULD REMOVE `makefile' ******
distclean maintainer-clean realclean : clean
	$(NO_OP)

clean : mostlyclean
	$(RM_) $(GSGEN)arch.h
	$(RM_) $(GS_XE)

#****** FOLLOWING IS WRONG, NEEDS TO BE PER-SUBSYSTEM ******
mostlyclean : config-clean
	$(RMN_) $(GSOBJ)*.$(OBJ) $(GSOBJ)*.a $(GSOBJ)core $(GSOBJ)gmon.out
	$(RMN_) $(GSGEN)deflate.h $(GSGEN)zutil.h
	$(RMN_) $(GSGEN)gconfig*.c $(GSGEN)gscdefs*.c $(GSGEN)iconfig*.c
	$(RMN_) $(GSGEN)_temp_* $(GSGEN)_temp_*.* $(GSOBJ)*.map $(GSOBJ)*.sym
	$(RMN_) $(ANSI2KNR_XE) $(ECHOGS_XE)
	$(RMN_) $(GENARCH_XE) $(GENCONF_XE) $(GENDEV_XE) $(GENHT_XE) $(GENINIT_XE)
	$(RMN_) $(GSGEN)gs_init.c $(BEGINFILES)

# Remove only configuration-dependent information.
#****** FOLLOWING IS WRONG, NEEDS TO BE PER-SUBSYSTEM ******
config-clean :
	$(RMN_) $(GSGEN)*.dev $(GSGEN)devs*.tr $(GSGEN)gconfig*.h
	$(RMN_) $(GSGEN)gconfx*.h $(GSGEN)j*.h
	$(RMN_) $(GSGEN)c*.tr $(GSGEN)o*.tr $(GSGEN)l*.tr

# A rule to do a quick and dirty compilation attempt when first installing
# the interpreter.  Many of the compilations will fail:
# follow this with 'make'.

#****** FOLLOWING IS WRONG, TIED TO INTERPRETER ******
begin :
	$(RMN_) $(GSGEN)arch.h $(GSGEN)gconfig*.h $(GSGEN)gconfx*.h
	$(RMN_) $(GENARCH_XE) $(GS_XE)
	$(RMN_) $(GSGEN)gconfig*.c $(GSGEN)gscdefs*.c $(GSGEN)iconfig*.c
	$(RMN_) $(GSGEN)gs_init.c $(BEGINFILES)
	make $(GSGEN)arch.h $(GSGEN)gconfigv.h
	- $(CCBEGIN)
	$(RMN_) $(GSOBJ)gconfig.$(OBJ) $(GSOBJ)gdev*.$(OBJ)
	$(RMN_) $(GSOBJ)gp_*.$(OBJ) $(GSOBJ)gscdefs.$(OBJ) $(GSOBJ)gsmisc.$(OBJ)
	$(RMN_) $(PSOBJ)icfontab.$(OBJ) $(PSOBJ)iconfig.$(OBJ)
	$(RMN_) $(PSOBJ)iinit.$(OBJ) $(PSOBJ)interp.$(OBJ)

# Macros for constructing the *.dev files that describe features and
# devices.
SETDEV=$(EXP)$(ECHOGS_XE) -e .dev -w- -l-dev -b -s -l-obj
SETPDEV=$(EXP)$(ECHOGS_XE) -e .dev -w- -l-dev -b -s -l-include -l$(GLGENDIR)$(D)page -l-obj
SETDEV2=$(EXP)$(ECHOGS_XE) -e .dev -w- -l-dev2 -b -s -l-obj
SETPDEV2=$(EXP)$(ECHOGS_XE) -e .dev -w- -l-dev2 -b -s -l-include -l$(GLGENDIR)$(D)page -l-obj
SETMOD=$(EXP)$(ECHOGS_XE) -e .dev -w- -l-obj
ADDMOD=$(EXP)$(ECHOGS_XE) -e .dev -a- $(NULL)

# Define the search lists and compilation switches for the third-party
# libraries, and the compilation switches for their clients.
# The search lists must be enclosed in $(I_) and $(_I).
# Note that we can't define the entire compilation command,
# because this must include $(GLSRCDIR), which isn't defined yet.
ICCI_=$(ICCSRCDIR)
ICCF_=
# Currently there is no option for sharing icclib.
ICCCF_=
IJSI_=$(IJSSRCDIR)
IJSF_=
# Currently there is no option for sharing ijs.
IJSCF_=
JI_=$(JSRCDIR)
JF_=
JCF_=$(D_)SHARE_JPEG=$(SHARE_JPEG)$(_D)
PI_=$(PSRCDIR) $(II)$(ZSRCDIR)
# PF_ should include PNG_USE_CONST, but this doesn't work.
#PF_=-DPNG_USE_CONST
PF_=
PCF_=$(D_)SHARE_LIBPNG=$(SHARE_LIBPNG)$(_D)
ZI_=$(ZSRCDIR)
ZF_=
ZCF_=$(D_)SHARE_ZLIB=$(SHARE_ZLIB)$(_D)

######################## How to define new 'features' #######################
#
# One defines new 'features' exactly like devices (see devs.mak for details).
# For example, one would define a feature abc by adding the following to
# gs.mak:
#
#	abc_=abc1.$(OBJ) ...
#	$(PSD)abc.dev : $(GS_MAK) $(ECHOGS_XE) $(abc_)
#		$(SETMOD) $(PSD)abc $(abc_)
#		$(ADDMOD) $(PSD)abc -obj ... [if needed]
#		$(ADDMOD) $(PSD)abc -oper ... [if appropriate]
#		$(ADDMOD) $(PSD)abc -ps ... [if appropriate]
#
# Use PSD for interpreter-related features, GLD for library-related.
# If the abc feature requires the presence of some other features jkl and
# pqr, then the rules must look like this:
#
#	abc_=abc1.$(OBJ) ...
#	$(PSD)abc.dev : $(GS_MAK) $(ECHOGS_XE) $(abc_)\
#	 $(PSD)jkl.dev $(PSD)pqr.dev
#		$(SETMOD) $(PSD)abc $(abc_)
#		...
#		$(ADDMOD) $(PSD)abc -include $(PSD)jkl
#		$(ADDMOD) $(PSD)abc -include $(PSD)pqr

# --------------------- Configuration-dependent files --------------------- #

# gconfig.h shouldn't have to depend on DEVS_ALL, but that would
# involve rewriting gsconfig to only save the device name, not the
# contents of the <device>.dev files.
# FEATURE_DEVS must precede DEVICE_DEVS so that devices can override
# features in obscure cases.

# FEATURE_DEVS_EXTRA and DEVICE_DEVS_EXTRA are explicitly reserved
# to be set from the command line.
FEATURE_DEVS_EXTRA=
DEVICE_DEVS_EXTRA=

DEVS_ALL=$(GLGENDIR)$(D)$(PLATFORM).dev\
 $(FEATURE_DEVS) $(FEATURE_DEVS_EXTRA) \
 $(DEVICE_DEVS) $(DEVICE_DEVS1) \
 $(DEVICE_DEVS2) $(DEVICE_DEVS3) $(DEVICE_DEVS4) $(DEVICE_DEVS5) \
 $(DEVICE_DEVS6) $(DEVICE_DEVS7) $(DEVICE_DEVS8) $(DEVICE_DEVS9) \
 $(DEVICE_DEVS10) $(DEVICE_DEVS11) $(DEVICE_DEVS12) $(DEVICE_DEVS13) \
 $(DEVICE_DEVS14) $(DEVICE_DEVS15) $(DEVICE_DEVS16) $(DEVICE_DEVS17) \
 $(DEVICE_DEVS18) $(DEVICE_DEVS19) $(DEVICE_DEVS20) $(DEVICE_DEVS_EXTRA)

devs_tr=$(GLGENDIR)$(D)devs.tr
$(devs_tr) : $(GS_MAK) $(TOP_MAKEFILES) $(ECHOGS_XE)
	$(EXP)$(ECHOGS_XE) -w $(devs_tr) - -include $(GLGENDIR)$(D)$(PLATFORM)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(FEATURE_DEVS)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(FEATURE_DEVS_EXTRA)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS1)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS2)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS3)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS4)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS5)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS6)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS7)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS8)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS9)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS10)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS11)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS12)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS13)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS14)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS15)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS16)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS17)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS18)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS19)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS20)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) -+ $(DEVICE_DEVS_EXTRA)
	$(EXP)$(ECHOGS_XE) -a $(devs_tr) - $(GLGENDIR)$(D)libcore

# GCONFIG_EXTRAS can be set on the command line.
# Note that it consists of arguments for echogs, i.e.,
# it isn't just literal text.
GCONFIG_EXTRAS=

ld_tr=$(GLGENDIR)$(D)ld.tr
$(gconfig_h) $(ld_tr) $(GLGENDIR)$(D)lib.tr : \
  $(GS_MAK) $(TOP_MAKEFILES) $(GLSRCDIR)$(D)version.mak $(GENCONF_XE) $(ECHOGS_XE) $(devs_tr) $(DEVS_ALL) $(GLGENDIR)$(D)libcore.dev
	$(EXP)$(GENCONF_XE) $(devs_tr) -h $(gconfig_h) $(CONFILES) $(CONFLDTR) $(ld_tr)
	$(EXP)$(ECHOGS_XE) -a $(gconfig_h) -x 23 define -s -u GS_LIB_DEFAULT -x 2022 $(GS_LIB_DEFAULT) -x 22
	$(EXP)$(ECHOGS_XE) -a $(gconfig_h) -x 23 define -s -u SEARCH_HERE_FIRST -s $(SEARCH_HERE_FIRST)
	$(EXP)$(ECHOGS_XE) -a $(gconfig_h) -x 23 define -s -u GS_DOCDIR -x 2022 $(GS_DOCDIR) -x 22
	$(EXP)$(ECHOGS_XE) -a $(gconfig_h) -x 23 define -s -u GS_INIT -x 2022 $(GS_INIT) -x 22
	$(EXP)$(ECHOGS_XE) -a $(gconfig_h) -x 23 define -s -u GS_REVISION -s $(GS_REVISION)
	$(EXP)$(ECHOGS_XE) -a $(gconfig_h) -x 23 define -s -u GS_REVISIONDATE -s $(GS_REVISIONDATE)
	$(EXP)$(ECHOGS_XE) -a $(gconfig_h) $(GCONFIG_EXTRAS)
