/* Master configuration file for WEB to C.  Almost all the definitions
   are wrapped with #ifndef's, so that you can override them from the
   command line, if you want to.  */

#ifndef __WEB2C_SITE_H
#define __WEB2C_SITE_H


/* Define if you're running under 4.2 or 4.3 BSD.  */
#ifndef BSD
#undef	BSD
#endif

/* Define if you're running under System V.  */
#ifndef SYSV
#undef	SYSV
#endif

/* Define if you're running under MS-DOS with Microsoft C.  */
#ifndef MS_DOS
#undef	MS_DOS
#endif

/* Define if you're running under a POSIX conforming OS  */
#ifndef _POSIX_SOURCE
#define	_POSIX_SOURCE
#endif

/* Define this if the system will be compiled with an ANSI C compiler,
   and never with a non-ANSI compiler.  It changes web2c so that it
   produces ANSI C as its output.  This is a good idea, but you don't
   necessarily gain anything in the production programs by doing it.  */
#ifndef ANSI
#define	ANSI
#endif

/* Define these according to your local setup.  */
#define	TEXINPUTS	".:/usr/lib/tex/macros"
#define	TEXFONTS	".:/usr/lib/tex/fonts/tfm"
#define	TEXFORMATS	".:/usr/lib/tex/macros"
#define	TEXPOOL		".:/usr/lib/tex"
#define	MFBASES		".:/usr/lib/mf"
#define	MFINPUTS	".:/usr/lib/mf"
#define	MFPOOL		".:/usr/lib/mf"

/* BibTeX search path for .bib files.  TEXINPUTS is used by BibTeX to
   search for .bst files.  */
#define	BIBINPUTS	".:/usr/lib/tex/macros"

/* Metafont window support: More than one may be defined, as long as you
   don't try to have both X10 and X11 support (because there are
   conflicting routine names in the libraries).  After you've defined
   these, make sure to update the top-level Makefile accordingly.  Also,
   if you want X11 support, see the `Online output from Metafont'
   section in ./README before compiling.  */
#undef	SUNWIN			/* SunWindows support. */
#undef	X10WIN			/* X Version 10 support. */
#undef	X11WIN			/* X Version 11 support. */
#undef	HP2627WIN		/* HP 2627 support. */
#undef	TEKTRONIXWIN		/* Tektronix 4014 support. */

#if defined(X10WIN) && defined(X11WIN)
sorry
#endif

/* Define to be the return type of your signal handlers.  POSIX says it
   should be `void', but some older systems want `int'.  Check your
   <signal.h> include file if you're not sure.  */
#ifndef SIGNAL_HANDLER_RETURN_TYPE
#define SIGNAL_HANDLER_RETURN_TYPE void
#endif

/* The type `glueratio' should be a floating point type which won't
   unnecessarily increase the size of the memoryword structure.  This is
   the basic requirement.  On most machines, if you're building a
   normal-sized TeX, then glueratio must probably meet the following
   restriction: sizeof(glueratio) <= sizeof(integer).  Usually, then,
   glueratio must be `float'.  But if you build a big TeX, you can (on
   most machines) and should make it `double' to avoid loss of precision
   and conversions to and from double during calculations.  (All this
   also goes for Metafont.)  Furthermore, if you have enough memory, it
   won't hurt to have this defined for running the trip/trap tests.  */
typedef double glueratio;

/* Define this if you want TeX to be compiled with local variables
   declared as `register'.  On SunOS 3.2 and 3.4 (at least), compiling
   with cc, this will cause problems.  If you're using gcc or the SunOS
   4.x compiler, and compiling with -O, register declarations are
   ignored, so there is no point in defining this.  */
#ifndef REGFIX
#undef	REGFIX
#endif

/* If the type `int' is at least 32 bits (including a sign bit), this
   symbol should be #undef'd; otherwise, it should be #define'd.  If
   your compiler uses 16-bit int's, arrays larger than 32K may give you
   problems, especially if indices are automatically cast to int's.  */
#ifndef SIXTEENBIT
#undef	SIXTEENBIT
#endif

/* Our character set is 8-bit ASCII unless NONASCII is defined.  For
   other character sets, make sure that first_text_char and
   last_text_char are defined correctly (they're 0 and 255,
   respectively, by default).  In the *.defines files, change the
   indicated range of type `char' to be the same as
   first_text_char..last_text_char, `#define NONASCII', and retangle and
   recompile everything.  */
#ifndef NONASCII
#undef	NONASCII
#endif

/* Default editor command string: %d expands to the line number where
   TeX or Metafont found an error and %s expands to the name of the
   file.  The environment variables TEXEDIT and MFEDIT override this.  */
#define	EDITOR	"/bin/ed %s"

/* The type `schar' should be defined here to be the smallest signed
   type available.  ANSI C compilers may need to use `signed char'.  If
   you don't have signed characters, then define schar to be the type
   `short'.  */
typedef	signed char schar;

/* The type `integer' must be a signed integer capable of holding at
   least the range of numbers (-2^31)..(2^32-1).  The ANSI C
   standard says that `long' meets this requirement, but if you don't
   have an ANSI C compiler, you might have to change this definition.  */
typedef long integer;

/* Define MAXPATHLENGTH to be the maximum number of characters in a
   search path.  This is used to size the buffers for the environment
   variables.  It is good to be quite generous here.  */
#ifndef MAXPATHLENGTH
#define	MAXPATHLENGTH	5000
#endif


/*******************************************************************
  The following definitions are for MetaPost support
*******************************************************************/

/* MetaPost search paths (MetaPost also uses TEXFONTS and MFINPUTS) */
#define	MPINPUTS	".:/usr/lib/mp"
#define	MPMEMS		".:/usr/lib/mp"
#define	MPPOOL		".:/usr/lib/mp"

/* Where dvitomp looks for virtual fonts */
#define VFPATH		"/usr/lib/tex/fonts/psvf"

/*
 * Command for translating MetaPost input to an mpx file
 * (Can be overridden by an environment variable)
 */
#define MPXCOMMAND	"/usr/lib/mp/makempx"

/************ End of MetaPost stuff *******************************/


#include "defaults.h"

#endif /* __WEB2C_H */
