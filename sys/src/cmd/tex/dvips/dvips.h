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

#define BANNER \
             "This is dvips 5.58 Copyright 1986, 1994 Radical Eye Software\n"
#include <stdio.h>
#if defined(SYSV) || defined(VMS) || defined(__THINK__) || defined(MSDOS) || defined(OS2) || defined(ATARIST)
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
#include "[]vms.h"
#endif /* VMS */
/*
 *   We use malloc and free; these may need to be customized for your
 *   site.
 */
#if defined MSDOS || defined OS2 || defined __alpha
void *malloc() ;
#else
#ifndef IBM6000
char *malloc() ;
#endif
#endif
void free() ;
char *mymalloc() ;
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
typedef char boolean;
typedef double real;
typedef short shalfword ;
typedef unsigned short halfword ;
typedef unsigned char quarterword ;
#ifndef __THINK__
typedef short Boolean ;
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
   char *Keyname, *PSname, *TeXname ;
   char *specialinstructions ;
   char *downloadheader ; /* possibly multiple files */
   quarterword sent ;
} ;

/*
 *   A chardesc describes an individual character.  Before the fonts are
 *   downloaded, the flags indicate that the character has already been used
 *   with the following meanings:
 */
typedef struct {
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
   halfword maxchars ;
   char *name, *area ;
   struct resfont *resfont ;
   struct tft *localfonts ;
   struct tfd *next ;
   struct tfd *nextsize;
   char *scalename;
   quarterword psflag;
   chardesctype chardesc[256] ;
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
