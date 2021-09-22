/*
 *   This is dvips, a freely redistributable PostScript driver
 *   for dvi files.  It is (C) Copyright 1986-94 by Tomas Rokicki.
 *   You may modify and use this program to your heart's content,
 *   so long as you send modifications to Tomas Rokicki.  It can
 *   be included in any distribution, commercial or otherwise, so
 *   long as the banner string defined below is not modified (except
 *   for the version number) and this banner is printed on program
 *   invocation, or can be printed on program invocation with the -? option.
 */

/*   This file is the header for dvips's global data structures. */

#ifdef Omega
#define BANNER \
"This is (Omega) odvips(k) 5.78, based on dvips 5.78 from www.radicaleye.com\n"
#else
#define BANNER \
"This is dvips(k) 5.78 Copyright 1998 Radical Eye Software (www.radicaleye.com)\n"
#endif
#ifdef KPATHSEA
#include "config.h"
#include "debug.h"
#endif
/*   Please don't turn debugging off! */
#ifndef DEBUG
#define DEBUG
#endif

#include <stdio.h>
#if defined(Plan9) || defined(SYSV) || defined(VMS) || defined(__THINK__) || defined(MSDOS) || defined(OS2) || defined(ATARIST) || defined(WIN32)
#include <string.h>
#else
#include <strings.h>
#endif
#if defined(lint) && defined(sun)
extern char *sprintf() ;
#endif
#include "paths.h"
#include "debug.h"
#ifdef VMS
#include "[.vms]vms.h"
#endif /* VMS */
#include <stdlib.h>
/*
 *   Is your malloc big?
 */
#if defined(MSDOS) && !defined(__EMX__) && !defined(DJGPP)
#define SMALLMALLOC
#endif
#if defined(OS2) && defined(_MSC_VER)
#define SMALLMALLOC
#endif
/*
 *   Constants, used to increase or decrease the capacity of this program.
 *
 *   Strings are now more dynamically allocated, so STRINGSIZE is not the
 *   strict limit it once was.  However, it still sets the maximum size
 *   of a string that can be handled in specials, so it should not be
 *   set too small.
 */
#define STRINGSIZE (25000)  /* maximum total chars in strings in program */
#define RASTERCHUNK (8192)  /* size of chunk of raster */
#define MINCHUNK (240)      /* minimum size char to get own raster */
#define STACKSIZE (100)     /* maximum stack size for dvi files */
#define MAXFRAME (10)       /* maximum depth of virtual font recursion */
#define MAXFONTHD (100)     /* number of unique names of included fonts */
#define STDOUTSIZE (75)     /* width of a standard output line */
/*
 *   Other constants, which define printer-dependent stuff.
 */
#define SWMEM (180000)      /* available virtual memory in PostScript printer */
#define DPI (actualdpi)     /* dots per inch */
#define VDPI (vactualdpi)   /* dots per inch */
#define RES_TOLERANCE(dpi) ((int)(1+dpi/500))
                            /* error in file name resolution versus desired */
#define FONTCOST (298)      /* overhead cost of each sw font */
#define PSFONTCOST (1100)   /* overhead cost for PostScript fonts */
#define PSCHARCOST (20)     /* overhead cost for PostScript font chars */
#define DNFONTCOST (35000)  /* overhead cost for downloaded PS font */
#define CHARCOST (15)       /* overhead cost for each character */
#define OVERCOST (30000)    /* cost of overhead */
#define DICTITEMCOST (20)   /* cost per key, value in dictionary */
#define NAMECOST (40)       /* overhead cost of each new name */
/*
 *   Type declarations.  integer must be a 32-bit signed; shalfword must
 *   be a sixteen-bit signed; halfword must be a sixteen-bit unsigned;
 *   quarterword must be an eight-bit unsigned.
 */
#if (defined(MSDOS) && !defined(DJGPP)) || (defined(OS2) && defined(_MSC_VER)) || defined(ATARIST)
typedef long integer;
#else
typedef int integer;
#endif
#ifndef KPATHSEA
typedef char boolean;
#endif
typedef double real;
typedef short shalfword ;
typedef unsigned short halfword ;
typedef unsigned char quarterword ;
#ifdef WIN32
#define Boolean boolean
#else
#ifndef __THINK__
typedef short Boolean ;
#endif
#endif
/*
 *   If the machine has a default integer size of 16 bits, and 32-bit
 *   integers must be manipulated with %ld, set the macro SHORTINT.
 */
#ifdef XENIX
#define SHORTINT
#else
#undef SHORTINT
#endif
#if defined(MSDOS) && !defined(__EMX__) && !defined(DJGPP) || defined(ATARIST)
#define SHORTINT
#endif
#if defined(OS2) && defined(_MSC_VER)
#define SHORTINT
#endif

/*
 *   This is the structure definition for resident fonts.  We use
 *   a small and simple hash table to handle these.  We don't need
 *   a big hash table.
 */
#define RESHASHPRIME (73)
struct resfont {
   struct resfont *next ;
   char *Keyname, *PSname, *TeXname, *Fontfile, *Vectfile;
   char *specialinstructions ;
   char *downloadheader ; /* possibly multiple files */
   quarterword sent ;
} ;

/*
 *   A chardesc describes an individual character.  Before the fonts are
 *   downloaded, the flags indicate that the character has already been used
 *   with the following meanings:
 */
typedef struct tcd {
   integer TFMwidth ;
   quarterword *packptr ;
   shalfword pixelwidth ;
   quarterword flags, dmy ;
} chardesctype ;
#define EXISTS (1)
#define PREVPAGE (2)
#define THISPAGE (4)
#define TOOBIG (8) /* not used at the moment */
#define REPACKED (16)
#define BIGCHAR (32)
#define STATUSFLAGS (EXISTS|REPACKED|BIGCHAR)
/*
 *   A fontdesc describes a font.  The name, area, and scalename are located in
 *   the string pool. The nextsize pointer is used to link fonts that are used
 *   in included psfiles and differ only in scaledsize.  Such fonts also have
 *   a non-NULL scalename that gives the scaledsize as found in the included
 *   file.  The psflag indicates that the font has been used in an included
 *   psfile.  It can be 0, PREVPAGE, THISPAGE, or EXISTS.
 */
typedef struct tfd {
   integer checksum, scaledsize, designsize, thinspace ;
   halfword dpi, loadeddpi ;
   halfword alreadyscaled ;
   halfword psname ;
   halfword loaded ;
#ifdef Omega
   integer maxchars ;
#else
   halfword maxchars ;
#endif
   char *name, *area ;
   struct resfont *resfont ;
   struct tft *localfonts ;
   struct tfd *next ;
   struct tfd *nextsize;
   char *scalename;
   quarterword psflag;
#ifdef Omega
   chardesctype *chardesc ;
#else
   chardesctype chardesc[256] ;
#endif
} fontdesctype ;

/*  A fontmap associates a fontdesc with a font number.
 */
typedef struct tft {
   integer fontnum ;
   fontdesctype *desc ;
   struct tft *next ;
} fontmaptype ;

/*   Virtual fonts require a `macro' capability that is implemented by
 *   using a stack of `frames'.
 */
typedef struct {
   quarterword *curp, *curl ;
   fontdesctype *curf ;
   fontmaptype *ff ;
} frametype ;

/*
 *   The next type holds the font usage information in a 256-bit table;
 *   there's a 1 for each character that was used in a section.
 */
typedef struct {
   fontdesctype *fd ;
   halfword psfused ;
   halfword bitmap[16] ;
} charusetype ;

/*   Next we want to record the relevant data for a section.  A section is
 *   a largest portion of the document whose font usage does not overflow
 *   the capacity of the printer.  (If a single page does overflow the
 *   capacity all by itself, it is made into its own section and a warning
 *   message is printed; the page is still printed.)
 *
 *   The sections are in a linked list, built during the prescan phase and
 *   processed in proper order (so that pages stack correctly on output) during
 *   the second phase.
 */
typedef struct t {
   integer bos ;
   struct t *next ;
   halfword numpages ;
} sectiontype ;

/*
 *   Sections are actually represented not by sectiontype but by a more
 *   complex data structure of variable size, having the following layout:
 *      sectiontype sect ;
 *      charusetype charuse[numfonts] ;
 *      fontdesctype *sentinel = NULL ;
 *   (Here numfonts is the number of bitmap fonts currently defined.)
 *    Since we can't declare this or take a sizeof it, we build it and
 *   manipulate it ourselves (see the end of the prescan routine).
 */
/*
 *   This is how we build up headers and other lists.
 */
struct header_list {
   struct header_list *next ;
   char *Hname ;
   char name[1] ;
} ;
/*
 *   Some machines define putlong in their library.
 *   We get around this here.
 */
#define putlong was_putlong
/*
 *   Information on available paper sizes is stored here.
 */
struct papsiz {
   struct papsiz *next ;
   integer xsize, ysize ;
   char *name ;
   char *specdat ;
} ;
#ifdef MVSXA /* IBM: MVS/XA */
/* this is where we fix problems with conflicts for truncation
   of long names (we might only do this if LONGNAME not set but ...) */
#   define pprescanpages pprscnpgs  /* confict with pprescan */
#   define flushDashedPath flshDshdPth  /* conflict with flushDash */
#   define PageList PgList  /* re-definition conflict with pagelist  */
/* adding ascii2ebcdic conversion table to extern */
    extern char ascii2ebcdic[] ;
#endif  /* IBM: MVS/XA */
#ifdef VMCMS /* IBM: VM/CMS */
/* this is where we fix problems with conflicts for truncation
   of long names (we might only do this if LONGNAME not set but ...) */
#   define pprescanpages pprscnpgs  /* confict with pprescan */
#   define flushDashedPath flshDshdPth  /* conflict with flushDash */
/* adding ascii2ebcdic conversion table to extern */
    extern char ascii2ebcdic[] ;
/* redefining fopen for VMCMS, see DVIPSCMS.C */
    extern FILE *cmsfopen() ;
#   ifdef fopen
#      undef fopen
#   endif
#   define fopen cmsfopen
#define downloadpspk dnldpspk
#endif  /* IBM: VM/CMS */
/*
 *   Remove namespace conflict with standard library on Macintosh
 */
#ifdef __THINK__
#define newstring newpoolstring
#endif

#ifndef VOID
#define VOID void
#endif

#define USE_PCLOSE (801)
#define USE_FCLOSE (802)

/* Things that KPATHSEA knows, and are useful even without it. */
#if !defined(KPATHSEA)

#if defined(MSDOS) || defined(OS2) || defined(WIN32)
#define FOPEN_ABIN_MODE "ab"
#define FOPEN_RBIN_MODE "rb"
#else
#define FOPEN_ABIN_MODE "a"
#define FOPEN_RBIN_MODE "r"
#endif

#if (defined MSDOS || defined OS2 || defined WIN32)
#define WRITEBIN "wb"
#else
#ifdef VMCMS
#define WRITEBIN "wb, lrecl=1024, recfm=f"
#else
#define WRITEBIN "w"
#endif
#endif

#if defined(WIN32)
#define STDC_HEADERS
#include <io.h>
#include <fcntl.h>
#define O_BINARY _O_BINARY
#define popen _popen
#define pclose _pclose
#define register
#define SET_BINARY(fd) _setmode((fd), _O_BINARY)
#else /* !WIN32 */
#define SET_BINARY(fd)
#endif

#if defined(DEVICESEP)
#define IS_DEVICE_SEP(c) ((c) == DEVICESEP)
#else
#define IS_DEVICE_SEP(c) 0
#endif
#define STREQ(s1, s2) (!strcmp((s1), (s2)))

#if defined(MSDOS) || defined(OS2) || defined(WIN32)
#define NAME_BEGINS_WITH_DEVICE(name) (*(name) && IS_DEVICE_SEP((name)[1]))
#define IS_DIR_SEP(c) ((c) == '/' || (c) == '\\')
#else
#define NAME_BEGINS_WITH_DEVICE(name) 0
#define IS_DIR_SEP(c) ((c) == DIRSEP)
#endif

#define TOLOWER(c) tolower(c)
#define ISALNUM(c) isalnum(c)
#define ISXDIGIT(c) (isascii (c) && isxdigit(c))

 /* To Prototype or not to prototype ? */
#ifdef NeedFunctionPrototypes

#define AA(args) args /* For an arbitrary number; ARGS must be in parens.  */

#define P1H(p1) (p1)
#define P2H(p1,p2) (p1, p2)
#define P3H(p1,p2,p3) (p1, p2, p3)
#define P4H(p1,p2,p3,p4) (p1, p2, p3, p4)
#define P5H(p1,p2,p3,p4,p5) (p1, p2, p3, p4, p5)
#define P6H(p1,p2,p3,p4,p5,p6) (p1, p2, p3, p4, p5, p6)

#define P1C(t1,n1)(t1 n1)
#define P2C(t1,n1, t2,n2)(t1 n1, t2 n2)
#define P3C(t1,n1, t2,n2, t3,n3)(t1 n1, t2 n2, t3 n3)
#define P4C(t1,n1, t2,n2, t3,n3, t4,n4)(t1 n1, t2 n2, t3 n3, t4 n4)
#define P5C(t1,n1, t2,n2, t3,n3, t4,n4, t5,n5) \
  (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5)
#define P6C(t1,n1, t2,n2, t3,n3, t4,n4, t5,n5, t6,n6) \
  (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6)

#else /* no function prototypes */

#define AA(args) ()

#define P1H(p1) ()
#define P2H(p1, p2) ()
#define P3H(p1, p2, p3) ()
#define P4H(p1, p2, p3, p4) ()
#define P5H(p1, p2, p3, p4, p5) ()
#define P6H(p1, p2, p3, p4, p5, p6) ()

#define P1C(t1,n1) (n1) t1 n1;
#define P2C(t1,n1, t2,n2) (n1,n2) t1 n1; t2 n2;
#define P3C(t1,n1, t2,n2, t3,n3) (n1,n2,n3) t1 n1; t2 n2; t3 n3;
#define P4C(t1,n1, t2,n2, t3,n3, t4,n4) (n1,n2,n3,n4) \
  t1 n1; t2 n2; t3 n3; t4 n4;
#define P5C(t1,n1, t2,n2, t3,n3, t4,n4, t5,n5) (n1,n2,n3,n4,n5) \
  t1 n1; t2 n2; t3 n3; t4 n4; t5 n5;
#define P6C(t1,n1, t2,n2, t3,n3, t4,n4, t5,n5, t6,n6) (n1,n2,n3,n4,n5,n6) \
  t1 n1; t2 n2; t3 n3; t4 n4; t5 n5; t6 n6;

#endif /* function prototypes */

#endif

#ifdef NeedFunctionPrototypes
#define P9C(t1,n1, t2,n2, t3,n3, t4,n4, t5,n5, t6,n6, t7,n7, t8,n8, t9,n9) \
  (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9)
#else
#define P9C(t1,n1, t2,n2, t3,n3, t4,n4, t5,n5, t6,n6, t7,n7, t8,n8, t9,n9) \
  t1 n1; t2 n2; t3 n3; t4 n4; t5 n5; t6 n6; t7 n7; t8 n8; t9 n9;
#endif
