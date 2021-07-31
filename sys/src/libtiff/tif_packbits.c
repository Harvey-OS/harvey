#ifndef lint
static char rcsid[] = "$Header: /usr/people/sam/tiff/libtiff/RCS/tif_packbits.c,v 1.23 92/03/30 18:29:40 sam Exp $";
#endif

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992 Sam Leffler
 * Copyright (c) 1991, 1992 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library.
 *
 * PackBits Compression Algorithm Support
 */
#include "tiffioP.h"
#include <stdio.h>
#include <assert.h>

#if USE_PROTOTYPES
static	int PackBitsPreEncode(TIFF *);
static	int PackBitsEncode(TIFF *, u_char *, int, u_int);
static	int PackBitsEncodeChunk(TIFF *, u_char *, int, u_int);
static	int PackBitsDecode(TIFF *, u_char *, int, u_int);
#else
static	int PackBitsPreEncode();
static	int PackBitsEncode(), PackBitsEncodeChunk();
static	int PackBitsDecode();
#endif

TIFFInitPackBits(tif)
	TIFF *tif;
{
	tif->tif_decoderow = PackBitsDecode;
	tif->tif_decodestrip = PackBitsDecode;
	tif->tif_decodetile = PackBitsDecode;
	tif->tif_preencode = PackBitsPreEncode;
	tif->tif_encoderow = PackBitsEncode;
	tif->tif_encodestrip = PackBitsEncodeChunk;
	tif->tif_encodetile = PackBitsEncodeChunk;
	return (1);
}

static int
PackBitsPreEncode(tif)
	TIFF *tif;
{
	/*
	 * Calculate the scanline/tile-width size in bytes.
	 */
	if (isTiled(tif))
		tif->tif_data = (char *) TIFFTileRowSize(tif);
	else
		tif->tif_data = (char *) TIFFScanlineSize(tif);
	return (1);
}

/*
 * Encode a rectangular chunk of pixels.  We break it up
 * into row-sized pieces to insure that encoded runs do
 * not span rows.  Otherwise, there can be problems with
 * the decoder if data is read, for example, by scanlines
 * when it was encoded by strips.
 */
static int
PackBitsEncodeChunk(tif, bp, cc, s)
	TIFF *tif;
	u_char *bp;
	int cc;
	u_int s;
{
	int rowsize = (int) tif->tif_data;

	assert(rowsize > 0);
	while (cc > 0) {
		if (PackBitsEncode(tif, bp, rowsize, s) < 0)
			return (-1);
		bp += rowsize;
		cc -= rowsize;
	}
	return (1);
}

/*
 * Encode a run of pixels.
 */
static int
PackBitsEncode(tif, bp, cc, s)
	TIFF *tif;
	u_char *bp;
	register int cc;
	u_int s;
{
	register char *op, *lastliteral;
	register int n, b;
	enum { BASE, LITERAL, RUN, LITERAL_RUN } state;
	char *ep;
	int slop;

	op = tif->tif_rawcp;
	ep = tif->tif_rawdata + tif->tif_rawdatasize;
	state = BASE;
	lastliteral = 0;
	while (cc > 0) {
		/*
		 * Find the longest string of identical bytes.
		 */
		b = *bp++, cc--, n = 1;
		for (; cc > 0 && b == *bp; cc--, bp++)
			n++;
	again:
		if (op + 2 >= ep) {		/* insure space for new data */
			/*
			 * Be careful about writing the last
			 * literal.  Must write up to that point
			 * and then copy the remainder to the
			 * front of the buffer.
			 */
			if (state == LITERAL || state == LITERAL_RUN) {
				slop = op - lastliteral;
				tif->tif_rawcc += lastliteral - tif->tif_rawcp;
				if (!TIFFFlushData1(tif))
					return (-1);
				op = tif->tif_rawcp;
				for (; slop-- > 0; *op++ = *lastliteral++)
					;
				lastliteral = tif->tif_rawcp;
			} else {
				tif->tif_rawcc += op - tif->tif_rawcp;
				if (!TIFFFlushData1(tif))
					return (-1);
				op = tif->tif_rawcp;
			}
		}
		switch (state) {
		case BASE:		/* initial state, set run/literal */
			if (n > 1) {
				state = RUN;
				if (n > 128) {
					*op++ = -127;
					*op++ = b;
					n -= 128;
					goto again;
				}
				*op++ = -(n-1);
				*op++ = b;
			} else {
				lastliteral = op;
				*op++ = 0;
				*op++ = b;
				state = LITERAL;
			}
			break;
		case LITERAL:		/* last object was literal string */
			if (n > 1) {
				state = LITERAL_RUN;
				if (n > 128) {
					*op++ = -127;
					*op++ = b;
					n -= 128;
					goto again;
				}
				*op++ = -(n-1);		/* encode run */
				*op++ = b;
			} else {			/* extend literal */
				if (++(*lastliteral) == 127)
					state = BASE;
				*op++ = b;
			}
			break;
		case RUN:		/* last object was run */
			if (n > 1) {
				if (n > 128) {
					*op++ = -127;
					*op++ = b;
					n -= 128;
					goto again;
				}
				*op++ = -(n-1);
				*op++ = b;
			} else {
				lastliteral = op;
				*op++ = 0;
				*op++ = b;
				state = LITERAL;
			}
			break;
		case LITERAL_RUN:	/* literal followed by a run */
			/*
			 * Check to see if previous run should
			 * be converted to a literal, in which
			 * case we convert literal-run-literal
			 * to a single literal.
			 */
			if (n == 1 && op[-2] == (char)-1 &&
			    *lastliteral < 126) {
				state = (((*lastliteral) += 2) == 127 ?
				    BASE : LITERAL);
				op[-2] = op[-1];	/* replicate */
			} else
				state = RUN;
			goto again;
		}
	}
	tif->tif_rawcc += op - tif->tif_rawcp;
	tif->tif_rawcp = op;
	return (1);
}

static int
PackBitsDecode(tif, op, occ, s)
	TIFF *tif;
	register u_char *op;
	register int occ;
	u_int s;
{
	register char *bp;
	register int n, b;
	register int cc;

	bp = tif->tif_rawcp; cc = tif->tif_rawcc;
	while (cc > 0 && occ > 0) {
		n = (int) *bp++;
		/*
		 * Watch out for compilers that
		 * don't sign extend chars...
		 */
		if (n >= 128)
			n -= 256;
		if (n < 0) {		/* replicate next byte -n+1 times */
			cc--;
			if (n == -128)	/* nop */
				continue;
			n = -n + 1;
			occ -= n;
			for (b = *bp++; n-- > 0;)
				*op++ = b;
		} else {		/* copy next n+1 bytes literally */
			bcopy(bp, op, ++n);
			op += n; occ -= n;
			bp += n; cc -= n;
		}
	}
	tif->tif_rawcp = bp;
	tif->tif_rawcc = cc;
	if (occ > 0) {
		TIFFError(tif->tif_name,
		    "PackBitsDecode: Not enough data for scanline %d",
		    tif->tif_row);
		return (0);
	}
	/* check for buffer overruns? */
	return (1);
}
