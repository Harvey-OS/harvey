/* common.h: Definitions and declarations common both to the change
   files and to web2c itself.  This is included from config.h, which
   everyone includes.  */

#ifndef COMMON_H
#define COMMON_H

#include "getopt.h"
#include "lib.h"
#include "ourpaths.h"
#include "pascal.h"
#include "types.h"


/* We never need the `link' system call, which is sometimes declared in
   <unistd.h>, but we do have lots of variables called `link' in the web
   sources.  */
#ifdef link
#undef link
#endif
#define link link_var


/* On VMS, `exit (1)' is success.  */
#ifndef EXIT_SUCCESS_CODE
#ifdef VMS
#define EXIT_SUCCESS_CODE 1
#else
#define EXIT_SUCCESS_CODE 0
#endif
#endif /* not EXIT_SUCCESS_CODE */

/* Some features are only needed in certain programs.  */

#ifdef TeX
/* The type `glueratio' should be a floating point type which won't
   unnecessarily increase the size of the memoryword structure.  This is
   the basic requirement.  On most machines, if you're building a
   normal-sized TeX, then glueratio must probably meet the following
   restriction: sizeof(glueratio) <= sizeof(integer).  Usually, then,
   glueratio must be `float'.  But if you build a big TeX, you can (on
   most machines) and should make it `double' to avoid loss of precision
   and conversions to and from double during calculations.  (All this
   also goes for Metafont.)  Furthermore, if you have enough memory, it
   won't hurt to have this defined to be `double' for running the
   trip/trap tests.
   
   This type is set automatically to `float' by configure if a small TeX
   is built.  */
#ifndef GLUERATIO_TYPE
#define GLUERATIO_TYPE double
#endif
typedef GLUERATIO_TYPE glueratio;
#endif

/* Define our own abbreviations, etc.  */

#define START_FATAL() do { fprintf (stderr, "fatal: ")
#define END_FATAL() fprintf (stderr, ".\n"); exit (1); } while (0)

#define FATAL(str)							\
  START_FATAL (); fprintf (stderr, "%s", str); END_FATAL ()
#define FATAL1(s, e1)							\
  START_FATAL (); fprintf (stderr, s, e1); END_FATAL ()
#define FATAL2(s, e1, e2)						\
  START_FATAL (); fprintf (stderr, s, e1, e2); END_FATAL ()
#define FATAL3(s, e1, e2, e3)						\
  START_FATAL (); fprintf (stderr, s, e1, e2, e3); END_FATAL ()

#define STREQ(s1, s2) (strcmp (s1, s2) == 0)

/* This should be called only after a system call fails.  */
#define FATAL_PERROR(s) do { perror (s); exit (errno); } while (0)
extern int errno;


/* Declarations for the routines we provide ourselves.  */

extern integer zround ();


/* File routines.  */
extern FILE *xfopen_pas ();
extern boolean test_eof ();
extern boolean eoln ();
extern boolean open_input ();
extern boolean open_output ();
extern void fprintreal ();
extern void printpascalstring (), errprintpascalstring ();
extern integer inputint ();
extern void zinput3ints ();
extern void setpaths ();

/* String routines.  */
extern void makesuffixpas ();
extern void make_c_string (), make_pascal_string ();
extern void null_terminate (), space_terminate ();


/* Argument handling.  */
extern int argc;
extern char **gargv;
extern void argv ();

#endif /* not COMMON_H */
