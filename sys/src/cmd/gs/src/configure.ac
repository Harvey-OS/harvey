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

dnl save $CFLAGS since AC_PROG_CC likes to insert "-g -O2"
dnl if $CFLAGS is blank
dnl cflags_save="$CFLAGS"
AC_PROG_CC
AC_PROG_CPP
dnl CFLAGS="$cflags_save"

AC_PROG_RANLIB
#AC_PROG_INSTALL

dnl --------------------------------------------------
dnl Set build flags based on environment
dnl --------------------------------------------------

#AC_CANONICAL_HOST

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

dnl --------------------------------------------------
dnl Check for libraries
dnl --------------------------------------------------

AC_CHECK_LIB(m, cos)
dnl AC_CHECK_LIB(pthread, pthread_create)

dnl we need to check for the blocknum patch in the shared
dnl libjpeg. in the meantime, always require local source
AC_CHECK_LIB(jpeg, jpeg_set_defaults)
AC_MSG_CHECKING([for local jpeg library source])
if test -d jpeg; then
	AC_MSG_RESULT([yes])
	JPEGDIR=jpeg
else
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([I wasn't able to find a local copy
  of the jpeg library source. This is required for
  compiling ghostscript. Please download a copy,
  e.g. from http://www.ijg.org/, unpack it at the
  top level of the gs source tree, and rename
  the directory to 'jpeg'.
])
fi
AC_SUBST(JPEGDIR)

dnl these are technically optional
dnl zlib is needed for language level 3, and libpng
# we must define ZLIBDIR regardless because libpng.mak does a -I$(ZLIBDIR)
# this seems a harmless default
ZLIBDIR=src
AC_CHECK_LIB(z, deflate, [SHARE_ZLIB=1], [
	SHARE_ZLIB=0
	AC_MSG_CHECKING([for local zlib source])
	if test -d zlib; then
		AC_MSG_RESULT([yes])
		ZLIBDIR=zlib
	else
		AC_MSG_ERROR([I did not find a copy of libz on your system.
Please either install it, or unpack a copy of the source in a
local directory named 'zlib'.
])
	fi
])
AC_SUBST(SHARE_ZLIB)
AC_SUBST(ZLIBDIR)

dnl png for the png output device; it also requires zlib
AC_CHECK_LIB(png, png_check_sig, [
	SHARE_LIBPNG=1
	PNGDEVS='$(DD)png16m.dev $(DD)pnggray.dev $(DD)pngmono.dev $(DD)png256.dev $(DD)png16.dev'
], [
	SHARE_LIBPNG=0
	AC_MSG_CHECKING([for local png library source])
	if test -d libpng; then
		AC_MSG_RESULT([yes])
		PNGDIR=libpng
		PNGDEVS='$(DD)png16m.dev $(DD)pnggray.dev $(DD)pngmono.dev $(DD)png256.dev $(DD)png16.dev'
	else
		AC_MSG_RESULT([no])
		AC_MSG_NOTICE([disabling png output devices])
		PNGDEVS=''
	fi
])
AC_SUBST(SHARE_LIBPNG)
AC_SUBST(PNGDIR)
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

dnl optional X11 for display devices
AC_PATH_X
if test x"$no_x" = x"yes"; then
	AC_MSG_NOTICE([disabling X11 output devices])
	X11DEVS=''
else
	X11DEVS='$(DD)x11.dev $(DD)x11alpha.dev $(DD)x11cmyk.dev $(DD)x11gray2.dev $(DD)x11gray4.dev $(DD)x11mono.dev'
fi
AC_SUBST(X11DEVS)
AC_SUBST(x_includes)
AC_SUBST(x_libraries)


dnl --------------------------------------------------
dnl Check for library functions
dnl --------------------------------------------------

AC_CHECK_FUNCS([mkstemp], [HAVE_MKSTEMP=-DHAVE_MKSTEMP])
AC_SUBST(HAVE_MKSTEMP)

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

AC_OUTPUT(Makefile)
