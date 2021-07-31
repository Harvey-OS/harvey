/* Copyright (C) 1995, 1997 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gdevbjc.h,v 1.2 2000/09/19 19:00:11 lpd Exp $*/

#ifndef gdevbjc_INCLUDED
#  define gdevbjc_INCLUDED

/*
 * Definitions for Canon BJC printers and the associated drivers.
 *
 * Copyright (C) Yves Arrouye <yves.arrouye@usa.net>, 1995, 1996.
 *
 */

/*
 * Please do read the definitions here and change the defaults if needed.
 *
 * Values that can be changed are all called BJC_DEFAULT_* for generic
 * values and BJC600_DEFAULT_* or BJC800_DEFAULT_* for specific values.
 *
 */

#ifndef _GDEV_BJC_H
#define _GDEV_CDJ_H

/*
 * Drivers names. I don't expect you to change them!
 *
 */

#define BJC_BJC600		"bjc600"
#define BJC_BJC800		"bjc800"

#define BJC_BJC600_VERSION	2.1700
#define BJC_BJC600_VERSIONSTR	"2.17.00 5/23/96 Yves Arrouye"

#define BJC_BJC800_VERSION	2.1700
#define BJC_BJC800_VERSIONSTR	"2.17.00 5/23/96 Yves Arrouye"

/*
 * Hardware limits. May be adjusted eventually.
 *
 */

#define BJC_PRINT_LIMIT		(3. / 25.4) 		/* In inches. */
#define BJC_A3_PRINT_LIMIT	(8. / 25.4)             /* In inches. */

#define BJC_HARD_LOWER_LIMIT	(7. / 25.4)		/* In inches. */
#define BJC_USED_LOWER_LIMIT	(9.54 / 25.4)		/* In inches. */
#define BJC_RECD_LOWER_LIMIT	(12.7 / 25.4)		/* In inches. */

#ifdef USE_RECOMMENDED_MARGINS
#define BJC_LOWER_LIMIT		BJC_RECD_LOWER_LIMIT
#undef BJC_DEFAULT_CENTEREDAREA
#define BJC_DEFAULT_CENTEREDAREA
#else
#ifdef USE_TIGHT_MARGINS
#define BJC_LOWER_LIMIT		BJC_HARD_LOWER_LIMIT	/* In inches. */
#else
#define BJC_LOWER_LIMIT		BJC_USED_LOWER_LIMIT	/* In inches. */
#endif
#endif

#ifndef BJC600_MEDIAWEIGHT_THICKLIMIT
#define BJC600_MEDIAWEIGHT_THICKLIMIT	105		/* In g/m2. */
#endif
#ifndef BJC800_MEDIAWEIGHT_THICKLIMIT
#define BJC800_MEDIAWEIGHT_THICKLIMIT	BJC600_MEDIAWEIGHT_THICKLIMIT
#endif

#define BJC_HEAD_ROWS 64	/* Number of heads. Do not change! */

/*
 * Margins resulting from the limits specified above.
 *
 * The margins are Left, Bottom, Right, Top and are expressed in inches.
 * You should not change them, better change the limits above.
 *
 */

#define BJC_MARGINS_LETTER \
    (6.5 / 25.4), BJC_LOWER_LIMIT, (6.5 / 25.4), BJC_PRINT_LIMIT
#define BJC_MARGINS_A4 \
    (3.4 / 25.4), BJC_LOWER_LIMIT, (3.4 / 25.4), BJC_PRINT_LIMIT
#define BJC_MARGINS_A3 \
    (4.0 / 25.4), BJC_LOWER_LIMIT, (4.0 / 25.4), BJC_A3_PRINT_LIMIT

/*
 * Drivers options names.
 *
 */

#define BJC_DEVINFO_VERSION		"Version"
#define BJC_DEVINFO_VERSIONSTRING	"VersionString"

#define BJC_DEVINFO_OUTPUTFACEUP	"OutputFaceUp"

#define BJC_OPTION_MANUALFEED		"ManualFeed"
#define BJC_OPTION_DITHERINGTYPE	"DitheringType"
#define BJC_OPTION_MEDIATYPE		"MediaType"
#define BJC_OPTION_MEDIAWEIGHT		"MediaWeight"
#define BJC_OPTION_PRINTQUALITY		"PrintQuality"
#define BJC_OPTION_COLORCOMPONENTS	"ColorComponents"
#define BJC_OPTION_PRINTCOLORS		"PrintColors"
#define BJC_OPTION_MONOCHROMEPRINT	"MonochromePrint"

/*
 * Definitions of parameters (options) values.
 *
 */

#define BJC_MEDIA_PLAINPAPER		0
#define BJC_MEDIA_COATEDPAPER		1
#define BJC_MEDIA_TRANSPARENCYFILM	2
#define BJC_MEDIA_BACKPRINTFILM		3	/* Unused */
#define BJC_MEDIA_ENVELOPE		8
#define BJC_MEDIA_CARD			9
#define BJC_MEDIA_OTHER			15

#define BJC_DITHER_NONE			0
#define BJC_DITHER_FS			1

#define BJC_QUALITY_NORMAL		0
#define BJC_QUALITY_HIGH		1
#define BJC_QUALITY_DRAFT		2
#define BJC_QUALITY_LOW			3

#define BJC_COLOR_ALLBLACK		0
#define BJC_COLOR_CYAN			1
#define BJC_COLOR_MAGENTA		2
#define BJC_COLOR_YELLOW		4
#define BJC_COLOR_BLACK			8

#define BJC_COLOR_CMY	(BJC_COLOR_CYAN | BJC_COLOR_MAGENTA | BJC_COLOR_YELLOW)
#define BJC_COLOR_CMYK	(BJC_COLOR_CMY | BJC_COLOR_BLACK)

/* Some compilers complain if this is a floating point value.... */
#define BJC_RESOLUTION_BASE		90

#define BJC_RESOLUTION_LOW		(1 * BJC_RESOLUTION_BASE)
#define BJC_RESOLUTION_MEDIUM		(2 * BJC_RESOLUTION_BASE)
#define BJC_RESOLUTION_NORMAL		(4 * BJC_RESOLUTION_BASE)

/*
 * Default values for parameters (long).
 *
 * Generic values are first given, and driver-specific values are by default
 * those generic values.
 *
 */

#ifndef BJC_DEFAULT_MEDIATYPE
#define BJC_DEFAULT_MEDIATYPE		BJC_MEDIA_PLAINPAPER
#endif
#ifndef BJC_DEFAULT_PRINTQUALITY
#define BJC_DEFAULT_PRINTQUALITY	BJC_QUALITY_NORMAL
#endif

#ifndef BJC_DEFAULT_DITHERINGTYPE
#define BJC_DEFAULT_DITHERINGTYPE	BJC_DITHER_FS
#endif

#ifndef BJC_DEFAULT_MANUALFEED
#define BJC_DEFAULT_MANUALFEED		false
#endif
#ifndef BJC_DEFAULT_MONOCHROMEPRINT
#define BJC_DEFAULT_MONOCHROMEPRINT	false
#endif

#ifndef BJC_DEFAULT_RESOLUTION
#define BJC_DEFAULT_RESOLUTION		BJC_RESOLUTION_NORMAL
#endif

/* If you change the bits per pixel, change the color components. For
   bpp = 1 color components = 1, bpp = 8 color components = { 1, 4},
   bpp = { 16, 24, 32 } color components = 4, comps = { 3 }, bpp = { 24 }. */

#ifndef BJC_DEFAULT_BITSPERPIXEL
#define BJC_DEFAULT_BITSPERPIXEL	24
#endif
#ifndef BJC_DEFAULT_COLORCOMPONENTS
#define BJC_DEFAULT_COLORCOMPONENTS	4
#endif

/* You should not have to change these defaults */

#ifndef BJC_DEFAULT_PRINTCOLORS
#define BJC_DEFAULT_PRINTCOLORS		BJC_COLOR_CMYK
#endif
#ifndef BJC_DEFAULT_MONOCHROMEPRINT
#define BJC_DEFAULT_MONOCHROMEPRINT	false
#endif
#ifndef BJC_DEFAULT_SETMEDIAWEIGHT
#define BJC_DEFAULT_SETMEDIAWEIGHT	0
#endif
#ifndef BJC_DEFAULT_MEDIAWEIGHT
#define BJC_DEFAULT_MEDIAWEIGHT		80
#endif

/*
 * Default values for the specific BJC drivers.
 *
 */

#ifndef BJC600_DEFAULT_MEDIATYPE
#define BJC600_DEFAULT_MEDIATYPE        BJC_DEFAULT_MEDIATYPE
#endif
#ifndef BJC600_DEFAULT_PRINTQUALITY
#define BJC600_DEFAULT_PRINTQUALITY	BJC_DEFAULT_PRINTQUALITY
#endif
#ifndef BJC600_DEFAULT_DITHERINGTYPE
#define BJC600_DEFAULT_DITHERINGTYPE	BJC_DEFAULT_DITHERINGTYPE
#endif
#ifndef BJC600_DEFAULT_MANUALFEED
#define BJC600_DEFAULT_MANUALFEED	BJC_DEFAULT_MANUALFEED
#endif
#ifndef BJC600_DEFAULT_MONOCHROMEPRINT
#define BJC600_DEFAULT_MONOCHROMEPRINT	BJC_DEFAULT_MONOCHROMEPRINT
#endif
#ifndef BJC600_DEFAULT_RESOLUTION
#define BJC600_DEFAULT_RESOLUTION	BJC_DEFAULT_RESOLUTION
#endif
#ifndef BJC600_DEFAULT_BITSPERPIXEL
#define BJC600_DEFAULT_BITSPERPIXEL	BJC_DEFAULT_BITSPERPIXEL
#endif
#ifndef BJC600_DEFAULT_COLORCOMPONENTS
#define BJC600_DEFAULT_COLORCOMPONENTS	BJC_DEFAULT_COLORCOMPONENTS
#endif
#ifndef BJC600_DEFAULT_PRINTCOLORS
#define BJC600_DEFAULT_PRINTCOLORS	BJC_DEFAULT_PRINTCOLORS
#endif
#ifndef BJC600_DEFAULT_SETMEDIAWEIGHT
#define BJC600_DEFAULT_SETMEDIAWEIGHT	BJC_DEFAULT_SETMEDIAWEIGHT
#endif
#ifndef BJC600_DEFAULT_MEDIAWEIGHT
#define BJC600_DEFAULT_MEDIAWEIGHT	BJC_DEFAULT_MEDIAWEIGHT
#endif

#ifndef BJC800_DEFAULT_MEDIATYPE
#define BJC800_DEFAULT_MEDIATYPE        BJC_DEFAULT_MEDIATYPE
#endif
#ifndef BJC800_DEFAULT_PRINTQUALITY
#define BJC800_DEFAULT_PRINTQUALITY	BJC_DEFAULT_PRINTQUALITY
#endif
#ifndef BJC800_DEFAULT_DITHERINGTYPE
#define BJC800_DEFAULT_DITHERINGTYPE	BJC_DEFAULT_DITHERINGTYPE
#endif
#ifndef BJC800_DEFAULT_MANUALFEED
#define BJC800_DEFAULT_MANUALFEED	BJC_DEFAULT_MANUALFEED
#endif
#ifndef BJC800_DEFAULT_RESOLUTION
#define BJC800_DEFAULT_RESOLUTION	BJC_DEFAULT_RESOLUTION
#endif
#ifndef BJC800_DEFAULT_BITSPERPIXEL
#define BJC800_DEFAULT_BITSPERPIXEL	BJC_DEFAULT_BITSPERPIXEL
#endif
#ifndef BJC800_DEFAULT_COLORCOMPONENTS
#define BJC800_DEFAULT_COLORCOMPONENTS	BJC_DEFAULT_COLORCOMPONENTS
#endif
#ifndef BJC800_DEFAULT_PRINTCOLORS
#define BJC800_DEFAULT_PRINTCOLORS	BJC_DEFAULT_PRINTCOLORS
#endif
#ifndef BJC800_DEFAULT_SETMEDIAWEIGHT
#define BJC800_DEFAULT_SETMEDIAWEIGHT	BJC_DEFAULT_SETMEDIAWEIGHT
#endif
#ifndef BJC800_DEFAULT_MEDIAWEIGHT
#define BJC800_DEFAULT_MEDIAWEIGHT	BJC_DEFAULT_MEDIAWEIGHT
#endif

#endif /* _GDEVBJC_H */

#endif				/* gdevbjc_INCLUDED */
