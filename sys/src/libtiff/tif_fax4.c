#ifndef lint
static char rcsid[] = "$Header: /usr/people/sam/tiff/libtiff/RCS/tif_fax4.c,v 1.14 92/03/11 15:45:17 sam Exp $";
#endif

/*
 * Copyright (c) 1990, 1991, 1992 Sam Leffler
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
 * CCITT Group 4 Facsimile-compatible
 * Compression Scheme Support.
 */
#include "tiffioP.h"
#include "tif_fax3.h"

#if USE_PROTOTYPES
static	int Fax4Decode(TIFF*, u_char *, int, u_int);
static	int Fax4Encode(TIFF*, u_char *, int, u_int);
static	int Fax4PostEncode(TIFF*);
#else
static	int Fax4Decode();
static	int Fax4Encode();
static	int Fax4PostEncode();
#endif

TIFFInitCCITTFax4(tif)
	TIFF *tif;
{
	TIFFInitCCITTFax3(tif);		/* reuse G3 compression */
	tif->tif_decoderow = Fax4Decode;
	tif->tif_decodestrip = Fax4Decode;
	tif->tif_decodetile = Fax4Decode;
	tif->tif_encoderow = Fax4Encode;
	tif->tif_encodestrip = Fax4Encode;
	tif->tif_encodetile = Fax4Encode;
	tif->tif_postencode = Fax4PostEncode;
	/*
	 * FAX3_NOEOL causes the regular G3 decompression
	 * code to not skip to the EOL mark at the end of
	 * a row (during normal decoding).  FAX3_CLASSF
	 * suppresses RTC generation at the end of an image.
	 */
	tif->tif_options = FAX3_NOEOL|FAX3_CLASSF;
	return (1);
}

/*
 * Decode the requested amount of data.
 */
static int
Fax4Decode(tif, buf, occ, s)
	TIFF *tif;
	u_char *buf;
	int occ;
	u_int s;
{
	Fax3BaseState *sp = (Fax3BaseState *)tif->tif_data;

	bzero(buf, occ);		/* decoding only sets non-zero bits */
	while (occ > 0) {
		if (!Fax3Decode2DRow(tif, buf, sp->rowpixels))
			return (0);
		bcopy(buf, sp->refline, sp->rowbytes);
		buf += sp->rowbytes;
		occ -= sp->rowbytes;
	}
	return (1);
}

/*
 * Encode the requested amount of data.
 */
static int
Fax4Encode(tif, bp, cc, s)
	TIFF *tif;
	u_char *bp;
	int cc;
	u_int s;
{
	Fax3BaseState *sp = (Fax3BaseState *)tif->tif_data;

	while (cc > 0) {
		if (!Fax3Encode2DRow(tif, bp, sp->refline, sp->rowpixels))
			return (0);
		bcopy(bp, sp->refline, sp->rowbytes);
		bp += sp->rowbytes;
		cc -= sp->rowbytes;
	}
	return (1);
}

static
Fax4PostEncode(tif)
	TIFF *tif;
{
	Fax3BaseState *sp = (Fax3BaseState *)tif->tif_data;

	/* terminate strip w/ EOFB */
	Fax3PutEOL(tif);
	Fax3PutEOL(tif);
	if (sp->bit != 8)
		Fax3FlushBits(tif, sp);
	return (1);
}
