#! /bin/sh
# $Id: bughunt.sh,v 1.1 2000/03/09 08:40:40 lpd Exp $
# NB: If your sh does not support functions, then try
# /usr/local/bin/bash or /bin/ksh, if you have them.
#
# Hunt down compiler bugs that break gs.
#
# Usage:
#	./BUGHUNT "optimization level"
# e.g.
#	./BUGHUNT "-O2"
#
# Start with the code compiled at the lowest optimization level where
# it works, then run this script with suitable compiler options.  The
# script will delete one object file at a time and rebuild gs at
# a higher optimization level.  This should uncover the routine(s)
# that the compiler is generating bad code for.
#
# In order to make this test possible in unattended batch mode,
# ghostscript is run with command-line options that force creation of
# a bitmap file, rather than a window.
#
# The okay subdirectory should contain correct output for each
# of the tests.
#
# [06-Dec-1995]

OBJECTS="	  adler32.o deflate.o gconfig.o gdevabuf.o gdevbit.o \
		  gdevbj10.o gdevcdj.o gdevdflt.o gdevdjet.o \
		  gdevemap.o gdevm1.o gdevm16.o gdevm2.o gdevm24.o \
		  gdevm32.o gdevm4.o gdevm8.o gdevmem.o gdevmpla.o \
		  gdevmrop.o gdevpbm.o gdevpccm.o gdevpcl.o gdevpcx.o \
		  gdevpipe.o gdevpng.o gdevprn.o gdevpsim.o gdevtfax.o \
		  gdevtfnx.o gdevtifs.o gdevx.o gdevxalt.o gdevxini.o \
		  gdevxxf.o gp_nofb.o gp_unifn.o gp_unifs.o gp_unix.o \
		  gs.o gsalloc.o gsbitops.o gsbittab.o gschar.o \
		  gschar0.o gscie.o gscolor.o gscolor1.o gscolor2.o \
		  gscoord.o gscsepr.o gsdevice.o gsdevmem.o gsdparam.o \
		  gsdps1.o gsfont.o gsfont0.o gshsb.o gsht.o gsht1.o \
		  gshtscr.o gsimage.o gsimage0.o gsimage1.o gsimage2.o \
		  gsimage3.o gsimpath.o gsinit.o gsiodev.o gsline.o \
		  gsmain.o gsmatrix.o gsmemory.o gsmisc.o gspaint.o \
		  gsparam.o gspath.o gspath1.o gspcolor.o gsrop.o \
		  gsroptab.o gsstate.o gstype1.o gsutil.o gxacpath.o \
		  gxbcache.o gxccache.o gxccman.o gxcht.o gxclbits.o \
		  gxclfile.o gxclip2.o gxclist.o gxclpath.o gxclread.o \
		  gxcmap.o gxcpath.o gxctable.o gxdcconv.o gxdither.o \
		  gxdraw.o gxfill.o gxhint1.o gxhint2.o gxhint3.o \
		  gxht.o gxpaint.o gxpath.o gxpath2.o gxpcmap.o \
		  gxpcopy.o gxstroke.o ialloc.o ibnum.o iccinit0.o \
		  iconfig.o idebug.o idict.o idparam.o igc.o igcref.o \
		  igcstr.o iinit.o ilocate.o iname.o interp.o iparam.o \
		  ireclaim.o isave.o iscan.o iscanbin.o iscannum.o \
		  iscantab.o istack.o iutil.o iutil2.o jcapimin.o \
		  jcapistd.o jccoefct.o jccolor.o jcdctmgr.o jchuff.o \
		  jcinit.o jcmainct.o jcmarker.o jcmaster.o jcomapi.o \
		  jcparam.o jcprepct.o jcsample.o jdapimin.o \
		  jdapistd.o jdcoefct.o jdcolor.o jddctmgr.o jdhuff.o \
		  jdinput.o jdmainct.o jdmarker.o jdmaster.o jdphuff.o \
		  jdpostct.o jdsample.o jfdctint.o jidctint.o jerror.o \
		  jmemmgr.o jutils.o png.o pngerror.o pngio.o pngmem.o \
		  pngtrans.o pngwrite.o pngwtran.o pngwutil.o sbcp.o \
		  sbhc.o sbwbs.o scfd.o scfdtab.o scfe.o scfetab.o \
		  sdctc.o sdctd.o sdcte.o seexec.o sfile.o sfilter1.o \
		  sfilter2.o shc.o shcgen.o siscale.o sjpegc.o \
		  sjpegd.o sjpege.o slzwc.o slzwd.o slzwe.o \
		  smtf.o spdiff.o srld.o srle.o sstring.o stream.o \
		  trees.o zarith.o zarray.o zbseq.o zchar.o zchar1.o \
		  zchar2.o zcie.o zcolor.o zcolor1.o zcolor2.o \
		  zcontrol.o zcrd.o zcsindex.o zcssepr.o zdevcal.o \
		  zdevice.o zdevice2.o zdict.o zdps1.o zfbcp.o \
		  zfdctc.o zfdctd.o zfdcte.o zfdecode.o zfile.o \
		  zfileio.o zfilter.o zfilter2.o zfilterx.o zfname.o \
		  zfont.o zfont0.o zfont1.o zfont2.o zfproc.o \
		  zgeneric.o zgstate.o zhsb.o zht.o zht1.o zht2.o \
		  zimage2.o ziodev.o ziodev2.o zmath.o zmatrix.o \
		  zmedia2.o zmisc.o zmisc1.o zmisc2.o zpacked.o \
		  zpaint.o zpath.o zpath1.o zpcolor.o zrelbit.o \
		  zstack.o zstring.o zsysvm.o ztoken.o ztype.o \
		  zupath.o zutil.o zvmem.o zvmem2.o zwppm.o"

TESTS="exepsf tiger"

dotest()
{
	# Create empty output file, so even if gs core dumps,
	# we will have something to compare against.
	touch $1.ljp
	./gs -sDEVICE=ljetplus -r75x75 -sOutputFile=$1.ljp \
		$1.ps quit.ps < /dev/null
	if cmp $1.ljp okay/$1.ljp
	then
		/bin/true
	else
		echo COMPARISON FAILURE: $1.ljp okay/$1.ljp
		echo "Remaking $f and gs with lower optimization"
		/bin/rm -f $f ./gs
		make $f gs
	fi
}

for f in $OBJECTS
do
	echo ==================== $f ====================

	date

	# Remove the old (good) object file and ghostscript
	/bin/rm -f $f gs

	# Rebuild gs with optimization; only one object file should be
	# recreated.
	make gs CC="cc $1"

	# Now check this new version of gs with each test file.
	for t in $TESTS
	do
		dotest $t
	done
done
