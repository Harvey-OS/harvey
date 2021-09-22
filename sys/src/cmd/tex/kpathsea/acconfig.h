/* acconfig.h -- used by autoheader when generating c-auto.in.

   If you're thinking of editing acconfig.h to fix a configuration
   problem, don't. Edit the c-auto.h file created by configure,
   instead.  Even better, fix configure to give the right answer.  */

/* kpathsea: the version string. */
#define KPSEVERSION "REPLACE-WITH-KPSEVERSION"
/* web2c: the version string. */
#define WEB2CVERSION "REPLACE-WITH-WEB2CVERSION"

/* kpathsea/configure.in tests for these functions with
   kb_AC_KLIBTOOL_REPLACE_FUNCS, and naturally Autoheader doesn't know
   about that macro.  Since the shared library stuff is all preliminary
   anyway, I decided not to change Autoheader, but rather to hack them
   in here.  */
#undef HAVE_BASENAME
#undef HAVE_PUTENV
#undef HAVE_STRCASECMP
#undef HAVE_STRTOL
#undef HAVE_STRSTR

@TOP@

/* Define if your compiler understands prototypes.  */
#undef HAVE_PROTOTYPES

/* Define if your putenv doesn't waste space when the same environment
   variable is assigned more than once, with different (malloced)
   values.  This is true only on NetBSD/FreeBSD, as far as I know. See
   xputenv.c.  */
#undef SMART_PUTENV

/* Define if getcwd if implemented using fork or vfork.  Let me know
   if you have to add this by hand because configure failed to detect
   it. */
#undef GETCWD_FORKS

/* Define if you are using GNU libc or otherwise have global variables
   `program_invocation_name' and `program_invocation_short_name'.  */
#undef HAVE_PROGRAM_INVOCATION_NAME

/* Define if you get clashes concerning wchar_t, between X's include
   files and system includes.  */
#undef FOIL_X_WCHAR_T

/* tex: Define to enable --ipc.  */
#undef IPC

/* all: Define to enable running scripts when missing input files.  */
#define MAKE_TEX_MF_BY_DEFAULT 0
#define MAKE_TEX_PK_BY_DEFAULT 0
#define MAKE_TEX_TEX_BY_DEFAULT 0
#define MAKE_TEX_TFM_BY_DEFAULT 0
#define MAKE_OMEGA_OFM_BY_DEFAULT 0
#define MAKE_OMEGA_OCP_BY_DEFAULT 0

/* web2c: Define if gcc asm needs _ on external symbols.  */
#undef ASM_NEEDS_UNDERSCORE

/* web2c: Define when using system-specific files for arithmetic.  */
#undef ASM_SCALED_FRACTION

/* web2c: Define to enable HackyInputFileNameForCoreDump.tex.  */
#undef FUNNY_CORE_DUMP

/* web2c: Define to disable architecture-independent dump files.
   Faster on LittleEndian architectures.  */
#undef NO_DUMP_SHARE

/* web2c: Default editor for interactive `e' option. */
#define EDITOR "vi +%d %s"

/* web2c: Window system support for Metafont. */
#undef EPSFWIN
#undef HP2627WIN
#undef MFTALKWIN
#undef NEXTWIN
#undef REGISWIN
#undef SUNWIN
#undef TEKTRONIXWIN
#undef UNITERMWIN
#undef X11WIN

/* xdvik: Define if you have SIGIO, F_SETOWN, and FASYNC.  */
#undef HAVE_SIGIO

/* xdvik: Define to avoid using any toolkit (and consequently omit lots
   of features).  */
#undef NOTOOL
