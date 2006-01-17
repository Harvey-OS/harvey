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

/* $Id: gdevbjcl.c,v 1.4 2002/02/21 22:24:51 giles Exp $*/
/* Canon BJC command generation library */
#include "std.h"
#include "gdevbjcl.h"

/****** PRELIMINARY, SUBJECT TO CHANGE WITHOUT NOTICE. ******/

/* ---------------- Utilities ---------------- */

private void
bjc_put_bytes(stream *s, const byte *data, uint count)
{
    uint ignore;

    sputs(s, data, count, &ignore);
}

private void
bjc_put_hi_lo(stream *s, int value)
{
    spputc(s, value >> 8);
    spputc(s, value & 0xff);
}

private void
bjc_put_lo_hi(stream *s, int value)
{
    spputc(s, value & 0xff);
    spputc(s, value >> 8);
}

private void
bjc_put_command(stream *s, int ch, int count)
{
    spputc(s, 033 /*ESC*/);
    spputc(s, '(');
    spputc(s, ch);
    bjc_put_lo_hi(s, count);
}

/* ---------------- Commands ---------------- */

/* Line feed (^J) */
void
bjc_put_LF(stream *s)
{
    spputc(s, 0x0a);
}

/* Form feed (^L) */
void
bjc_put_FF(stream *s)
{
    spputc(s, 0x0c);
}

/* Carriage return (^M) */
void
bjc_put_CR(stream *s)
{
    spputc(s, 0x0d);
}

/* Return to initial condition (ESC @) */
void
bjc_put_initialize(stream *s)
{
    bjc_put_bytes(s, (const byte *)"\033@", 2);
}

/* Set initial condition (ESC [ K <count> <init> <id> <parm1> <parm2>) */
void
bjc_put_set_initial(stream *s)
{
    bjc_put_bytes(s, (const byte *)"\033[K\002\000\000\017", 7);
}

/* Set data compression (ESC [ b <count> <state>) */
void
bjc_put_set_compression(stream *s, bjc_raster_compression_t compression)
{
    bjc_put_command(s, 'b', 1);
    spputc(s, compression);
}

/* Select print method (ESC ( c <count> <parm1> <parm2> [<parm3>]) */
void
bjc_put_print_method_short(stream *s, bjc_print_color_short_t color)
{
    bjc_put_command(s, 'c', 1);
    spputc(s, color);
}
void
bjc_put_print_method(stream *s, bjc_print_color_t color,
		     bjc_print_media_t media, bjc_print_quality_t quality,
		     bjc_black_density_t density)
{
    bjc_put_command(s, 'c', 2 + (density != 0));
    spputc(s, 0x10 | color);
    spputc(s, (media << 4) | quality);
    if (density)
	spputc(s, density << 4);
}

/* Set raster resolution (ESC ( d <count> <y_res> [<x_res>]) */
void
bjc_put_raster_resolution(stream *s, int x_resolution, int y_resolution)
{
    if (x_resolution == y_resolution) {
	bjc_put_command(s, 'd', 2);
    } else {
	bjc_put_command(s, 'd', 4);
	bjc_put_hi_lo(s, y_resolution);
    }
    bjc_put_hi_lo(s, x_resolution);
}

/* Raster skip (ESC ( e <count> <skip>) */
void
bjc_put_raster_skip(stream *s, int skip)
{
    bjc_put_command(s, 'e', 2);
    bjc_put_hi_lo(s, skip);
}

/* Set page margins (ESC ( g <count> <length> <lm> <rm> <top>) */
void
bjc_put_page_margins(stream *s, int length, int lm, int rm, int top)
{
    byte parms[4];
    int count;

    parms[0] = length, parms[1] = lm, parms[2] = rm, parms[3] = top;
    count = 4;		/* could be 1..3 */
    bjc_put_command(s, 'g', count);
    bjc_put_bytes(s, parms, count);
}

/* Set media supply method (ESC * l <count> <parm1> <parm2>) */
void
bjc_put_media_supply(stream *s, bjc_media_supply_t supply,
		     bjc_media_type_t type)
{
    bjc_put_command(s, 'l', 2);
    spputc(s, 0x10 | supply);
    spputc(s, type << 4);
}

/* Identify ink cartridge (ESC ( m <count> <type>) */
void
bjc_put_identify_cartridge(stream *s,
			   bjc_identify_cartridge_command_t command)
{
    bjc_put_command(s, 'm', 1);
    spputc(s, command);
}

/* CMYK raster image (ESC ( A <count> <color>) */
void
bjc_put_cmyk_image(stream *s, bjc_cmyk_image_component_t component,
		   const byte *data, int count)
{
    bjc_put_command(s, 'A', count + 1);
    spputc(s, component);
    bjc_put_bytes(s, data, count);
}

/* Move by raster lines (ESC ( n <count> <lines>) */
void
bjc_put_move_lines(stream *s, int lines)
{
    bjc_put_command(s, 'n', 2);
    bjc_put_hi_lo(s, lines);
}

/* Set unit for movement by raster lines (ESC ( o <count> <unit>) */
void
bjc_put_move_lines_unit(stream *s, int unit)
{
    bjc_put_command(s, 'o', 2);
    bjc_put_hi_lo(s, unit);
}

/* Set extended margins (ESC ( p <count> <length60ths> <lm60ths> */
/*   <rm60ths> <top60ths>) */
void
bjc_put_extended_margins(stream *s, int length, int lm, int rm, int top)
{
    bjc_put_command(s, 'p', 8);
    bjc_put_hi_lo(s, length);
    bjc_put_hi_lo(s, lm);
    bjc_put_hi_lo(s, rm);
    bjc_put_hi_lo(s, top);
}

/* Set image format (ESC ( t <count> <depth> <format> <ink>) */
void
bjc_put_image_format(stream *s, int depth, bjc_image_format_t format,
			     bjc_ink_system_t ink)

{
    bjc_put_command(s, 't', 3);
    spputc(s, depth);
    spputc(s, format);
    spputc(s, ink);
}

/* Page ID (ESC ( q <count> <id>) */
void
bjc_put_page_id(stream *s, int id)
{
    bjc_put_command(s, 'q', 1);
    spputc(s, id);
}

/* Continue raster image (ESC ( F <count> <data>) */
void
bjc_put_continue_image(stream *s, const byte *data, int count)
{
    bjc_put_command(s, 'F', count);
    bjc_put_bytes(s, data, count);
}

/* BJ indexed image (ESC ( f <count> R <dot_rows> <dot_cols> <layers> */
/*   <index>) */
void
bjc_put_indexed_image(stream *s, int dot_rows, int dot_cols, int layers)
{
    bjc_put_command(s, 'f', 5);
    spputc(s, 'R');			/* per spec */
    spputc(s, dot_rows);
    spputc(s, dot_cols);
    spputc(s, layers);
}
