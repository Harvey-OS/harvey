/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpxut.c,v 1.8 2005/07/11 22:08:30 stefan Exp $ */
/* Utilities for PCL XL generation */
#include "math_.h"
#include "string_.h"
#include "gx.h"
#include "stream.h"
#include "gxdevcli.h"
#include "gdevpxat.h"
#include "gdevpxen.h"
#include "gdevpxop.h"
#include "gdevpxut.h"
#include <assert.h>

/* ---------------- High-level constructs ---------------- */

/* Write the file header, including the resolution. */
int
px_write_file_header(stream *s, const gx_device *dev)
{
    static const char *const enter_pjl_header =
        "\033%-12345X@PJL SET RENDERMODE=";
    static const char *const rendermode_gray = "GRAYSCALE";
    static const char *const rendermode_color = "COLOR";
    static const char *const file_header =
	"\n@PJL ENTER LANGUAGE = PCLXL\n\
) HP-PCL XL;1;1;Comment Copyright artofcode LLC 2005\000\n";
    static const byte stream_header[] = {
	DA(pxaUnitsPerMeasure),
	DUB(0), DA(pxaMeasure),
	DUB(eBackChAndErrPage), DA(pxaErrorReport),
	pxtBeginSession,
	DUB(0), DA(pxaSourceType),
	DUB(eBinaryLowByteFirst), DA(pxaDataOrg),
	pxtOpenDataSource
    };

    px_put_bytes(s, (const byte *)enter_pjl_header,
		 strlen(enter_pjl_header));

    if (dev->color_info.num_components == 1)
	px_put_bytes(s, (const byte *)rendermode_gray,
		     strlen(rendermode_gray));
    else
	px_put_bytes(s, (const byte *)rendermode_color,
		     strlen(rendermode_color));

    /* We have to add 2 to the strlen because the next-to-last */
    /* character is a null. */
    px_put_bytes(s, (const byte *)file_header,
		 strlen(file_header) + 2);
    px_put_usp(s, (uint) (dev->HWResolution[0] + 0.5),
	       (uint) (dev->HWResolution[1] + 0.5));
    PX_PUT_LIT(s, stream_header);
    return 0;
}

/* Write the page header, including orientation. */
int
px_write_page_header(stream *s, const gx_device *dev)
{
    static const byte page_header_1[] = {
	DUB(ePortraitOrientation), DA(pxaOrientation)
    };

    PX_PUT_LIT(s, page_header_1);
    return 0;
}

/* Write the media selection command if needed, updating the media size. */
int
px_write_select_media(stream *s, const gx_device *dev, 
		      pxeMediaSize_t *pms, byte *media_source)
{
#define MSD(ms, res, w, h)\
  { ms, (float)((w) * 1.0 / (res)), (float)((h) * 1.0 / res) },
    static const struct {
	pxeMediaSize_t ms;
	float width, height;
    } media_sizes[] = {
	px_enumerate_media(MSD)
	{ pxeMediaSize_next }
    };
#undef MSD
    float w = dev->width / dev->HWResolution[0],
	h = dev->height / dev->HWResolution[1];
    int i;
    pxeMediaSize_t size;
    byte tray = eAutoSelect;

    /* The default is eLetterPaper, media size 0. */
    for (i = countof(media_sizes) - 2; i > 0; --i)
	if (fabs(media_sizes[i].width - w) < 5.0 / 72 &&
	    fabs(media_sizes[i].height - h) < 5.0 / 72
	    )
	    break;
    size = media_sizes[i].ms;
    /*
     * According to the PCL XL documentation, MediaSize must always
     * be specified, but MediaSource is optional.
     */
    px_put_uba(s, (byte)size, pxaMediaSize);

    if (media_source != NULL)
	tray = *media_source;
    px_put_uba(s, tray, pxaMediaSource);
    if (pms)
	*pms = size;

    return 0;
}

/*
 * Write the file trailer.  Note that this takes a FILE *, not a stream *,
 * since it may be called after the stream is closed.
 */
int
px_write_file_trailer(FILE *file)
{
    static const byte file_trailer[] = {
	pxtCloseDataSource,
	pxtEndSession,
	033, '%', '-', '1', '2', '3', '4', '5', 'X'
    };

    fwrite(file_trailer, 1, sizeof(file_trailer), file);
    return 0;
}

/* ---------------- Low-level data output ---------------- */

/* Write a sequence of bytes. */
void
px_put_bytes(stream * s, const byte * data, uint count)
{
    uint used;

    sputs(s, data, count, &used);
}

/* Utilities for writing data values. */
/* H-P printers only support little-endian data, so that's what we emit. */
void
px_put_a(stream * s, px_attribute_t a)
{
    sputc(s, pxt_attr_ubyte);
    sputc(s, (byte)a);
}
void
px_put_ac(stream *s, px_attribute_t a, px_tag_t op)
{
    px_put_a(s, a);
    sputc(s, (byte)op);
}

void
px_put_ub(stream * s, byte b)
{
    sputc(s, pxt_ubyte);
    sputc(s, b);
}
void
px_put_uba(stream *s, byte b, px_attribute_t a)
{
    px_put_ub(s, b);
    px_put_a(s, a);
}

void
px_put_s(stream * s, uint i)
{
    sputc(s, (byte) i);
    sputc(s, (byte) (i >> 8));
}
void
px_put_us(stream * s, uint i)
{
    sputc(s, pxt_uint16);
    px_put_s(s, i);
}
void
px_put_usa(stream *s, uint i, px_attribute_t a)
{
    px_put_us(s, i);
    px_put_a(s, a);
}
void
px_put_u(stream * s, uint i)
{
    if (i <= 255)
	px_put_ub(s, (byte)i);
    else
	px_put_us(s, i);
}

void
px_put_usp(stream * s, uint ix, uint iy)
{
    spputc(s, pxt_uint16_xy);
    px_put_s(s, ix);
    px_put_s(s, iy);
}
void
px_put_usq_fixed(stream * s, fixed x0, fixed y0, fixed x1, fixed y1)
{
    spputc(s, pxt_uint16_box);
    px_put_s(s, fixed2int(x0));
    px_put_s(s, fixed2int(y0));
    px_put_s(s, fixed2int(x1));
    px_put_s(s, fixed2int(y1));
}

void
px_put_ss(stream * s, int i)
{
    sputc(s, pxt_sint16);
    px_put_s(s, (uint) i);
}
void
px_put_ssp(stream * s, int ix, int iy)
{
    sputc(s, pxt_sint16_xy);
    px_put_s(s, (uint) ix);
    px_put_s(s, (uint) iy);
}

void
px_put_l(stream * s, ulong l)
{
    px_put_s(s, (uint) l);
    px_put_s(s, (uint) (l >> 16));
}

void
px_put_r(stream * s, floatp r)
{				/* Convert to single-precision IEEE float. */
    int exp;
    long mantissa = (long)(frexp(r, &exp) * 0x1000000);

    if (exp < -126)
	mantissa = 0, exp = 0;	/* unnormalized */
    if (mantissa < 0)
	exp += 128, mantissa = -mantissa;
    /* All quantities are little-endian. */
    spputc(s, (byte) mantissa);
    spputc(s, (byte) (mantissa >> 8));
    spputc(s, (byte) (((exp + 127) << 7) + ((mantissa >> 16) & 0x7f)));
    spputc(s, (byte) ((exp + 127) >> 1));
}
void
px_put_rl(stream * s, floatp r)
{
    spputc(s, pxt_real32);
    px_put_r(s, r);
}

void
px_put_data_length(stream * s, uint num_bytes)
{
    if (num_bytes > 255) {
	spputc(s, pxt_dataLength);
	px_put_l(s, (ulong) num_bytes);
    } else {
	spputc(s, pxt_dataLengthByte);
	spputc(s, (byte) num_bytes);
    }
}
