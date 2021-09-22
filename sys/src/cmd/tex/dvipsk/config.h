/* config.h: master configuration file, included first by all compilable
   source files (not headers).  */

#ifndef CONFIG_H
#define CONFIG_H

/* The stuff from the path searching library.  */
#include <kpathsea/config.h>

/* How to open files with fopen.  */
#include <kpathsea/c-fopen.h>

/* How the filenames' parts are separated.  */
#include <kpathsea/c-pathch.h>

/* Have to get the enum constants below, sigh.  Still better than
   repeating the definitions everywhere we need them.  */
#include <kpathsea/tex-file.h>

#ifdef __STDC__
#define NeedFunctionPrototypes 1
#include <kpathsea/c-proto.h>
#endif

/* For kpathsea, we don't have paths, we have formats.  This is so
   we can do lazy evaluation of only the formats we need, instead of
   having to initialize everything in the world.  */
#define figpath kpse_pict_format
#define pictpath kpse_pict_format
#define pkpath kpse_pk_format
#define tfmpath kpse_tfm_format
#ifdef Omega
#define ovfpath kpse_ovf_format
#endif
#define vfpath kpse_vf_format
#define configpath kpse_dvips_config_format
#define headerpath kpse_tex_ps_header_format
#define type1 kpse_type1_format

#if (defined (DOS) || defined (MSDOS)) && !defined (__DJGPP__)
#undef DOS
#undef MSDOS
#define DOS
#define MSDOS
#endif

/* dvips has a different name for this.  */
#if SIZEOF_INT < 4
#define SHORTINT
#endif

#define READ FOPEN_R_MODE
#define READBIN FOPEN_RBIN_MODE
#define WRITEBIN FOPEN_WBIN_MODE

/* Include various things by default.  */
#ifndef NO_HPS
#define HPS
#endif
#ifndef NO_TPIC
#define TPIC
#endif
#ifndef NO_EMTEX
#define EMTEX
#endif

/* Include debugging by default, too.  */
#ifndef NO_DEBUG
#undef DEBUG
#define DEBUG

/* To pass along to kpathsea.  (Avoid changing debug.h.)  */
#define D_STAT		(1<<9)
#define D_HASH		(1<<10)
#define D_EXPAND	(1<<11)
#define D_SEARCH	(1<<12)
#endif

/* These are defined under NT, but we have variable names.  */
#undef ERROR
#undef NO_ERROR

#endif /* not CONFIG_H */
