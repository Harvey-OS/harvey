/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*$Id: gdevpxut.h,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Utilities for PCL XL generation */
/* Requires gdevpxat.h, gdevpxen.h, gdevpxop.h */

#ifndef gdevpxut_INCLUDED
#  define gdevpxut_INCLUDED

/* ---------------- High-level constructs ---------------- */

/* Write the file header, including the resolution. */
int px_write_file_header(P2(stream *s, const gx_device *dev));

/* Write the page header, including orientation. */
int px_write_page_header(P2(stream *s, const gx_device *dev));

/* Write the media selection command if needed, updating the media size. */
int px_write_select_media(P3(stream *s, const gx_device *dev,
			     pxeMediaSize_t *pms));

/*
 * Write the file trailer.  Note that this takes a FILE *, not a stream *,
 * since it may be called after the stream is closed.
 */
int px_write_file_trailer(P1(FILE *file));

/* ---------------- Low-level data output ---------------- */

/* Write a sequence of bytes. */
#define PX_PUT_LIT(s, bytes) px_put_bytes(s, bytes, sizeof(bytes))
void px_put_bytes(P3(stream * s, const byte * data, uint count));

/* Utilities for writing data values. */
/* H-P printers only support little-endian data, so that's what we emit. */

#define DA(a) pxt_attr_ubyte, (a)
void px_put_a(P2(stream * s, px_attribute_t a));
void px_put_ac(P3(stream *s, px_attribute_t a, px_tag_t op));

#define DUB(b) pxt_ubyte, (byte)(b)
void px_put_ub(P2(stream * s, byte b));
void px_put_uba(P3(stream *s, byte b, px_attribute_t a));

#define DS(i) (byte)(i), (byte)((i) >> 8)
void px_put_s(P2(stream * s, uint i));

#define DUS(i) pxt_uint16, DS(i)
void px_put_us(P2(stream * s, uint i));
void px_put_usa(P3(stream *s, uint i, px_attribute_t a));
void px_put_u(P2(stream * s, uint i));

#define DUSP(ix,iy) pxt_uint16_xy, DS(ix), DS(iy)
void px_put_usp(P3(stream * s, uint ix, uint iy));
void px_put_usq_fixed(P5(stream * s, fixed x0, fixed y0, fixed x1, fixed y1));

void px_put_ss(P2(stream * s, int i));
void px_put_ssp(P3(stream * s, int ix, int iy));

void px_put_l(P2(stream * s, ulong l));

void px_put_r(P2(stream * s, floatp r));  /* no tag */
void px_put_rl(P2(stream * s, floatp r));  /* pxt_real32 tag */

void px_put_data_length(P2(stream * s, uint num_bytes));

#endif /* gdevpxut_INCLUDED */
