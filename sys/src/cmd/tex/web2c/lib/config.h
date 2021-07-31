/* config.h: Master configuration file.  This is included by common.h,
   which everyone includes.  */

#ifndef CONFIG_H
#define CONFIG_H

/* Site-specific preferences, hand-edited by the installer.  */
#include "site.h"

/* System dependencies that are figured out by `configure'.  */
#include "c-auto.h"

/* ``Standard'' headers.  */
#include "c-std.h"

/* strchr vs. index, memcpy vs. bcopy, etc.
   If `index' is defined, don't include any of this: we are compiling
   one of the programs which uses a variable named `index'.  */
#ifndef index
#include "c-memstr.h"
#endif

/* Error numbers and errno declaration.  */
#include "c-errno.h"

/* Minima and maxima.  */
#include "c-minmax.h"

/* PATH_MAX.  */
#include "c-pathmx.h"

/* The arguments for fseek.  */
#include "c-seek.h"

/* How to open files with fopen.  */
#include "c-fopen.h"

/* Macros to discard or keep prototypes.  */
#include "c-proto.h"


/* The smallest signed type: use `signed char' if ANSI C, `short' if
   char is unsigned, otherwise `char'.  */
#ifndef SCHAR_TYPE
#ifdef __STDC__
#define SCHAR_TYPE signed char
#else /* not __STDC */
#ifdef __CHAR_UNSIGNED__
#define SCHAR_TYPE short
#else
#define SCHAR_TYPE char
#endif
#endif /* not __STDC__ */
#endif /* not SCHAR_TYPE */
typedef SCHAR_TYPE schar;

/* The type `integer' must be a signed integer capable of holding at
   least the range of numbers (-2^31)..(2^31-1).  If your compiler goes
   to great lengths to make programs fail, you might have to change this
   definition.  If this changes, you will probably have to modify
   web2c/fixwrites.c, since it generates code to do integer output using
   "%ld", and casts all integral values to be printed to `long'.  */
#ifndef INTEGER_TYPE
#define INTEGER_TYPE long
#endif
typedef INTEGER_TYPE integer;

/* `volatile' is only used in Metafont to avoid bugs in the MIPS C
   compiler.  If this definition goes wrong somehow, just get rid of it
   and the two corresponding substitutions in mf/convert.  */
#ifndef __STDC__
#define volatile
#endif

/* Needed in web2c.yacc.  */
#ifndef UNSIGNED_SHORT_STRING
#define UNSIGNED_SHORT_STRING "unsigned short"
#endif

/* System-dependent hacks.  */

/* Hack to get around High C on an IBM RT treating `char' differently
   than normal compilers, etc.   */
#if defined (__HIGHC__) && defined (ibm032)
pragma	Off(Char_default_unsigned);
pragma	On(Char_is_rep);
pragma	On(Parm_warnings);
pragma	On(Pointers_compatible);
pragma	On(Pointers_compatible_with_ints);
#endif	/* __HIGHC__ and ibm032 */


/* Some definitions of our own.  */
#include "common.h"

#endif /* not CONFIG_H */
