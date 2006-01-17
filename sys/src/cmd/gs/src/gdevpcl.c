/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpcl.c,v 1.8 2002/08/22 07:12:28 henrys Exp $ */
/* Utilities for PCL printers */
#include "gdevprn.h"
#include "gdevpcl.h"
#include "math_.h"

/* ------ Get paper size ------ */

/* Get the paper size code, based on width and height. */
int
gdev_pcl_paper_size(gx_device * dev)
{
    float width_inches = dev->width / dev->x_pixels_per_inch;
    float height_inches = dev->height / dev->y_pixels_per_inch;
    /* The initial value for height_difference, and for code below, is
       unnecessary and is here just to stop the compiler from
       complaining about a possible uninitialized variable usage. */
    float width_difference = -1.0, height_difference = -1.0;
    float new_width_difference, new_height_difference;
    int code = PAPER_SIZE_LETTER;

    /* Since we're telling the printer when to eject and start a new
       page, the paper height doesn't matter a great deal, as long as
       we ensure that it's at least as high as we want our pages to
       be.  However, the paper width is important, because on printers
       which center the paper in the input tray, having the wrong
       width will cause the image to appear in the wrong place on the
       paper (perhaps even partially missing the paper completely).
       Therefore, we choose our paper size by finding one whose width
       and height are both equal to or greater than the desired width
       and height and whose width is the closest to what we want.  We
       only pay close attention to the height when the widths of two
       different paper sizes are equal.

       We use "foo - bar > -0.01" instead of "foo >= bar" to avoid
       minor floating point and rounding errors.
    */
#define CHECK_PAPER_SIZE(w,h,c)							\
    new_width_difference = w - width_inches;					\
    new_height_difference = h - height_inches;					\
    if ((new_width_difference > -0.01) && (new_height_difference > -0.01) &&	\
	((width_difference == -1.0) ||						\
	 (new_width_difference < width_difference) ||				\
	 ((new_width_difference == width_difference) &&				\
	  (new_height_difference < height_difference)))) {			\
      width_difference = new_width_difference;					\
      height_difference = new_height_difference;				\
      code = c;									\
    }

    CHECK_PAPER_SIZE( 7.25, 10.5,  PAPER_SIZE_EXECUTIVE);
    CHECK_PAPER_SIZE( 8.5 , 11.0,  PAPER_SIZE_LETTER);
    CHECK_PAPER_SIZE( 8.5 , 14.0,  PAPER_SIZE_LEGAL);
    CHECK_PAPER_SIZE(11.0 , 17.0,  PAPER_SIZE_LEDGER);
    CHECK_PAPER_SIZE( 8.27, 11.69, PAPER_SIZE_A4);
    CHECK_PAPER_SIZE(11.69, 16.54, PAPER_SIZE_A3);
    CHECK_PAPER_SIZE(16.54, 23.39, PAPER_SIZE_A2);
    CHECK_PAPER_SIZE(23.39, 33.11, PAPER_SIZE_A1);
    CHECK_PAPER_SIZE(33.11, 46.81, PAPER_SIZE_A0);
    CHECK_PAPER_SIZE( 7.16, 10.12, PAPER_SIZE_JIS_B5);
    CHECK_PAPER_SIZE(10.12, 14.33, PAPER_SIZE_JIS_B4);
    CHECK_PAPER_SIZE( 3.94,  5.83, PAPER_SIZE_JPOST);
    CHECK_PAPER_SIZE( 5.83,  7.87, PAPER_SIZE_JPOSTD);
    CHECK_PAPER_SIZE( 3.87,  7.5 , PAPER_SIZE_MONARCH);
    CHECK_PAPER_SIZE( 4.12,  9.5 , PAPER_SIZE_COM10);
    CHECK_PAPER_SIZE( 4.33,  8.66, PAPER_SIZE_DL);
    CHECK_PAPER_SIZE( 6.38,  9.01, PAPER_SIZE_C5);
    CHECK_PAPER_SIZE( 6.93,  9.84, PAPER_SIZE_B5);

#undef CHECK_PAPER_SIZE

    return code;
}

/* ------ Color mapping ------ */

/* The PaintJet and DeskJet 500C use additive colors in separate planes. */
/* We only keep one bit of color, with 1 = R, 2 = G, 4 = B. */
/* Because the buffering routines assume 0 = white, */
/* we complement all the color components. */
#define cv_shift (sizeof(gx_color_value) * 8 - 1)

/* Map an RGB color to a printer color. */
gx_color_index
gdev_pcl_3bit_map_rgb_color(gx_device * dev, const gx_color_value cv[])
{
    gx_color_value r, g, b;
    r = cv[0]; g = cv[1]; b = cv[2];
    return (((b >> cv_shift) << 2) + ((g >> cv_shift) << 1) + (r >> cv_shift)) ^ 7;
}

/* Map the printer color back to RGB. */
int
gdev_pcl_3bit_map_color_rgb(gx_device * dev, gx_color_index color,
			    gx_color_value prgb[3])
{
    ushort cc = (ushort) color ^ 7;

    prgb[0] = -(cc & 1);
    prgb[1] = -((cc >> 1) & 1);
    prgb[2] = -(cc >> 2);
    return 0;
}

/* ------ Compression ------ */

/*
 * Mode 2 Row compression routine for the HP DeskJet & LaserJet IIp.
 * Compresses data from row up to end_row, storing the result
 * starting at compressed.  Returns the number of bytes stored.
 * Runs of K<=127 literal bytes are encoded as K-1 followed by
 * the bytes; runs of 2<=K<=127 identical bytes are encoded as
 * 257-K followed by the byte.
 * In the worst case, the result is N+(N/127)+1 bytes long,
 * where N is the original byte count (end_row - row).
 * To speed up the search, we examine an entire word at a time.
 * We will miss a few blocks of identical bytes; tant pis.
 */
int
gdev_pcl_mode2compress_padded(const word * row, const word * end_row,
			      byte * compressed, bool pad)
{
    register const word *exam = row;	/* word being examined in the row to compress */
    register byte *cptr = compressed;	/* output pointer into compressed bytes */

    while (exam < end_row) {	/* Search ahead in the input looking for a run */
	/* of at least 4 identical bytes. */
	const byte *compr = (const byte *)exam;
	const byte *end_dis;
	const word *next;
	register word test = *exam;

	while (((test << 8) ^ test) > 0xff) {
	    if (++exam >= end_row)
		break;
	    test = *exam;
	}

	/* Find out how long the run is */
	end_dis = (const byte *)exam;
	if (exam == end_row) {	/* no run */
	    /* See if any of the last 3 "dissimilar" bytes are 0. */
	    if (!pad && end_dis > compr && end_dis[-1] == 0) {
		if (end_dis[-2] != 0)
		    end_dis--;
		else if (end_dis[-3] != 0)
		    end_dis -= 2;
		else
		    end_dis -= 3;
	    }
	    next = --end_row;
	} else {
	    next = exam + 1;
	    while (next < end_row && *next == test)
		next++;
	    /* See if any of the last 3 "dissimilar" bytes */
	    /* are the same as the repeated byte. */
	    if (end_dis > compr && end_dis[-1] == (byte) test) {
		if (end_dis[-2] != (byte) test)
		    end_dis--;
		else if (end_dis[-3] != (byte) test)
		    end_dis -= 2;
		else
		    end_dis -= 3;
	    }
	}

	/* Now [compr..end_dis) should be encoded as dissimilar, */
	/* and [end_dis..next) should be encoded as similar. */
	/* Note that either of these ranges may be empty. */

	for (;;) {		/* Encode up to 127 dissimilar bytes */
	    uint count = end_dis - compr;	/* uint for faster switch */

	    switch (count) {	/* Use memcpy only if it's worthwhile. */
		case 6:
		    cptr[6] = compr[5];
		case 5:
		    cptr[5] = compr[4];
		case 4:
		    cptr[4] = compr[3];
		case 3:
		    cptr[3] = compr[2];
		case 2:
		    cptr[2] = compr[1];
		case 1:
		    cptr[1] = compr[0];
		    *cptr = count - 1;
		    cptr += count + 1;
		case 0:	/* all done */
		    break;
		default:
		    if (count > 127)
			count = 127;
		    *cptr++ = count - 1;
		    memcpy(cptr, compr, count);
		    cptr += count, compr += count;
		    continue;
	    }
	    break;
	}

	{			/* Encode up to 127 similar bytes. */
	    /* Note that count may be <0 at end of row. */
	    int count = (const byte *)next - end_dis;

	    while (count > 0) {
		int this = (count > 127 ? 127 : count);

		*cptr++ = 257 - this;
		*cptr++ = (byte) test;
		count -= this;
	    }
	    exam = next;
	}
    }
    return (cptr - compressed);
}
int
gdev_pcl_mode2compress(const word * row, const word * end_row,
		       byte * compressed)
{
    return gdev_pcl_mode2compress_padded(row, end_row, compressed, false);
}

/*
 * Mode 3 compression routine for the HP LaserJet III family.
 * Compresses bytecount bytes starting at current, storing the result
 * in compressed, comparing against and updating previous.
 * Returns the number of bytes stored.  In the worst case,
 * the number of bytes is bytecount+(bytecount/8)+1.
 */
int
gdev_pcl_mode3compress(int bytecount, const byte * current, byte * previous, byte * compressed)
{
    register const byte *cur = current;
    register byte *prev = previous;
    register byte *out = compressed;
    const byte *end = current + bytecount;

    while (cur < end) {		/* Detect a maximum run of unchanged bytes. */
	const byte *run = cur;
	register const byte *diff;
	const byte *stop;
	int offset, cbyte;

	while (cur < end && *cur == *prev) {
	    cur++, prev++;
	}
	if (cur == end)
	    break;		/* rest of row is unchanged */
	/* Detect a run of up to 8 changed bytes. */
	/* We know that *cur != *prev. */
	diff = cur;
	stop = (end - cur > 8 ? cur + 8 : end);
	do {
	    *prev++ = *cur++;
	}
	while (cur < stop && *cur != *prev);
	/* Now [run..diff) are unchanged, and */
	/* [diff..cur) are changed. */
	/* Generate the command byte(s). */
	offset = diff - run;
	cbyte = (cur - diff - 1) << 5;
	if (offset < 31)
	    *out++ = cbyte + offset;
	else {
	    *out++ = cbyte + 31;
	    offset -= 31;
	    while (offset >= 255)
		*out++ = 255, offset -= 255;
	    *out++ = offset;
	}
	/* Copy the changed data. */
	while (diff < cur)
	    *out++ = *diff++;
    }
    return out - compressed;
}

/*
 * Mode 9 2D compression for the HP DeskJets . This mode can give
 * very good compression ratios, especially if there are areas of flat
 * colour (or blank areas), and so is 'highly recommended' for colour
 * printing in particular because of the very large amounts of data which
 * can be generated
 */
int
gdev_pcl_mode9compress(int bytecount, const byte *current,
		       const byte *previous, byte *compressed)
{
    register const byte *cur = current;
    register const byte *prev = previous;
    register byte *out = compressed;
    const byte *end = current + bytecount;

    while (cur < end) {		/* Detect a run of unchanged bytes. */
	const byte *run = cur;
	register const byte *diff;
	int offset;

	while (cur < end && *cur == *prev) {
	    cur++, prev++;
	}
	if (cur == end)
	    break;		/* rest of row is unchanged */
	/* Detect a run of changed bytes. */
	/* We know that *cur != *prev. */
	diff = cur;
	do {
	    prev++;
	    cur++;
	}
	while (cur < end && *cur != *prev);
	/* Now [run..diff) are unchanged, and */
	/* [diff..cur) are changed. */
	offset = diff - run;
	{
	    const byte *stop_test = cur - 4;
	    int dissimilar, similar;

	    while (diff < cur) {
		const byte *compr = diff;
		const byte *next;	/* end of run */
		byte value = 0;

		while (diff <= stop_test &&
		       ((value = *diff) != diff[1] ||
			value != diff[2] ||
			value != diff[3]))
		    diff++;

		/* Find out how long the run is */
		if (diff > stop_test)	/* no run */
		    next = diff = cur;
		else {
		    next = diff + 4;
		    while (next < cur && *next == value)
			next++;
		}

#define MAXOFFSETU 15
#define MAXCOUNTU 7
		/* output 'dissimilar' bytes, uncompressed */
		if ((dissimilar = diff - compr)) {
		    int temp, i;

		    if ((temp = --dissimilar) > MAXCOUNTU)
			temp = MAXCOUNTU;
		    if (offset < MAXOFFSETU)
			*out++ = (offset << 3) | (byte) temp;
		    else {
			*out++ = (MAXOFFSETU << 3) | (byte) temp;
			offset -= MAXOFFSETU;
			while (offset >= 255) {
			    *out++ = 255;
			    offset -= 255;
			}
			*out++ = offset;
		    }
		    if (temp == MAXCOUNTU) {
			temp = dissimilar - MAXCOUNTU;
			while (temp >= 255) {
			    *out++ = 255;
			    temp -= 255;
			}
			*out++ = (byte) temp;
		    }
		    for (i = 0; i <= dissimilar; i++)
			*out++ = *compr++;
		    offset = 0;
		}		/* end uncompressed */
#undef MAXOFFSETU
#undef MAXCOUNTU

#define MAXOFFSETC 3
#define MAXCOUNTC 31
		/* output 'similar' bytes, run-length encoded */
		if ((similar = next - diff)) {
		    int temp;

		    if ((temp = (similar -= 2)) > MAXCOUNTC)
			temp = MAXCOUNTC;
		    if (offset < MAXOFFSETC)
			*out++ = 0x80 | (offset << 5) | (byte) temp;
		    else {
			*out++ = 0x80 | (MAXOFFSETC << 5) | (byte) temp;
			offset -= MAXOFFSETC;
			while (offset >= 255) {
			    *out++ = 255;
			    offset -= 255;
			}
			*out++ = offset;
		    }
		    if (temp == MAXCOUNTC) {
			temp = similar - MAXCOUNTC;
			while (temp >= 255) {
			    *out++ = 255;
			    temp -= 255;
			}
			*out++ = (byte) temp;
		    }
		    *out++ = value;
		    offset = 0;
		}		/* end compressed */
#undef MAXOFFSETC
#undef MAXCOUNTC

		diff = next;
	    }
	}
    }
    return out - compressed;
}
