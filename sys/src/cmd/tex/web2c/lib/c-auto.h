/* lib/c-auto.h.  Generated automatically by configure.  */
/* c-auto.h.in: template for c-auto.h.  */

/* Define to be however your lex declares yytext, including the semicolon.  */
#define DECLARE_YYTEXT extern char yytext[];

/* Define if your system has <dirent.h>.  */
#define DIRENT 1

/* Define if your system has <sys/ndir.h>.  */
#undef SYSNDIR

/* Define if the directory library header declares closedir as void.  */
#undef VOID_CLOSEDIR

/* Define if your system lacks <limits.h>.  */
#undef LIMITS_H_MISSING

/* Likewise, for <float.h>.  */
#undef FLOAT_H_MISSING

/* Define if <string.h> does not declare memcmp etc., and <memory.h> does.  */
#undef NEED_MEMORY_H

/* Define if your system has ANSI C header files: <stdlib.h>, etc.  */
#define STDC_HEADERS 1

/* Define if your system has <unistd.h>.  */
#define HAVE_UNISTD_H 1

/* Define if your system lacks rindex, bzero, etc.  */
#define USG 1

/* Define as your system's signal handler return type, if not void.  */
#undef RETSIGTYPE

/* Define if the C type `char' is unsigned.  We protect this one with
   #ifndef because the compiler might (should) define it -- and if it
   is, then we don't want to undefine it.  */
#ifndef __CHAR_UNSIGNED__
#undef __CHAR_UNSIGNED__
#endif

/* Define if words are stored most-significant-byte first.  */
#define WORDS_BIGENDIAN 1

/* Define as `float' if making a ``small'' TeX.  */
#undef GLUERATIO_TYPE
