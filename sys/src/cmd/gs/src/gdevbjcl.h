/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevbjcl.h,v 1.6 2002/06/16 07:25:26 lpd Exp $*/
/* Canon BJC command generation library interface */

/****** PRELIMINARY, SUBJECT TO CHANGE WITHOUT NOTICE. ******/

#ifndef gdevbjcl_INCLUDED
#  define gdevbjcl_INCLUDED

#include <stdio.h>			/* ****** PATCH FOR stream.h ****** */
#include "stream.h"

/*
 * These procedures generate command strings for the Canon BJC family of
 * printers.  Note that not all printers support all commands.
 */

/* ---------------- Printer capabilities ---------------- */

/*
 * Different printer models implement different subsets of the command set.
 * We define a mask bit for each capability, and a mask for each printer
 * indicating which capabilities it supports.  In some cases, a capability
 * is a parameter value for a command rather than a separate command.
 */

/*
 * Single-character commands.
 *
 * All BJC models implement CR and FF.
 */
#define BJC_OPT_NUL 0x00000001
#define BJC_OPT_LF  0x00000002

/*
 * Session commands.
 *
 * All BJC models implement Set initial condition, Initialize,
 * Print method, and Media Supply.
 */
#define BJC_OPT_IDENTIFY_CARTRIDGE   0x00000004
#define BJC_OPT_MONOCHROME_SMOOTHING 0x00000008	/* for bjc_print_color_t */

/*
 * Page commands.
 *
 * All BJC models implement Page margins.
 */
#define BJC_OPT_EXTENDED_MARGINS 0x00000010
#define BJC_OPT_PAGE_ID          0x00000020

/*
 * Resolution.  This varies considerably from model to model.
 * The _300 or _360 option gives the base resolution; the other options
 * indicate which multiples of the base are available.
 * Note that the resolution multipliers are specified as X, then Y.
 */
#define BJC_OPT_RESOLUTION_360     0x00000040
#define BJC_OPT_RESOLUTION_300     0x00000080
#define BJC_OPT_RESOLUTION_HALF    0x00000100 /* 180 or 150 */
#define BJC_OPT_RESOLUTION_QUARTER 0x00000200 /* 90 or 75 */
#define BJC_OPT_RESOLUTION_2X      0x00000400 /* 720 or 600 */
#define BJC_OPT_RESOLUTION_2X_1X   0x00000800
#define BJC_OPT_RESOLUTION_4X_2X   0x00001000

/*
 * Image commands.
 *
 * All BJC models implement Raster resolution, Raster skip, CMYK image,
 * and Data compression.
 */
#define BJC_OPT_X_Y_RESOLUTION      0x00002000	/* for raster_resolution */
#define BJC_OPT_MOVE_LINES          0x00004000
#define BJC_OPT_IMAGE_FORMAT        0x00008000
#define BJC_OPT_CONTINUE_IMAGE      0x00010000
#define BJC_OPT_INDEXED_IMAGE       0x00020000
#define BJC_OPT_SET_COLOR_COMPONENT 0x00040000

/*
 * Define the capabilities of the models that we know about.
 */
/*
 * We don't have the documentation for the 50, but Canon says it's the
 * same as the 80.
 */
#define BJC_OPT_50\
  (BJC_OPT_NUL | BJC_OPT_LF |\
   BJC_OPT_IDENTIFY_CARTRIDGE |\
   BJC_OPT_EXTENDED_MARGINS | BJC_OPT_PAGE_ID |\
   BJC_OPT_RESOLUTION_360 | BJC_OPT_RESOLUTION_HALF |\
     BJC_OPT_RESOLUTION_QUARTER | BJC_OPT_RESOLUTION_2X_1X |\
   BJC_OPT_X_Y_RESOLUTION | BJC_OPT_MOVE_LINES | BJC_OPT_IMAGE_FORMAT |\
     BJC_OPT_CONTINUE_IMAGE)
#define BJC_OPT_70\
  (BJC_OPT_LF |\
   BJC_OPT_IDENTIFY_CARTRIDGE | BJC_OPT_MONOCHROME_SMOOTHING |\
   BJC_OPT_RESOLUTION_360)
#define BJC_OPT_80\
  BJC_OPT_50
#define BJC_OPT_210\
  (BJC_OPT_IDENTIFY_CARTRIDGE | BJC_OPT_MONOCHROME_SMOOTHING |\
   BJC_OPT_EXTENDED_MARGINS |\
   BJC_OPT_RESOLUTION_360 |\
   BJC_OPT_X_Y_RESOLUTION | BJC_OPT_MOVE_LINES | BJC_OPT_CONTINUE_IMAGE)
#define BJC_OPT_250\
  (BJC_OPT_LF |\
   BJC_OPT_IDENTIFY_CARTRIDGE | BJC_OPT_MONOCHROME_SMOOTHING |\
   BJC_OPT_EXTENDED_MARGINS | BJC_OPT_PAGE_ID |\
   BJC_OPT_RESOLUTION_360 | BJC_OPT_RESOLUTION_HALF |\
     BJC_OPT_RESOLUTION_QUARTER | BJC_OPT_RESOLUTION_2X_1X |\
   BJC_OPT_X_Y_RESOLUTION | BJC_OPT_MOVE_LINES | BJC_OPT_IMAGE_FORMAT |\
     BJC_OPT_CONTINUE_IMAGE)
#define BJC_OPT_610\
  (BJC_OPT_LF |\
   BJC_OPT_RESOLUTION_360)
#define BJC_OPT_620\
  BJC_OPT_610
#define BJC_OPT_4000\
  (BJC_OPT_LF |\
   BJC_OPT_IDENTIFY_CARTRIDGE | BJC_OPT_MONOCHROME_SMOOTHING |\
   BJC_OPT_RESOLUTION_360 | BJC_OPT_RESOLUTION_HALF |\
     BJC_OPT_RESOLUTION_QUARTER)
#define BJC_OPT_4100\
  (BJC_OPT_IDENTIFY_CARTRIDGE |\
   BJC_OPT_EXTENDED_MARGINS |\
   BJC_OPT_RESOLUTION_360 |\
   BJC_OPT_MOVE_LINES | BJC_OPT_CONTINUE_IMAGE)
#define BJC_OPT_4200\
  (BJC_OPT_LF |\
   BJC_OPT_IDENTIFY_CARTRIDGE |\
   BJC_OPT_EXTENDED_MARGINS | BJC_OPT_PAGE_ID |\
   BJC_OPT_RESOLUTION_360 |\
   BJC_OPT_MOVE_LINES | BJC_OPT_IMAGE_FORMAT | BJC_OPT_CONTINUE_IMAGE)
#define BJC_OPT_4300\
  BJC_OPT_250
#define BJC_OPT_4550\
  BJC_OPT_250
#define BJC_OPT_4650\
  BJC_OPT_250
#define BJC_OPT_5500\
  (BJC_OPT_IDENTIFY_CARTRIDGE | BJC_OPT_MONOCHROME_SMOOTHING |\
   BJC_OPT_EXTENDED_MARGINS |\
   BJC_OPT_RESOLUTION_360 |\
   BJC_OPT_MOVE_LINES | BJC_OPT_CONTINUE_IMAGE)
/* The 7000 is not well documented.  The following is a semi-guess. */
#define BJC_OPT_7000\
  (BJC_OPT_NUL | BJC_OPT_LF |\
   BJC_OPT_IDENTIFY_CARTRIDGE |\
   BJC_OPT_EXTENDED_MARGINS | BJC_OPT_PAGE_ID |\
   BJC_OPT_RESOLUTION_300 | BJC_OPT_RESOLUTION_2X_1X |\
     BJC_OPT_RESOLUTION_4X_2X |\
   BJC_OPT_MOVE_LINES | BJC_OPT_IMAGE_FORMAT | BJC_OPT_CONTINUE_IMAGE |\
     BJC_OPT_INDEXED_IMAGE | BJC_OPT_SET_COLOR_COMPONENT)

/*
 * Enumerate the options for all the printer models we know about.
 * m(x, y) will normally be {x, y}, to generate a table.
 */
#define BJC_ENUMERATE_OPTIONS(m)\
  m(50, BJC_OPT_50)\
  m(70, BJC_OPT_70)\
  m(80, BJC_OPT_80)\
  m(210, BJC_OPT_210)\
  m(250, BJC_OPT_250)\
  m(610, BJC_OPT_610)\
  m(620, BJC_OPT_620)\
  m(4000, BJC_OPT_4000)\
  m(4100, BJC_OPT_4100)\
  m(4200, BJC_OPT_4200)\
  m(4300, BJC_OPT_4300)\
  m(4550, BJC_OPT_4550)\
  m(4650, BJC_OPT_4650)\
  m(5500, BJC_OPT_5500)\
  m(7000, BJC_OPT_7000)

/* ---------------- Command generation ---------------- */

/*
 * Single-character commands.
 */

/* Carriage return (^M) */
void bjc_put_CR(stream *s);

/* Form feed (^L) */
void bjc_put_FF(stream *s);

/* Line feed (^J) */
void bjc_put_LF(stream *s);

/*
 * Session commands.
 */

/* Set initial condition */
void bjc_put_initial_condition(stream *s);

/* Return to initial condition */
void bjc_put_initialize(stream *s);

/* Select print method */
/****** DIFFERENT FOR 7000 ******/
typedef enum {
    BJC_PRINT_COLOR_COLOR = 0x0,
    BJC_PRINT_COLOR_MONOCHROME = 0x1,
    BJC_PRINT_COLOR_MONOCHROME_WITH_SMOOTHING = 0x2	/* option */
} bjc_print_color_t;
typedef enum {
    BJC_PRINT_MEDIA_PLAIN_PAPER = 0x0,
    BJC_PRINT_MEDIA_COATED_PAPER = 0x1,
    BJC_PRINT_MEDIA_TRANSPARENCY_FILM = 0x2,
    BJC_PRINT_MEDIA_BACK_PRINT_FILM = 0x3,
    BJC_PRINT_MEDIA_TEXTILE_SHEET = 0x4,
    BJC_PRINT_MEDIA_GLOSSY_PAPER = 0x5,
    BJC_PRINT_MEDIA_HIGH_GLOSS_FILM = 0x6,
    BJC_PRINT_MEDIA_HIGH_RESOLUTION_PAPER = 0x7	/* BJC-80 only */
} bjc_print_media_t;
typedef enum {
    BJC_PRINT_QUALITY_NORMAL = 0x0,
    BJC_PRINT_QUALITY_HIGH = 0x1,
    BJC_PRINT_QUALITY_DRAFT = 0x2,
    BJC_PRINT_QUALITY_COLOR_NON_BLEED = 0x8	/* not 6x0 */
} bjc_print_quality_t;
typedef enum {
    /* 6x0 only */
    BJC_BLACK_DENSITY_NORMAL = 0x0,
    BJC_BLACK_DENSITY_HIGH = 0x1
} bjc_black_density_t;
void bjc_put_print_method(stream *s, bjc_print_color_t color,
			  bjc_print_media_t media,
			  bjc_print_quality_t quality,
			  bjc_black_density_t density);
typedef enum {
    /* 70, 4000, 4550, 4650 */
    BJC_70_PRINT_COLOR_SHORT_FINE = 0x0,		/* also 0x1, 0x2 */
    BJC_70_PRINT_COLOR_SHORT_HQ = 0x3,
    BJC_70_PRINT_COLOR_SHORT_ECO = 0x4,
    /* 80, 250, 4200, 4300 */
    BJC_80_PRINT_COLOR_SHORT_STD = 0x0,
    BJC_80_PRINT_COLOR_SHORT_STD_SPECIALTY = 0x1,
    BJC_80_PRINT_COLOR_SHORT_HQ_SPECIALTY = 0x2,
    BJC_80_PRINT_COLOR_SHORT_HQ = 0x3,
    BJC_80_PRINT_COLOR_SHORT_HIGH_SPEED = 0x4,
    /* 210, 4100 */
    BJC_210_PRINT_COLOR_SHORT_HQ = 0x0,	/* also 0x1 */
    BJC_210_PRINT_COLOR_SHORT_FINE = 0x2,	/* also 0x3 */
    BJC_210_PRINT_COLOR_SHORT_HIGH_SPEED = 0x4,
    /* 5500 */
    BJC_5500_PRINT_COLOR_SHORT_COATED = 0x0,
    BJC_5500_PRINT_COLOR_SHORT_TRANSPARENCY = 0x1,
    BJC_5500_PRINT_COLOR_SHORT_PLAIN = 0x2,
    BJC_5500_PRINT_COLOR_SHORT_HQ_NON_BLEED = 0x3,
    BJC_5500_PRINT_COLOR_SHORT_HIGH_SPEED = 0x4
} bjc_print_color_short_t;
void bjc_put_print_method_short(stream *s, bjc_print_color_short_t color);

/* Set media supply method */
/****** DIFFERENT FOR 7000 ******/
typedef enum {
    /* 70, 210, 250, 6x0, 4100 */
    BJC_70_MEDIA_SUPPLY_MANUAL_1 = 0x0,
    BJC_70_MEDIA_SUPPLY_MANUAL_2 = 0x1,
    BJC_70_MEDIA_SUPPLY_ASF = 0x4,
    /* 250, 4000, 4300, 4650, 5500 */
    BJC_250_MEDIA_SUPPLY_CONTINUOUS_FORM = 0x2,
    BJC_250_MEDIA_SUPPLY_ASF_BIN_2 = 0x5,
    /* 250, 4650, 5500 */
    BJC_250_MEDIA_SUPPLY_AUTO_SWITCH = 0xf,
    /* 4000, 4300, 4650 */
    BJC_4000_MEDIA_SUPPLY_CASSETTE = 0x8,
    /* 80 */
    BJC_80_MEDIA_SUPPLY_ASF_OFFLINE = 0x0,
    BJC_80_MEDIA_SUPPLY_ASF_ONLINE = 0x1		/* also 0x4 */
} bjc_media_supply_t;
typedef enum {
    BJC_MEDIA_TYPE_PLAIN_PAPER = 0x0,
    BJC_MEDIA_TYPE_COATED_PAPER = 0x1,
    BJC_MEDIA_TYPE_TRANSPARENCY_FILM = 0x2,
    BJC_MEDIA_TYPE_BACK_PRINT_FILM = 0x3,
    BJC_MEDIA_TYPE_PAPER_WITH_LEAD = 0x4,
    BJC_MEDIA_TYPE_TEXTILE_SHEET = 0x5,
    BJC_MEDIA_TYPE_GLOSSY_PAPER = 0x6,
    BJC_MEDIA_TYPE_HIGH_GLOSS_FILM = 0x7,
    BJC_MEDIA_TYPE_ENVELOPE = 0x8,
    BJC_MEDIA_TYPE_CARD = 0x9,
    BJC_MEDIA_TYPE_HIGH_RESOLUTION_6X0 = 0xa,	/* 6x0 only */
    BJC_MEDIA_TYPE_HIGH_RESOLUTION = 0xb,	/* 720x720, other models */
    BJC_MEDIA_TYPE_FULL_BLEED = 0xc,
    BJC_MEDIA_TYPE_BANNER = 0xd
} bjc_media_type_t;
void bjc_put_media_supply(stream *s, bjc_media_supply_t supply,
			  bjc_media_type_t type);

/* Identify ink cartridge */
typedef enum {
    BJC_IDENTIFY_CARTRIDGE_PREPARE = 0x0,
    BJC_IDENTIFY_CARTRIDGE_REQUEST = 0x1
} bjc_identify_cartridge_command_t;
void bjc_put_identify_cartridge(stream *s,
				bjc_identify_cartridge_command_t command);

/*
 * Page commands.
 */

/* Set page margins */
/* Left margin is 1-origin; margins are both from left edge; indent <= 8 */
void bjc_put_page_margins(stream *s, int length10ths, int lm10ths,
			  int rm10ths, int indent60ths);

/* Set extended margins */
/* All values are 0-origin; margins are both from left edge; indent <= 8 */
void bjc_put_extended_margins(stream *s, int length60ths, int lm60ths,
			      int rm60ths, int indent60ths);

/* Page ID */
/* 0 <= id <= 127 */
void bjc_put_page_id(stream *s, int id);

/*
 * Image commands.
 */

/* Set raster compression */
typedef enum {
    BJC_RASTER_COMPRESSION_NONE = 0x0,
    BJC_RASTER_COMPRESSION_PACKBITS = 0x1
} bjc_raster_compression_t;
void bjc_put_compression(stream *s, bjc_raster_compression_t compression);

/* Set raster resolution */
void bjc_put_raster_resolution(stream *s, int x_resolution, int y_resolution);

/* Raster skip */
/* Maximum skip on 6x0 and 4000 is 0x17ff */
void bjc_put_raster_skip(stream *s, int skip);

/* CMYK raster image */
typedef enum {
    BJC_CMYK_IMAGE_CYAN = 'C',
    BJC_CMYK_IMAGE_MAGENTA = 'M',
    BJC_CMYK_IMAGE_YELLOW = 'Y',
    BJC_CMYK_IMAGE_BLACK = 'K',
} bjc_cmyk_image_component_t;
void bjc_put_cmyk_image(stream *s, bjc_cmyk_image_component_t component,
			const byte *data, int count);

/* Move by raster lines */
/* Distance must be a multiple of the raster resolution */
void bjc_put_move_lines(stream *s, int lines);

/* Set unit for movement by raster lines */
/* unit = 360 for printers other than 7000 */
/* unit = 300 or 600 for 7000 */
void bjc_put_move_lines_unit(stream *s, int unit);

/* Set image format */
/* depth is 1 or 2 */
/****** DIFFERENT FOR 7000 ******/
typedef enum {
    BJC_IMAGE_FORMAT_REGULAR = 0x00,
    BJC_IMAGE_FORMAT_INDEXED = 0x80
} bjc_image_format_t;
typedef enum {
    BJC_INK_SYSTEM_REGULAR = 0x01,
    BJC_INK_SYSTEM_PHOTO = 0x02,
    BJC_INK_SYSTEM_REGULAR_DVM = 0x09,	/* drop volume modulation */
    BJC_INK_SYSTEM_PHOTO_DVM = 0x0a	/* drop volume modulation */
} bjc_ink_system_t;
void bjc_put_image_format(stream *s, int depth,
			  bjc_image_format_t format,
			  bjc_ink_system_t ink);
/* 4550 only */
void bjc_put_photo_image(stream *s, bool photo);

/* Continue raster image */
void bjc_put_continue_image(stream *s, const byte *data, int count);

/* BJ indexed image */
void bjc_put_indexed_image(stream *s, int dot_rows, int dot_cols, int layers);

#endif				/* gdevbjcl_INCLUDED */
