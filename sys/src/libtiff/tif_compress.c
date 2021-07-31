#ifndef lint
static char rcsid[] = "$Header: /usr/people/sam/tiff/libtiff/RCS/tif_compress.c,v 1.26 92/02/10 19:06:13 sam Exp $";
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
 * TIFF Library
 *
 * Compression Scheme Configuration Support.
 */
#include "tiffioP.h"

#if USE_PROTOTYPES
extern	int TIFFInitDumpMode(TIFF*);
#ifdef PACKBITS_SUPPORT
extern	int TIFFInitPackBits(TIFF*);
#endif
#ifdef CCITT_SUPPORT
extern	int TIFFInitCCITTRLE(TIFF*), TIFFInitCCITTRLEW(TIFF*);
extern	int TIFFInitCCITTFax3(TIFF*), TIFFInitCCITTFax4(TIFF*);
#endif
#ifdef THUNDER_SUPPORT
extern	int TIFFInitThunderScan(TIFF*);
#endif
#ifdef NEXT_SUPPORT
extern	int TIFFInitNeXT(TIFF*);
#endif
#ifdef LZW_SUPPORT
extern	int TIFFInitLZW(TIFF*);
#endif
#ifdef JPEG_SUPPORT
extern	int TIFFInitJPEG(TIFF*);
#endif
#else
extern	int TIFFInitDumpMode();
#ifdef PACKBITS_SUPPORT
extern	int TIFFInitPackBits();
#endif
#ifdef CCITT_SUPPORT
extern	int TIFFInitCCITTRLE(), TIFFInitCCITTRLEW();
extern	int TIFFInitCCITTFax3(), TIFFInitCCITTFax4();
#endif
#ifdef THUNDER_SUPPORT
extern	int TIFFInitThunderScan();
#endif
#ifdef NEXT_SUPPORT
extern	int TIFFInitNeXT();
#endif
#ifdef LZW_SUPPORT
extern	int TIFFInitLZW();
#endif
#ifdef JPEG_SUPPORT
extern	int TIFFInitJPEG();
#endif
#endif

struct cscheme {
	char*	name;
	int	scheme;
	int	(*init)();
};
static const struct cscheme CompressionSchemes[] = {
    { "Null",		COMPRESSION_NONE,	TIFFInitDumpMode },
#ifdef LZW_SUPPORT
    { "LZW",		COMPRESSION_LZW,	TIFFInitLZW },
#endif
#ifdef PACKBITS_SUPPORT
    { "PackBits",	COMPRESSION_PACKBITS,	TIFFInitPackBits },
#endif
#ifdef THUNDER_SUPPORT
    { "ThunderScan",	COMPRESSION_THUNDERSCAN,TIFFInitThunderScan },
#endif
#ifdef NEXT_SUPPORT
    { "NeXT",		COMPRESSION_NEXT,	TIFFInitNeXT },
#endif
#ifdef JPEG_SUPPORT
    { "JPEG",		COMPRESSION_JPEG,	TIFFInitJPEG },
#endif
#ifdef CCITT_SUPPORT
    { "CCITT RLE",	COMPRESSION_CCITTRLE,	TIFFInitCCITTRLE },
    { "CCITT RLE/W",	COMPRESSION_CCITTRLEW,	TIFFInitCCITTRLEW },
    { "CCITT Group3",	COMPRESSION_CCITTFAX3,	TIFFInitCCITTFax3 },
    { "CCITT Group4",	COMPRESSION_CCITTFAX4,	TIFFInitCCITTFax4 },
#endif
};
#define	NSCHEMES (sizeof (CompressionSchemes) / sizeof (CompressionSchemes[0]))

static struct cscheme const *
findScheme(scheme)
	int scheme;
{
	register struct cscheme const *c;

	for (c = CompressionSchemes; c < &CompressionSchemes[NSCHEMES]; c++)
		if (c->scheme == scheme)
			return (c);
	return ((struct cscheme const *)0);
}

static int
TIFFNoEncode(tif, method)
	TIFF *tif;
	char *method;
{
	struct cscheme const *c = findScheme(tif->tif_dir.td_compression);
	TIFFError(tif->tif_name,
	    "%s %s encoding is not implemented", c->name, method);
	return (-1);
}

int
TIFFNoRowEncode(tif, pp, cc, s)
	TIFF *tif;
	u_char *pp;
	int cc;
	u_int s;
{
	return (TIFFNoEncode(tif, "scanline"));
}

int
TIFFNoStripEncode(tif, pp, cc, s)
	TIFF *tif;
	u_char *pp;
	int cc;
	u_int s;
{
	return (TIFFNoEncode(tif, "strip"));
}

int
TIFFNoTileEncode(tif, pp, cc, s)
	TIFF *tif;
	u_char *pp;
	int cc;
	u_int s;
{
	return (TIFFNoEncode(tif, "tile"));
}

int
TIFFNoDecode(tif, method)
	TIFF *tif;
	char *method;
{
	struct cscheme const *c = findScheme(tif->tif_dir.td_compression);
	TIFFError(tif->tif_name,
	    "%s %s decoding is not implemented", c->name, method);
	return (-1);
}

int
TIFFNoRowDecode(tif, pp, cc, s)
	TIFF *tif;
	u_char *pp;
	int cc;
	u_int s;
{
	return (TIFFNoDecode(tif, "scanline"));
}

int
TIFFNoStripDecode(tif, pp, cc, s)
	TIFF *tif;
	u_char *pp;
	int cc;
	u_int s;
{
	return (TIFFNoDecode(tif, "strip"));
}

int
TIFFNoTileDecode(tif, pp, cc, s)
	TIFF *tif;
	u_char *pp;
	int cc;
	u_int s;
{
	return (TIFFNoDecode(tif, "tile"));
}

TIFFSetCompressionScheme(tif, scheme)
	TIFF *tif;
	int scheme;
{
	struct cscheme const *c = findScheme(scheme);

	if (!c) {
		TIFFError(tif->tif_name,
		    "Unknown data compression algorithm %u (0x%x)",
		    scheme, scheme);
		return (0);
	}
	tif->tif_predecode = NULL;
	tif->tif_decoderow = TIFFNoRowDecode;
	tif->tif_decodestrip = TIFFNoStripDecode;
	tif->tif_decodetile = TIFFNoTileDecode;
	tif->tif_preencode = NULL;
	tif->tif_postencode = NULL;
	tif->tif_encoderow = TIFFNoRowEncode;
	tif->tif_encodestrip = TIFFNoStripEncode;
	tif->tif_encodetile = TIFFNoTileEncode;
	tif->tif_close = NULL;
	tif->tif_seek = NULL;
	tif->tif_cleanup = NULL;
	tif->tif_flags &= ~TIFF_NOBITREV;
	tif->tif_options = 0;
	return ((*c->init)(tif));
}
