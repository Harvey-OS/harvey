dnl Copyright (C) 2002-2004 artofcode LLC. All rights reserved.
  
dnl This software is provided AS-IS with no warranty, either express or
dnl implied.
  
dnl This software is distributed under license and may not be copied,
dnl modified or distributed except as expressly authorized under the terms
dnl of the license contained in the file LICENSE in this distribution.
  
dnl For more information about licensing, please refer to
dnl http://www.ghostscript.com/licensing/. For information on
dnl commercial licensing, go to http://www.artifex.com/licensing/ or
dnl contact Artifex Software, Inc., 101 Lucas Valley Road #110,
dnl San Rafael, CA  94903, U.S.A., +1(415)492-9861.

dnl $Id: configure.ac,v 1.51 2005/07/15 05:23:42 giles Exp $

dnl Process this file with autoconf to produce a configure script

dnl ------------------------------------------------
dnl Initialization and Versioning
dnl ------------------------------------------------

AC_INIT
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(src/gs.c)

dnl --------------------------------------------------  
dnl Check for programs
dnl --------------------------------------------------  

dnl AC_PROG_CC likes to add '-g -O2' to CFLAGS. however,
dnl we ignore those flags and construct our own.
save_cflags=$CFLAGS
AC_PROG_CC
AC_PROG_CPP
CFLAGS=$save_cflags

AC_PROG_RANLIB
#AC_PROG_INSTALL

dnl --------------------------------------------------
dnl Set build flags based on environment
dnl --------------------------------------------------

#AC_CANONICAL_HOST

if test $ac_cv_prog_gcc = yes; then
    cflags_to_try="-Wall -Wstrict-prototypes \
-Wmissing-declarations -Wmissing-prototypes \
-Wcast-qual -Wwrite-strings \
-fno-builtin -fno-common"
    optflags_to_try="-O2"
else
    cflags_to_try=
    optflags_to_try="-O"
fi

AC_MSG_CHECKING([supported compiler flags])
old_cflags=$CFLAGS
echo
for flag in $optflags_to_try; do
    CFLAGS="$CFLAGS $flag"
    AC_TRY_COMPILE(, [return 0;], [
	echo "   $flag"
	OPT_CFLAGS="$OPT_CFLAGS $flag"
    ])
    CFLAGS=$old_cflags
done
for flag in $cflags_to_try; do
	CFLAGS="$CFLAGS $flag"
	AC_TRY_COMPILE(, [return 0;], [
		echo "   $flag"
		GCFLAGS="$GCFLAGS $flag"
	])
	CFLAGS=$old_cflags
done
AC_MSG_RESULT([ ...done.])

dnl --------------------------------------------------
dnl Check for headers
dnl --------------------------------------------------

AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h fcntl.h limits.h malloc.h memory.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/time.h syslog.h unistd.h])

# for gdev3b1.c (AT&T terminal interface)
AC_CHECK_HEADER([sys/window.h])

dnl --------------------------------------------------
dnl Check for typedefs, structures, etc
dnl --------------------------------------------------

AC_C_CONST
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_STRUCT_ST_BLOCKS
AC_HEADER_TIME
AC_STRUCT_TM

dnl see if we're on a system that puts the *int*_t types 
dnl from stdint.h in sys/types.h
if test "x$ac_cv_header_stdint_h" != xyes; then
    AC_CHECK_TYPES([int8_t, int16_t, int32_t, uint8_t, uint16_t, uint32_t],,,[#include <sys/types.h>])
    if test "$ac_cv_type_uint8_t" = yes; then
        AC_DEFINE([SYS_TYPES_HAS_STDINT_TYPES])
	GCFLAGS="$GCFLAGS -DSYS_TYPES_HAS_STDINT_TYPES"
    fi
fi

dnl we aren't interested in all of DEFS, so manually insert
dnl the flags we care about
if test "$ac_cv_c_const" != yes; then
	GCFLAGS="$GCFLAGS -Dconst="
fi
if test "x$ac_cv_header_stdint_h" = xyes; then
	GCFLAGS="$GCFLAGS -DHAVE_STDINT_H"
fi

dnl try to find a 64 bit type for devicen color index
uint64_type="none"
 AC_CHECK_SIZEOF(unsigned long int)
 if test $ac_cv_sizeof_unsigned_long_int = 8; then
	uint64_type="unsigned long int"
 else
  AC_CHECK_SIZEOF(unsigned long long)
  if test $ac_cv_sizeof_unsigned_long_long = 8; then
	uint64_type="unsigned long long"
  else
   AC_CHECK_SIZEOF(unsigned __int64)
   if test $ac_cv_sizeof_unsigned___int64 = 8; then
	uint64_type="unsigned __int64"
   else
    AC_CHECK_SIZEOF(u_int64_t)
    if test $ac_cv_sizeof_u_int64_t = 8; then
	uint64_type="u_int64_t"
    fi
   fi
  fi
 fi
dnl we don't need to do anything if a 64-bit type wasn't found
dnl the code falls back to a (probably 32-bit) default
if test "$uint64_type" != "none"; then
	GCFLAGS="$GCFLAGS -DGX_COLOR_INDEX_TYPE=\"$uint64_type\""
fi


dnl --------------------------------------------------
dnl Check for libraries
dnl --------------------------------------------------

AC_CHECK_LIB(m, cos)
dnl AC_CHECK_LIB(pthread, pthread_create)

AC_MSG_CHECKING([for local jpeg library source])
dnl At present, we give the local source priority over the shared
dnl build, so that the D_MAX_BLOCKS_IN_MCU patch will be applied.
dnl A more sophisticated approach would be to test the shared lib
dnl to see whether it has already been patched.
LIBJPEGDIR=src
if test -f jpeg/jpeglib.h; then
	AC_MSG_RESULT([jpeg])
	SHARE_LIBJPEG=0
	LIBJPEGDIR=jpeg
elif test -f jpeg-6b/jpeglib.h; then
	AC_MSG_RESULT([jpeg-6b])
	SHARE_LIBJPEG=0
	LIBJPEGDIR=jpeg-6b
else
	AC_MSG_RESULT([no])
	AC_CHECK_LIB(jpeg, jpeg_set_defaults, [
	  AC_CHECK_HEADERS([jpeglib.h], [SHARE_LIBJPEG=1])
	])
fi
if test -z "$SHARE_LIBJPEG"; then
  AC_MSG_ERROR([I wasn't able to find a copy
  of the jpeg library. This is required for compiling
  ghostscript. Please download a copy of the source,
  e.g. from http://www.ijg.org/, unpack it at the
  top level of the gs source tree, and rename
  the directory to 'jpeg'.
  ])
fi
AC_SUBST(SHARE_LIBJPEG)
AC_SUBST(LIBJPEGDIR)
dnl check for the internal jpeg memory header
AC_MSG_CHECKING([for jmemsys.h])
if test -r $LIBJPEGDIR/jmemsys.h; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
  AC_DEFINE([DONT_HAVE_JMEMSYS_H], 1, 
    [define if the libjpeg memory system prototypes aren't available])
fi

dnl these are technically optional

AC_MSG_CHECKING([for local zlib source])
dnl zlib is needed for language level 3, and libpng
# we must define ZLIBDIR regardless because libpng.mak does a -I$(ZLIBDIR)
# this seems a harmless default
ZLIBDIR=src
if test -d zlib; then
	AC_MSG_RESULT([yes])
	SHARE_ZLIB=0
	ZLIBDIR=zlib
else
	AC_MSG_RESULT([no])
	AC_CHECK_LIB(z, deflate, [
	  AC_CHECK_HEADERS(zlib.h, [SHARE_ZLIB=1])
	])
fi
if test -z "$SHARE_ZLIB"; then
  AC_MSG_ERROR([I did not find a copy of zlib on your system.
  Please either install it, or unpack a copy of the source in a
  local directory named 'zlib'. See http://www.gzip.org/zlib/
  for more information.
  ])
fi
AC_SUBST(SHARE_ZLIB)
AC_SUBST(ZLIBDIR)

dnl png for the png output device; it also requires zlib
LIBPNGDIR=src
PNGDEVS=''
PNGDEVS_ALL='$(DD)png48.dev $(DD)png16m.dev $(DD)pnggray.dev $(DD)pngmono.dev $(DD)png256.dev $(DD)png16.dev $(DD)pngalpha.dev'
AC_MSG_CHECKING([for local png library source])
if test -f libpng/pngread.c; then
	AC_MSG_RESULT([yes])
	SHARE_LIBPNG=0
	LIBPNGDIR=libpng
	PNGDEVS="$PNGDEVS_ALL"
else
	AC_MSG_RESULT([no])
	AC_CHECK_LIB(png, png_check_sig, [
	  AC_CHECK_HEADERS(png.h, [
		SHARE_LIBPNG=1
		PNGDEVS="$PNGDEVS_ALL"
	  ], [SHARE_LIBPNG=0])
	], [SHARE_LIBPNG=0], [-lz])
fi
if test -z "$PNGDEVS"; then
  AC_MSG_NOTICE([disabling png output devices])
fi
AC_SUBST(SHARE_LIBPNG)
AC_SUBST(LIBPNGDIR)
AC_SUBST(PNGDEVS)

dnl look for IJS implementation
AC_ARG_WITH(ijs, AC_HELP_STRING([--with-ijs],[include IJS driver support]))
dnl set safe defaults
    IJSDIR=src
    IJSDEVS=''
if test x$with_ijs != xno; then
    AC_MSG_CHECKING([for local ijs library source])
    if test -d ijs; then
        AC_MSG_RESULT([yes])
        IJSDIR=ijs
        IJSDEVS='$(DD)ijs.dev'
    else
        AC_MSG_RESULT([no])
    fi
fi
AC_SUBST(IJSDIR)
AC_SUBST(IJSDEVS)

dnl look for jbig2dec
AC_ARG_WITH(jbig2dec, AC_HELP_STRING([--with-jbig2dec],[include JBIG2 decode support]))
JBIG2DIR=src
SHARE_JBIG2=0
JBIG2DEVS=''
if test x$with_jbig2dec != xno; then
  AC_MSG_CHECKING([for local jbig2dec library source])
  for d in jbig2dec jbig2dec-0.2 jbig2dec-0.3; do
    test -d "$d" && JBIG2DIR=$d && break
  done
  if test "x$JBIG2DIR" != xsrc; then
    AC_MSG_RESULT([$JBIG2DIR])
  else
    AC_MSG_RESULT([no])
    AC_CHECK_LIB([jbig2dec], [jbig2_page_out], [
	SHARE_JBIG2=1
    ], [
	AC_MSG_WARN([disabling support for JBIG2 files])
        with_jbig2dec=no
    ])
  fi
fi
if test x$with_jbig2dec != xno; then
  if test x$ac_cv_header_stdint_h != xyes; then
    AC_MSG_WARN([JBIG2 support requires stdint types which do not seem to be available.])
  else
    JBIG2DEVS='$(PSD)jbig2.dev'
  fi
fi

AC_SUBST(JBIG2DIR)
AC_SUBST(SHARE_JBIG2)
AC_SUBST(JBIG2DEVS)

dnl look for the jasper JPEG 2000 library
AC_ARG_WITH(jasper, AC_HELP_STRING([--with-jasper],[link to the JasPer library for JPEG 2000]))
JASPERDIR=src
SHARE_JASPER=0
JPXDEVS=''
if test x$with_jasper != xno; then
  AC_MSG_CHECKING([for local jasper library source])
  for d in jasper jasper-1.7*; do
    test -d "$d" && JASPERDIR=$d && break
  done
  if test "x$JASPERDIR" != xsrc; then
    AC_MSG_RESULT([$JASPERDIR])
    AC_MSG_CHECKING([for local jasper configure script])
    if test -x $JASPERDIR/configure; then
      AC_MSG_RESULT([yes])
      echo
      echo "Running jasper configure script..."
      olddir=`pwd`
      cd $JASPERDIR && ./configure
      cd $olddir
      echo
      echo "Continuing with Ghostscript configuration..."
    else
      AC_MSG_RESULT([no])
      AC_MSG_CHECKING([for local jasper autogen.sh script])
      if test -x $JASPERDIR/autogen.sh; then
        AC_MSG_RESULT([yes])
        echo
        echo "Running jasper autogen script..."
        olddir=`pwd`
        cd $JASPERDIR && ./autogen.sh
        cd $olddir
        echo
        echo "Continuing with Ghostscript configuration..."
      else
        AC_MSG_ERROR([
Unable to find $JASPERDIR/src/libjasper/include/jas_config.h,
or generate generate such a file for this system. You will
have to build one by hand, or configure, build and install
the jasper library separately.

You can also pass --without-jasper to configure to disable
JPEG 2000 PDF image support entirely.
])
      fi
    fi
  else
    AC_MSG_RESULT([no])
    AC_CHECK_LIB([jasper], [jas_image_create], [
	SHARE_JASPER=1
    ], [
	AC_MSG_WARN([disabling support for JPEG 2000 images])
        with_jasper=no
    ])
  fi
fi
if test x$with_jasper != xno; then
  JPXDEVS='$(PSD)jpx.dev'
fi

AC_SUBST(JASPERDIR)
AC_SUBST(SHARE_JASPER)
AC_SUBST(JPXDEVS)

dnl optional X11 for display devices
AC_PATH_XTRA
XLIBS=''
if test x"$no_x" = x"yes"; then
	AC_MSG_NOTICE([disabling X11 output devices])
	X11DEVS=''
else
	# enable X11 output devices
	X11DEVS='$(DD)x11.dev $(DD)x11alpha.dev $(DD)x11cmyk.dev $(DD)x11gray2.dev $(DD)x11gray4.dev $(DD)x11mono.dev'
	# the makefile wants a list of just the library names in XLIBS
	for item in -lXt $X_PRE_LIBS -lXext -lX11 $X_EXTRA_LIBS; do
		stripped=`echo "$item" | sed -e 's/-l//'`
		if test "x$stripped" != "x$item"; then
			XLIBS="$XLIBS $stripped"
		fi
	done
fi
AC_SUBST(X11DEVS)
AC_SUBST(XLIBS)

dnl executible name
AC_ARG_WITH(gs, AC_HELP_STRING([--with-gs=NAME],
	[name of the ghostscript executible [[gs]]]),
	[GS="$with_gs"],[GS='gs'])
AC_SUBST(GS)

dnl do we compile the postscript initialization files into Ghostscript?
COMPILE_INITS="0"
AC_ARG_ENABLE(compile-inits, AC_HELP_STRING([--enable-compile-inits],
       [compile initialization files into Ghostscript]),[
               if test x$enable_compile_inits = xyes; then
                       COMPILE_INITS="1"
               fi])
AC_SUBST(COMPILE_INITS)

dnl --------------------------------------------------
dnl Check for library functions
dnl --------------------------------------------------

AC_CHECK_FUNCS([mkstemp], [HAVE_MKSTEMP=-DHAVE_MKSTEMP])
AC_SUBST(HAVE_MKSTEMP)

AC_CHECK_FUNCS([hypot], [HAVE_HYPOT=-DHAVE_HYPOT])
AC_SUBST(HAVE_HYPOT)

AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero dup2 floor gettimeofday memchr memmove memset mkdir mkfifo modf pow putenv rint setenv sqrt strchr strerror strrchr strspn strstr])

dnl --------------------------------------------------
dnl Do substitutions
dnl --------------------------------------------------

AC_SUBST(OPT_CFLAGS)
AC_SUBST(GCFLAGS)
AC_OUTPUT(Makefile)
