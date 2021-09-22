/* c-auto.h.  Generated automatically by configure.  */
/* c-auto.in.  Generated automatically from configure.in by autoheader.  */
/* acconfig.h -- used by autoheader when generating c-auto.in.

   If you're thinking of editing acconfig.h to fix a configuration
   problem, don't. Edit the c-auto.h file created by configure,
   instead.  Even better, fix configure to give the right answer.  */

/* kpathsea: the version string. */
#define KPSEVERSION "kpathsea version 3.2"
/* web2c: the version string. */
#define WEB2CVERSION "REPLACE-WITH-WEB2CVERSION"

/* kpathsea/configure.in tests for these functions with
   kb_AC_KLIBTOOL_REPLACE_FUNCS, and naturally Autoheader doesn't know
   about that macro.  Since the shared library stuff is all preliminary
   anyway, I decided not to change Autoheader, but rather to hack them
   in here.  */
/* #undef HAVE_BASENAME */
#define HAVE_PUTENV 1
/* #undef HAVE_STRCASECMP */
#define HAVE_STRTOL 1
#define HAVE_STRSTR 1


/* Define if the closedir function returns void instead of int.  */
/* #undef CLOSEDIR_VOID */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you have the ANSI C header files.  */
/* #undef STDC_HEADERS */

/* Define if your compiler understands prototypes.  */
#define HAVE_PROTOTYPES 1

/* Define if your putenv doesn't waste space when the same environment
   variable is assigned more than once, with different (malloced)
   values.  This is true only on NetBSD/FreeBSD, as far as I know. See
   xputenv.c.  */
/* #undef SMART_PUTENV */

/* Define if getcwd if implemented using fork or vfork.  Let me know
   if you have to add this by hand because configure failed to detect
   it. */
/* #undef GETCWD_FORKS */

/* Define if you are using GNU libc or otherwise have global variables
   `program_invocation_name' and `program_invocation_short_name'.  */
/* #undef HAVE_PROGRAM_INVOCATION_NAME */

/* all: Define to enable running scripts when missing input files.  */
#define MAKE_TEX_MF_BY_DEFAULT 1
#define MAKE_TEX_PK_BY_DEFAULT 1
#define MAKE_TEX_TEX_BY_DEFAULT 0
#define MAKE_TEX_TFM_BY_DEFAULT 1
#define MAKE_OMEGA_OFM_BY_DEFAULT 1
#define MAKE_OMEGA_OCP_BY_DEFAULT 1

/* Define if you have the basename function.  */
/* #undef HAVE_BASENAME */

/* Define if you have the bcopy function.  */
/* #undef HAVE_BCOPY */

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the getwd function.  */
/* #undef HAVE_GETWD */

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the strcasecmp function.  */
/* #undef HAVE_STRCASECMP */

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the strtol function.  */
#define HAVE_STRTOL 1

/* Define if you have the <assert.h> header file.  */
#define HAVE_ASSERT_H 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <float.h> header file.  */
#define HAVE_FLOAT_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 0

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <pwd.h> header file.  */
#define HAVE_PWD_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1
