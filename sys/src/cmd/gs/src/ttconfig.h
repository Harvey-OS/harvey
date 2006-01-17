/* Copyright (C) 2003 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: ttconfig.h,v 1.1 2003/10/01 13:44:56 igor Exp $ */
/* Changes after FreeType: cut out the TrueType instruction interpreter. */

/*******************************************************************
 *
 *  ttconfig.h                                                1.0   
 *
 *    Configuration settings header file (spec only).            
 *
 *  Copyright 1996-1998 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT.  By continuing to use, modify, or distribute 
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 *  Notes:
 *
 *    All the configuration #define statements have been gathered in
 *    this file to allow easy check and modification.       
 *
 ******************************************************************/

#ifndef TTCONFIG_H
#define TTCONFIG_H

/* ------------ auto configuration ------------------------------------- */

/* Here we include the file ft_conf.h for system dependent stuff.        */
/* The specific makefile is responsible for providing the right path to  */
/* this file.                                                            */

#include "ttconf.h"


/* ------------ general debugging -------------------------------------- */

/* Define DEBUG if you want the program to output a series of messages   */
/* to stderr regarding its behaviour.  Only useful during development.   */

/* #define DEBUG */


/* ------------ arithmetic and processor support - ttcalc, ttraster ---- */

/* Define ONE_COMPLEMENT if this matches your processor's artihmetic.    */
/* The default is 2's complement.  1's complement is not supported yet   */
/* (and probably never will :-).                                         */

/* #define ONE_COMPLEMENT */


/* BOUND_CALC isn't needed anymore due to changes in the ttcalc */
/* component.  All computations are now bounded.                */


/* Define _GNUC_LONG64_ if you want to enable the use of the 'long long' */
/* 64-bit type provided by gcc.  Note that:                              */
/*                                                                       */
/*   1. The type isn't ANSI, and thus will produce many warnings         */
/*      during library compilation.                                      */
/*                                                                       */
/*   2. Though the generated object files are slightly smaller, the      */
/*      resulting executables are bigger of about 4Kb! gcc must be       */
/*      linking some extra code in there!                                */
/*                                                                       */
/*   3. There is really no speed gain in doing so (but it may help       */
/*      debug the ttcalc component).                                     */
/*                                                                       */
/* IMPORTANT NOTE: You don't need to define it on 64-bits machines!      */

/* #define _GNUC_LONG64_ */


/* define BUS_ERROR if your processor is unable to access words that */
/* are not aligned to their respective size (i.e. a 4byte dword      */
/* beginning at address 3 will result in a bus error on a Sun).      */

/* This may speed up a bit some parts of the engine */

/* #define BUS_ERROR */


/* define ALIGNMENT to your processor/environment preferred alignment */
/* size. A value of 8 should work on all current processors, even     */
/* 64-bits ones.                                                      */

#define ALIGNMENT 8


/* ------------ rasterizer configuration ----- ttraster ----------------- */

/* Define this if you want to use the 'MulDiv' function from 'ttcalc'.    */
/* (It computes (A*B)/C with 64 bits intermediate accuracy.  However, for */
/* 99.9% of screen display, this operation can be done directly with      */
/* good accuracy, because 'B' is only a 6bit integer.)                    */
/*                                                                        */
/* Note that some compilers can manage directly 'a*b/c' with intermediate */
/* accuracy (GCC can use long longs, for example).  Using the unsecure    */
/* definition of MulDiv would then be sufficient.                         */
/*                                                                        */
/* The SECURE_COMPUTATIONS option is probably a good option for 16 bits   */
/* compilers.                                                             */

#define SECURE_COMPUTATIONS


/* Define this if you want to generate a debug version of the rasterizer. */
/* This will progressively draw the glyphs while the computations are     */
/* done directly on the graphics screen... (with inverted glyphs)         */
/*                                                                        */

/* IMPORTANT: This is reserved to developers willing to debug the */
/*            rasterizer, which seems working very well in its    */
/*            current state...                                    */

/* #define DEBUG_RASTER */


/* The TrueType specs stipulate that the filled regions delimited by  */
/* the contours must be to the right of the drawing orientation.      */
/* Unfortunately, a lot of cheapo fonts do not respect this rule.     */
/*                                                                    */
/* Defining IGNORE_FILL_FLOW builds an engine that manages all cases. */
/* Not defining it will only draw 'valid' glyphs & contours.          */

#define IGNORE_FILL_FLOW
/* We want to draw all kinds of glyphs, even incorrect ones... */




/* --------------- automatic setup -- don't touch ------------------ */

/* Some systems can't use vfprintf for error messages on stderr; if  */
/* HAVE_PRINT_FUNCTION is defined, the Print macro must be supplied  */
/* externally (having the same parameters).                          */

#ifndef HAVE_PRINT_FUNCTION
#define Print( format, ap )  vfprintf( stderr, (format), (ap) )
#endif

#define FT_BIG_ENDIAN     4321
#define FT_LITTLE_ENDIAN  1234

#ifdef WORDS_BIGENDIAN
#define FT_BYTE_ORDER  FT_BIG_ENDIAN
#else
#define FT_BYTE_ORDER  FT_LITTLE_ENDIAN
#endif

#if FT_BYTE_ORDER == FT_BIG_ENDIAN
#ifndef BUS_ERROR

/* Some big-endian machines that are not alignment-sensitive may   */
/* benefit from an easier access to the data found in the TrueType */
/* files. (used in ttfile.c)                                       */

#define LOOSE_ACCESS

#endif /* !BUS_ERROR */
#endif /* FT_BYTE_ORDER */

/* -------------------- table management configuration ------------ */

/* Define TT_CONFIG_THREAD_SAFE if you want to build a thread-safe */
/* version of the library.                                         */
#undef TT_CONFIG_THREAD_SAFE

/* Define TT_CONFIG_REENTRANT if you want to build a re-entrant version */
/* of the library.  This flag takes over TT_CONFIG_THREAD_SAFE but it   */
/* is highly recommended to leave only one of them defined.             */
#undef TT_CONFIG_REENTRANT

#if defined(TT_CONFIG_THREAD_SAFE) || defined(TT_CONFIG_REENTRANT)
#define TT_CONFIG_THREADS
#endif

/* Defining none of these two flags produces a single-thread version of */
/* the library.                                                         */

#undef TT_STATIC_INTERPRETER
/* Do not undefine this configuration macro. It is now a default that */
/* must be kept in all release builds.                                */

#undef TT_STATIC_RASTER
/* Define this if you want to generate a static raster.  This makes */
/* a non re-entrant version of the scan-line converter, which is    */
/* about 10% faster and 50% bigger than an indirect one!            */

#define TT_EXTEND_ENGINE
/* Undefine this macro if you don't want to generate any extensions to */
/* the engine.  This may be useful to detect if a bug comes from the   */
/* engine itself or some badly written extension.                      */

#endif /* TTCONFIG_H */


/* END */
