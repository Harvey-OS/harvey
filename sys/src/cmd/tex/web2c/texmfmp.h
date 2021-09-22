/* texmf.h: Main include file for TeX and Metafont in C. This file is
   included by {tex,mf}d.h, which is the first include in the C files
   output by web2c.  */

#include "cpascal.h"
#include <kpathsea/c-pathch.h> /* for IS_DIR_SEP, used in the change files */
#include <kpathsea/tex-make.h> /* for kpse_make_tex_discard_errors */

/* If we have these macros, use them, as they provide a better guide to
   the endianess when cross-compiling. */
#if defined (BYTE_ORDER) && defined (BIG_ENDIAN) && defined (LITTLE_ENDIAN)
#ifdef WORDS_BIGENDIAN
#undef WORDS_BIGENDIAN
#endif
#if BYTE_ORDER == BIG_ENDIAN
#define WORDS_BIGENDIAN
#endif
#endif
/* More of the same, but now NeXT-specific. */
#ifdef NeXT
#ifdef WORDS_BIGENDIAN
#undef WORDS_BIGENDIAN
#endif
#ifdef __BIG_ENDIAN__
#define WORDS_BIGENDIAN
#endif
#endif

/* Some things are the same except for the name.  */
#ifdef TeX
#ifdef PDFTeX
#define PDFTEXPOOLNAME "pdftex.pool"
#endif
#if defined (eTeX)
#define TEXPOOLNAME "etex.pool"
#elif defined (Omega)
#define TEXPOOLNAME "omega.pool"
#else
#define TEXPOOLNAME "tex.pool"
#endif
#define DUMP_FILE fmtfile
#define DUMP_FORMAT kpse_fmt_format
#define writedvi write_out
#define flushdvi flush_out
#define OUT_FILE dvifile
#define OUT_BUF dvibuf
#endif /* TeX */
#ifdef MF
#define DUMP_FILE basefile
#define DUMP_FORMAT kpse_base_format
#define writegf write_out
#define OUT_FILE gffile
#define OUT_BUF gfbuf
#endif /* MF */
#ifdef MP
#define DUMP_FILE memfile
#define DUMP_FORMAT kpse_mem_format
#endif /* MP */

/* Restore underscores.  */
#define kpsedvipsconfigformat kpse_dvips_config_format
#define kpsemfpoolformat kpse_mfpool_format
#define kpsempformat kpse_mp_format
#define kpsemppoolformat kpse_mppool_format
#define kpsetexpoolformat kpse_texpool_format
#define kpsetexformat kpse_tex_format

/* Hacks for TeX that are better not to #ifdef, see texmfmp.c.  */
extern int tfmtemp, texinputtype;

/* Both TeX and MetaPost use this.  */
extern boolean openoutnameok P1H(const_string);

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

extern void setupcharset P1H(void);

#ifdef IPC
extern void ipcpage P1H(int);
#endif /* IPC */
#endif /* TeX */

/* How to output to the GF or DVI file.  */
#define	write_out(a, b)							\
  if (fwrite ((char *) &OUT_BUF[a], sizeof (OUT_BUF[a]),		\
                 (int) ((b) - (a) + 1), OUT_FILE) 			\
      != (int) ((b) - (a) + 1))						\
    FATAL_PERROR ("fwrite");

#define flush_out() fflush (OUT_FILE)

/* Used to write to a TFM file.  */
#define put2bytes(f, h) do { \
    integer v = (integer) (h); putbyte (v >> 8, f);  putbyte (v & 0xff, f); \
  } while (0)
#define put4bytes(f, w) do { \
    integer v = (integer) (w); \
    putbyte (v >> 24, f); putbyte (v >> 16, f); \
    putbyte (v >> 8, f);  putbyte (v & 0xff, f); \
  } while (0)

/* Read a line of input as quickly as possible.  */
#define	inputln(stream, flag) input_line (stream)
extern boolean input_line P1H(FILE *);

/* This routine has to return four values.  */
#define	dateandtime(i,j,k,l) get_date_and_time (&(i), &(j), &(k), &(l))
extern void get_date_and_time P4H(integer *, integer *, integer *, integer *);

/* Copy command-line arguments into the buffer, despite the name.  */
extern void topenin P1H(void);

/* Can't prototype this since it uses poolpointer and ASCIIcode, which
   are defined later in mfd.h, and mfd.h uses stuff from here.  */
/* sure we can: ASCIIcode is a uchar.  -rsc */
extern void calledit();

/* Used to allocate the hyphenation arrays at runtime in initex.  Always
   add one to SIZE because Pascal arrays start at 1 and continue through
   the maximum.  */
#define xmallocarray(array,size) array = xmalloc ((size+1) * sizeof (array[0]))

/* Set an array size from texmf.cnf.  */
extern void setupboundvariable P3H(integer *, const_string, integer);

/* `bopenin' (and out) is used only for reading (and writing) .tfm
   files; `wopenin' (and out) only for dump files.  The filenames are
   passed in as a global variable, `nameoffile'.  */
#define bopenin(f)	open_input (&(f), kpse_tfm_format, FOPEN_RBIN_MODE)
#define ocpopenin(f)	open_input (&(f), kpse_ocp_format, FOPEN_RBIN_MODE)
#define ofmopenin(f)	open_input (&(f), kpse_ofm_format, FOPEN_RBIN_MODE)
#define bopenout(f)	open_output (&(f), FOPEN_WBIN_MODE)
#define bclose		aclose
#define wopenin(f)	open_input (&(f), DUMP_FORMAT, FOPEN_RBIN_MODE)
#define wopenout	bopenout
#define wclose		aclose

/* Used in tex.ch (section 1338) to get a core dump in debugging mode.  */
#ifdef unix
#define dumpcore abort
#else
#define dumpcore uexit (1)
#endif

#ifdef MP
extern boolean callmakempx P2H(string, string);
#endif

#ifdef MF
extern boolean initscreen P1H(void);
extern void updatescreen P1H(void);
/* Can't prototype these for same reason as `calledit' above.  */
extern void blankrectangle (screencol, screencol, screenrow, screenrow);
extern void paintrow (/*screenrow, pixelcolor, transspec, screencol*/);
#endif /* MF */

/* (Un)dumping.  These are called from the change file.  */
#define	dumpthings(base, len) \
  do_dump ((char *) &(base), sizeof (base), (int) (len), DUMP_FILE)
#define	undumpthings(base, len) \
  do_undump ((char *) &(base), sizeof (base), (int) (len), DUMP_FILE)

/* Like do_undump, but check each value against LOW and HIGH.  The
   slowdown isn't significant, and this improves the chances of
   detecting incompatible format files.  In fact, Knuth himself noted
   this problem with Web2c some years ago, so it seems worth fixing.  We
   can't make this a subroutine because then we lose the type of BASE.  */
#define undumpcheckedthings(low, high, base, len)			\
  do {                                                                  \
    unsigned i;                                                         \
    undumpthings (base, len);                                           \
    for (i = 0; i < (len); i++) {                                       \
      if ((&(base))[i] < (low) || (&(base))[i] > (high)) {              \
        FATAL5 ("Item %u (=%ld) of .fmt array at %lx <%ld or >%ld",     \
                i, (integer) (&(base))[i], (unsigned long) &(base),     \
                (integer) low, (integer) high);                         \
      }                                                                 \
    }									\
  } while (0)

/* Like undump_checked_things, but only check the upper value. We use
   this when the base type is unsigned, and thus all the values will be
   greater than zero by definition.  */
#define undumpuppercheckthings(high, base, len)				\
  do {                                                                  \
    unsigned i;                                                         \
    undumpthings (base, len);                                           \
    for (i = 0; i < (len); i++) {                                       \
      if ((&(base))[i] > (high)) {              			\
        FATAL4 ("Item %u (=%ld) of .fmt array at %lx >%ld",     	\
                i, (integer) (&(base))[i], (unsigned long) &(base),     \
                (integer) high);                         		\
      }                                                                 \
    }									\
  } while (0)

/* We define the routines to do the actual work in texmf.c.  */
extern void do_dump P4H(char *, int, int, FILE *);
extern void do_undump P4H(char *, int, int, FILE *);

/* Use the above for all the other dumping and undumping.  */
#define generic_dump(x) dumpthings (x, 1)
#define generic_undump(x) undumpthings (x, 1)

#ifdef PDFTeX
#include <pdftexdir/pdftex.h>
#endif /* PDFTeX */

#define dumpwd   generic_dump
#define dumphh   generic_dump
#define dumpqqqq generic_dump
#define undumpwd   generic_undump
#define undumphh   generic_undump
#define	undumpqqqq generic_undump

/* `dump_int' is called with constant integers, so we put them into a
   variable first.  */
#define	dumpint(x)							\
  do									\
    {									\
      integer x_val = (x);						\
      generic_dump (x_val);						\
    }									\
  while (0)

/* web2c/regfix puts variables in the format file loading into
   registers.  Some compilers aren't willing to take addresses of such
   variables.  So we must kludge.  */
#ifdef REGFIX
#define undumpint(x)							\
  do									\
    {									\
      integer x_val;							\
      generic_undump (x_val);						\
      x = x_val;							\
    }									\
  while (0)
#else
#define	undumpint generic_undump
#endif
