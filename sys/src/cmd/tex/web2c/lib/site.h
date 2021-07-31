/* site.h: Site-specific preferences, to be edited to taste.  */

#ifndef WEB2C_SITE_H
#define WEB2C_SITE_H

/* Define this if you want to compile the small (64K memory) TeX/MF.
   The default is to compile the big (260K memory) versions.
   Similarly for BibTeX.  */
#undef SMALLTeX
#undef SMALLMF
#undef SMALLBibTeX

/* Define these according to your local setup.  The default is to search
   subdirectories of /usr/local/lib/tex/fonts for fonts, and of
   /usr/local/lib/tex/macros for TeX source files.  If your directory
   scheme is not organized for subdirectory searching, and you compile
   with these values, there is a chance that TeX will start up very,
   very slowly.  If this happens, modify these paths appropriately.
   
   See the documentation for all the details on which paths are searched
   for what files.

   If `.' is not in the font or input paths, then none of these
   programs will look in the current directory.  For example, `tex foo'
   will not find `./foo.tex', and `gftopk foo.300gf' will not find
   `./foo.300gf'.  If you do this, users will probably be confused.
   
   Do not put a leading or trailing colon in these paths, or double a
   colon in the middle.  That might lead to infinite recursion.  */

#define	BIBINPUTS  ".:/sys/lib/tex/macros"		/* bib file */
#define BSTINPUTS  TEXINPUTS				/* bib style file */
#define GFFONTS    ".:/sys/lib/tex/fonts"		/* GF font */
#define MFBASES    "/sys/lib/mf"			/* base file */
#define MFINPUTS   ".:/sys/lib/mf"
#define MFPOOL     "/sys/lib/mf"			/* mf.pool */
#define PKFONTS    ".:/sys/lib/tex/fonts/canonpk"		/* PK font */
#define TEXFONTS   "/sys/lib/tex/fonts/tfm:."		/* TFM font */
#define TEXFORMATS "/sys/lib/tex/macros"		/* format file */
#define TEXINPUTS  ".:/sys/lib/tex/macros"		/* TeX source */
#define TEXPOOL    "/sys/lib/tex"			/* tex.pool */
#define VFFONTS	   "/sys/lib/tex/fonts/psvf:."		/* VFM font */

/* Metafont online output support: More than one may be defined, except
   that you can't have both X10 and X11 support (because there are
   conflicting routine names in the libraries).
   
   If you want X11 support, see the `Online output from Metafont'
   section in README.W2C before compiling.  */
#undef	HP2627WIN		/* HP 2627. */
#undef	SUNWIN			/* SunWindows. */
#undef	TEKTRONIXWIN		/* Tektronix 4014. */
#undef	UNITERMWIN		/* Uniterm Tektronix.  */
#undef	X10WIN			/* X Version 10. */
#undef	X11WIN			/* X Version 11. */

/* Default editor command string: `%d' expands to the line number where
   TeX or Metafont found an error and `%s' expands to the name of the
   file.  The environment variables TEXEDIT and MFEDIT override this.  */
#define	EDITOR	"B -%d %s"

/* If you want to be able to share format/base files across architectures,
   define this.  Makes loading slower on LittleEndian machines.  */
#define FMTBASE_SWAP

/* If you want to be able to produce a core dump (to make a preloaded
   TeX) by giving the filename `HackyInputFileNameForCoreDump.tex' to
   TeX (or Metafont), define this.  Probably works only for BSD-like
   systems.  */
#undef FUNNY_CORE_DUMP

/* You should redefine this only if you are using some non-standard TeX
   variant which has a different string pool, e.g., Michael Ferguson's
   MLTeX.  */
#define TEXPOOLNAME "tex.pool"

/* Our character set is 8-bit ASCII unless NONASCII is defined.  For
   other character sets, make sure that first_text_char and
   last_text_char are defined correctly (they're 0 and 255,
   respectively, by default).  In the *.defines files, change the
   indicated range of type `char' to be the same as
   first_text_char..last_text_char, `#define NONASCII', and retangle and
   recompile everything.  */
#undef	NONASCII

/* On some systems, explicit register declarations make a big
   difference.  On others, they make no difference at all.  (GCC ignores
   them when optimizing, for example.)  */
#undef	REGFIX

#endif /* not WEB2C_SITE_H */
