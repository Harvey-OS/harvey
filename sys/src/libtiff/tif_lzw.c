#ifndef lint
static char rcsid[] = "$Header: /usr/people/sam/tiff/libtiff/RCS/tif_lzw.c,v 1.37 92/02/12 11:27:28 sam Exp $";
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
 * Rev 5.0 Lempel-Ziv & Welch Compression Support
 *
 * This code is derived from the compress program whose code is
 * derived from software contributed to Berkeley by James A. Woods,
 * derived from original work by Spencer Thomas and Joseph Orost.
 *
 * The original Berkeley copyright notice appears below in its entirety.
 */
#include "tiffioP.h"
#include <stdio.h>
#include <assert.h>
#include "prototypes.h"

#define MAXCODE(n)	((1 << (n)) - 1)
/*
 * The TIFF spec specifies that encoded bit strings range
 * from 9 to 12 bits.  This is somewhat unfortunate in that
 * experience indicates full color RGB pictures often need
 * ~14 bits for reasonable compression.
 */
#define	BITS_MIN	9		/* start with 9 bits */
#define	BITS_MAX	12		/* max of 12 bit strings */
/* predefined codes */
#define	CODE_CLEAR	256		/* code to clear string table */
#define	CODE_EOI	257		/* end-of-information code */
#define CODE_FIRST	258		/* first free code entry */
#define	CODE_MAX	MAXCODE(BITS_MAX)
#ifdef notdef
#define	HSIZE		9001		/* 91% occupancy */
#define	HSHIFT		(8-(16-13))
#else
#define	HSIZE		5003		/* 80% occupancy */
#define	HSHIFT		(8-(16-12))
#endif

/*
 * NB: The 5.0 spec describes a different algorithm than Aldus
 *     implements.  Specifically, Aldus does code length transitions
 *     one code earlier than should be done (for real LZW).
 *     Earlier versions of this library implemented the correct
 *     LZW algorithm, but emitted codes in a bit order opposite
 *     to the TIFF spec.  Thus, to maintain compatibility w/ Aldus
 *     we interpret MSB-LSB ordered codes to be images written w/
 *     old versions of this library, but otherwise adhere to the
 *     Aldus "off by one" algorithm.
 *
 * Future revisions to the TIFF spec are expected to "clarify this issue".
 */
#define	SetMaxCode(sp, v) {			\
	(sp)->lzw_maxcode = (v)-1;		\
	if ((sp)->lzw_flags & LZW_COMPAT)	\
		(sp)->lzw_maxcode++;		\
}

/*
 * Decoding-specific state.
 */
struct decode {
	short	prefixtab[HSIZE];	/* prefix(code) */
	u_char	suffixtab[CODE_MAX+1];	/* suffix(code) */
	u_char	stack[HSIZE-(CODE_MAX+1)];
	u_char	*stackp;		/* stack pointer */
	int	firstchar;		/* of string associated w/ last code */
};

/*
 * Encoding-specific state.
 */
struct encode {
	long	checkpoint;		/* point at which to clear table */
#define CHECK_GAP	10000		/* enc_ratio check interval */
	long	ratio;			/* current compression ratio */
	long	incount;		/* (input) data bytes encoded */
	long	outcount;		/* encoded (output) bytes */
	long	htab[HSIZE];		/* hash table */
	short	codetab[HSIZE];		/* code table */
};

#if USE_PROTOTYPES
typedef	void (*predictorFunc)(char* data, int nbytes, int stride);
#else
typedef	void (*predictorFunc)();
#endif

/*
 * State block for each open TIFF
 * file using LZW compression/decompression.
 */
typedef	struct {
	int	lzw_oldcode;		/* last code encountered */
	u_short	lzw_flags;
#define	LZW_RESTART	0x01		/* restart interrupted decode */
#define	LZW_COMPAT	0x02		/* read old bit-reversed codes */
	u_short	lzw_nbits;		/* number of bits/code */
	u_short	lzw_stride;		/* horizontal diferencing stride */
	u_short	lzw_rowsize;		/* XXX maybe should be a long? */
	predictorFunc lzw_hordiff;
	int	lzw_maxcode;		/* maximum code for lzw_nbits */
	long	lzw_bitoff;		/* bit offset into data */
	long	lzw_bitsize;		/* size of strip in bits */
	int	lzw_free_ent;		/* next free entry in hash table */
	union {
		struct	decode dec;
		struct	encode enc;
	} u;
} LZWState;
#define	dec_prefix	u.dec.prefixtab
#define	dec_suffix	u.dec.suffixtab
#define	dec_stack	u.dec.stack
#define	dec_stackp	u.dec.stackp
#define	dec_firstchar	u.dec.firstchar

#define	enc_checkpoint	u.enc.checkpoint
#define	enc_ratio	u.enc.ratio
#define	enc_incount	u.enc.incount
#define	enc_outcount	u.enc.outcount
#define	enc_htab	u.enc.htab
#define	enc_codetab	u.enc.codetab

/* masks for extracting/inserting variable length bit codes */
static const u_char rmask[9] =
    { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
static const u_char lmask[9] =
    { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };

#if USE_PROTOTYPES
static	int LZWPreEncode(TIFF*);
static	int LZWEncode(TIFF*, u_char*, int, u_int);
static	int LZWEncodePredRow(TIFF*, u_char*, int, u_int);
static	int LZWEncodePredTile(TIFF*, u_char*, int, u_int);
static	int LZWPostEncode(TIFF*);
static	int LZWDecode(TIFF*, u_char*, int, u_int);
static	int LZWDecodePredRow(TIFF*, u_char*, int, u_int);
static	int LZWDecodePredTile(TIFF*, u_char*, int, u_int);
static	int LZWPreDecode(TIFF*);
static	int LZWCleanup(TIFF*);
static	int GetNextCode(TIFF*);
static	void PutNextCode(TIFF*, int);
static	void cl_block(TIFF*);
static	void cl_hash(LZWState*);
extern	int TIFFFlushData1(TIFF *);
#else
static	int LZWPreEncode(), LZWEncode(), LZWPostEncode();
static	int LZWEncodePredRow(), LZWEncodePredTile();
static	int LZWPreDecode(), LZWDecode();
static	int LZWDecodePredRow(), LZWDecodePredTile();
static	int LZWCleanup();
static	int GetNextCode();
static	void PutNextCode();
static	void cl_block();
static	void cl_hash();
extern	int TIFFFlushData1();
#endif

TIFFInitLZW(tif)
	TIFF *tif;
{
	tif->tif_predecode = LZWPreDecode;
	tif->tif_decoderow = LZWDecode;
	tif->tif_decodestrip = LZWDecode;
	tif->tif_decodetile = LZWDecode;
	tif->tif_preencode = LZWPreEncode;
	tif->tif_postencode = LZWPostEncode;
	tif->tif_encoderow = LZWEncode;
	tif->tif_encodestrip = LZWEncode;
	tif->tif_encodetile = LZWEncode;
	tif->tif_cleanup = LZWCleanup;
	return (1);
}

static
DECLARE4(LZWCheckPredictor,
	TIFF*, tif,
	LZWState*, sp,
	predictorFunc, pred8bit,
	predictorFunc, pred16bit
)
{
	TIFFDirectory *td = &tif->tif_dir;

	switch (td->td_predictor) {
	case 1:
		break;
	case 2:
		sp->lzw_stride = (td->td_planarconfig == PLANARCONFIG_CONTIG ?
		    td->td_samplesperpixel : 1);
		switch (td->td_bitspersample) {
		case 8:
			sp->lzw_hordiff = pred8bit;
			break;
		case 16:
			sp->lzw_hordiff = pred16bit;
			break;
		default:
			TIFFError(tif->tif_name,
    "Horizontal differencing \"Predictor\" not supported with %d-bit samples",
			    td->td_bitspersample);
			return (0);
		}
		break;
	default:
		TIFFError(tif->tif_name, "\"Predictor\" value %d not supported",
		    td->td_predictor);
		return (0);
	}
	if (sp->lzw_hordiff) {
		/*
		 * Calculate the scanline/tile-width size in bytes.
		 */
		if (isTiled(tif))
			sp->lzw_rowsize = TIFFTileRowSize(tif);
		else
			sp->lzw_rowsize = TIFFScanlineSize(tif);
	}
	return (1);
}

/*
 * LZW Decoder.
 */

#define REPEAT4(n, op)		\
    switch (n) {		\
    default: { int i; for (i = n-4; i > 0; i--) { op; } } \
    case 4:  op;		\
    case 3:  op;		\
    case 2:  op;		\
    case 1:  op;		\
    case 0:  ;			\
    }

static void
DECLARE3(horizontalAccumulate8,
	register char*, cp,
	register int, cc,
	register int, stride
)
{
	if (cc > stride) {
		cc -= stride;
		do {
			REPEAT4(stride, cp[stride] += cp[0]; cp++)
			cc -= stride;
		} while (cc > 0);
	}
}

static void
DECLARE3(horizontalAccumulate16,
	char*, cp,
	int, cc,
	register int, stride
)
{
	register short* wp = (short *)cp;
	register int wc = cc / 2;

	if (wc > stride) {
		wc -= stride;
		do {
			REPEAT4(stride, wp[stride] += wp[0]; wp++)
			wc -= stride;
		} while (wc > 0);
	}
}

/*
 * Setup state for decoding a strip.
 */
static
LZWPreDecode(tif)
	TIFF *tif;
{
	register LZWState *sp = (LZWState *)tif->tif_data;
	register int code;

	if (sp == NULL) {
		tif->tif_data = malloc(sizeof (LZWState));
		if (tif->tif_data == NULL) {
			TIFFError("LZWPreDecode",
			    "No space for LZW state block");
			return (0);
		}
		sp = (LZWState *)tif->tif_data;
		sp->lzw_flags = 0;
		sp->lzw_hordiff = 0;
		sp->lzw_rowsize = 0;
		if (!LZWCheckPredictor(tif, sp,
		  horizontalAccumulate8, horizontalAccumulate16))
			return (0);
		if (sp->lzw_hordiff) {
			/*
			 * Override default decoding method with
			 * one that does the predictor stuff.
			 */
			tif->tif_decoderow = LZWDecodePredRow;
			tif->tif_decodestrip = LZWDecodePredTile;
			tif->tif_decodetile = LZWDecodePredTile;
		}
	} else
		sp->lzw_flags &= ~LZW_RESTART;
	sp->lzw_nbits = BITS_MIN;
	/*
	 * Pre-load the table.
	 */
	for (code = 255; code >= 0; code--)
		sp->dec_suffix[code] = (u_char)code;
	sp->lzw_free_ent = CODE_FIRST;
	sp->lzw_bitoff = 0;
	/* calculate data size in bits */
	sp->lzw_bitsize = tif->tif_rawdatasize;
	sp->lzw_bitsize = (sp->lzw_bitsize << 3) - (BITS_MAX-1);
	sp->dec_stackp = sp->dec_stack;
	sp->lzw_oldcode = -1;
	sp->dec_firstchar = -1;
	/*
	 * Check for old bit-reversed codes.  All the flag
	 * manipulations are to insure only one warning is
	 * given for a file.
	 */
	if (tif->tif_rawdata[0] == 0 && (tif->tif_rawdata[1] & 0x1)) {
		if ((sp->lzw_flags & LZW_COMPAT) == 0)
			TIFFWarning(tif->tif_name,
			    "Old-style LZW codes, convert file");
		sp->lzw_flags |= LZW_COMPAT;
	} else
		sp->lzw_flags &= ~LZW_COMPAT;
	SetMaxCode(sp, MAXCODE(BITS_MIN));
	return (1);
}

/*
 * Decode a "hunk of data".
 */
static
LZWDecode(tif, op0, occ0, s)
	TIFF *tif;
	u_char *op0;
	u_int s;
{
	register char *op = (char *)op0;
	register int occ = occ0;
	register LZWState *sp = (LZWState *)tif->tif_data;
	register int code;
	register u_char *stackp;
	int firstchar, oldcode, incode;

	stackp = sp->dec_stackp;
	/*
	 * Restart interrupted unstacking operations.
	 */
	if (sp->lzw_flags & LZW_RESTART) {
		do {
			if (--occ < 0) {	/* end of scanline */
				sp->dec_stackp = stackp;
				return (1);
			}
			*op++ = *--stackp;
		} while (stackp > sp->dec_stack);
		sp->lzw_flags &= ~LZW_RESTART;
	}
	oldcode = sp->lzw_oldcode;
	firstchar = sp->dec_firstchar;
	while (occ > 0 && (code = GetNextCode(tif)) != CODE_EOI) {
		if (code == CODE_CLEAR) {
			bzero(sp->dec_prefix, sizeof (sp->dec_prefix));
			sp->lzw_free_ent = CODE_FIRST;
			sp->lzw_nbits = BITS_MIN;
			SetMaxCode(sp, MAXCODE(BITS_MIN));
			if ((code = GetNextCode(tif)) == CODE_EOI)
				break;
			*op++ = code, occ--;
			oldcode = firstchar = code;
			continue;
		}
		incode = code;
		/*
		 * When a code is not in the table we use (as spec'd):
		 *    StringFromCode(oldcode) +
		 *        FirstChar(StringFromCode(oldcode))
		 */
		if (code >= sp->lzw_free_ent) {	/* code not in table */
			*stackp++ = firstchar;
			code = oldcode;
		}

		/*
		 * Generate output string (first in reverse).
		 */
		for (; code >= 256; code = sp->dec_prefix[code])
			*stackp++ = sp->dec_suffix[code];
		*stackp++ = firstchar = sp->dec_suffix[code];
		do {
			if (--occ < 0) {	/* end of scanline */
				sp->lzw_flags |= LZW_RESTART;
				break;
			}
			*op++ = *--stackp;
		} while (stackp > sp->dec_stack);

		/*
		 * Add the new entry to the code table.
		 */
		if ((code = sp->lzw_free_ent) < CODE_MAX) {
			sp->dec_prefix[code] = (u_short)oldcode;
			sp->dec_suffix[code] = firstchar;
			sp->lzw_free_ent++;
			/*
			 * If the next entry is too big for the
			 * current code size, then increase the
			 * size up to the maximum possible.
			 */
			if (sp->lzw_free_ent > sp->lzw_maxcode) {
				sp->lzw_nbits++;
				if (sp->lzw_nbits > BITS_MAX)
					sp->lzw_nbits = BITS_MAX;
				SetMaxCode(sp, MAXCODE(sp->lzw_nbits));
			}
		} 
		oldcode = incode;
	}
	sp->dec_stackp = stackp;
	sp->lzw_oldcode = oldcode;
	sp->dec_firstchar = firstchar;
	if (occ > 0) {
		TIFFError(tif->tif_name,
		"LZWDecode: Not enough data at scanline %d (short %d bytes)",
		    tif->tif_row, occ);
		return (0);
	}
	return (1);
}

/*
 * Decode a scanline and apply the predictor routine.
 */
static
LZWDecodePredRow(tif, op0, occ0, s)
	TIFF *tif;
	u_char *op0;
	u_int s;
{
	LZWState *sp = (LZWState *)tif->tif_data;

	if (LZWDecode(tif, op0, occ0, s)) {
		(*sp->lzw_hordiff)((char *)op0, occ0, sp->lzw_stride);
		return (1);
	} else
		return (0);
}

/*
 * Decode a tile/strip and apply the predictor routine.
 * Note that horizontal differencing must be done on a
 * row-by-row basis.  The width of a "row" has already
 * been calculated at pre-decode time according to the
 * strip/tile dimensions.
 */
static
LZWDecodePredTile(tif, op0, occ0, s)
	TIFF *tif;
	u_char *op0;
	u_int s;
{
	LZWState *sp = (LZWState *)tif->tif_data;

	if (!LZWDecode(tif, op0, occ0, s))
		return (0);
	while (occ0 > 0) {
		(*sp->lzw_hordiff)((char *)op0, sp->lzw_rowsize, sp->lzw_stride);
		occ0 -= sp->lzw_rowsize;
		op0 += sp->lzw_rowsize;
	}
	return (1);
}

/*
 * Get the next code from the raw data buffer.
 */
static
GetNextCode(tif)
	TIFF *tif;
{
	register LZWState *sp = (LZWState *)tif->tif_data;
	register int code, bits;
	register long r_off;
	register u_char *bp;

	/*
	 * This check shouldn't be necessary because each
	 * strip is suppose to be terminated with CODE_EOI.
	 * At worst it's a substitute for the CODE_EOI that's
	 * supposed to be there (see calculation of lzw_bitsize
	 * in LZWPreDecode()).
	 */
	if (sp->lzw_bitoff > sp->lzw_bitsize) {
		TIFFWarning(tif->tif_name,
		    "LZWDecode: Strip %d not terminated with EOI code",
		    tif->tif_curstrip);
		return (CODE_EOI);
	}
	r_off = sp->lzw_bitoff;
	bits = sp->lzw_nbits;
	/*
	 * Get to the first byte.
	 */
	bp = (u_char *)tif->tif_rawdata + (r_off >> 3);
	r_off &= 7;
	if (sp->lzw_flags & LZW_COMPAT) {
		/* Get first part (low order bits) */
		code = (*bp++ >> r_off);
		r_off = 8 - r_off;		/* now, offset into code word */
		bits -= r_off;
		/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
		if (bits >= 8) {
			code |= *bp++ << r_off;
			r_off += 8;
			bits -= 8;
		}
		/* high order bits. */
		code |= (*bp & rmask[bits]) << r_off;
	} else {
		r_off = 8 - r_off;		/* convert offset to count */
		code = *bp++ & rmask[r_off];	/* high order bits */
		bits -= r_off;
		if (bits >= 8) {
			code = (code<<8) | *bp++;
			bits -= 8;
		}
		/* low order bits */
		code = (code << bits) |
		    (((unsigned)(*bp & lmask[bits])) >> (8 - bits));
	}
	sp->lzw_bitoff += sp->lzw_nbits;
	return (code);
}

/*
 * LZW Encoding.
 */

static void
DECLARE3(horizontalDifference8,
	register char*, cp,
	register int, cc,
	register int, stride
)
{
	if (cc > stride) {
		cc -= stride;
		cp += cc - 1;
		do {
			REPEAT4(stride, cp[stride] -= cp[0]; cp--)
			cc -= stride;
		} while (cc > 0);
	}
}

static void
DECLARE3(horizontalDifference16,
	char*, cp,
	int, cc,
	register int, stride
)
{
	register short *wp = (short *)cp;
	register int wc = cc/2;

	if (wc > stride) {
		wc -= stride;
		wp += wc - 1;
		do {
			REPEAT4(stride, wp[stride] -= wp[0]; wp--)
			wc -= stride;
		} while (wc > 0);
	}
}

/*
 * Reset encoding state at the start of a strip.
 */
static
LZWPreEncode(tif)
	TIFF *tif;
{
	register LZWState *sp = (LZWState *)tif->tif_data;

	if (sp == NULL) {
		tif->tif_data = malloc(sizeof (LZWState));
		if (tif->tif_data == NULL) {
			TIFFError("LZWPreEncode",
			    "No space for LZW state block");
			return (0);
		}
		sp = (LZWState *)tif->tif_data;
		sp->lzw_flags = 0;
		sp->lzw_hordiff = 0;
		if (!LZWCheckPredictor(tif, sp,
		    horizontalDifference8, horizontalDifference16))
			return (0);
		if (sp->lzw_hordiff) {
			tif->tif_encoderow = LZWEncodePredRow;
			tif->tif_encodestrip = LZWEncodePredTile;
			tif->tif_encodetile = LZWEncodePredTile;
		}
	}
	sp->enc_checkpoint = CHECK_GAP;
	SetMaxCode(sp, MAXCODE(sp->lzw_nbits = BITS_MIN)+1);
	cl_hash(sp);		/* clear hash table */
	sp->lzw_bitoff = 0;
	sp->lzw_bitsize = (tif->tif_rawdatasize << 3) - (BITS_MAX-1);
	sp->lzw_oldcode = -1;	/* generates CODE_CLEAR in LZWEncode */
	return (1);
}

/*
 * Encode a scanline of pixels.
 *
 * Uses an open addressing double hashing (no chaining) on the 
 * prefix code/next character combination.  We do a variant of
 * Knuth's algorithm D (vol. 3, sec. 6.4) along with G. Knott's
 * relatively-prime secondary probe.  Here, the modular division
 * first probe is gives way to a faster exclusive-or manipulation. 
 * Also do block compression with an adaptive reset, whereby the
 * code table is cleared when the compression ratio decreases,
 * but after the table fills.  The variable-length output codes
 * are re-sized at this point, and a CODE_CLEAR is generated
 * for the decoder. 
 */
static
LZWEncode(tif, bp, cc, s)
	TIFF *tif;
	u_char *bp;
	int cc;
	u_int s;
{
	static char module[] = "LZWEncode";
	register LZWState *sp;
	register long fcode;
	register int h, c, ent, disp;

	if ((sp = (LZWState *)tif->tif_data) == NULL)
		return (0);
	ent = sp->lzw_oldcode;
	if (ent == -1 && cc > 0) {
		PutNextCode(tif, CODE_CLEAR);
		ent = *bp++; cc--; sp->enc_incount++;
	}
	while (cc > 0) {
		c = *bp++; cc--; sp->enc_incount++;
		fcode = ((long)c << BITS_MAX) + ent;
		h = (c << HSHIFT) ^ ent;	/* xor hashing */
		if (sp->enc_htab[h] == fcode) {
			ent = sp->enc_codetab[h];
			continue;
		}
		if (sp->enc_htab[h] >= 0) {
			/*
			 * Primary hash failed, check secondary hash.
			 */
			disp = HSIZE - h;
			if (h == 0)
				disp = 1;
			do {
				if ((h -= disp) < 0)
					h += HSIZE;
				if (sp->enc_htab[h] == fcode) {
					ent = sp->enc_codetab[h];
					goto hit;
				}
			} while (sp->enc_htab[h] >= 0);
		}
		/*
		 * New entry, emit code and add to table.
		 */
		PutNextCode(tif, ent);
		ent = c;
		sp->enc_codetab[h] = sp->lzw_free_ent++;
		sp->enc_htab[h] = fcode;
		if (sp->lzw_free_ent == CODE_MAX-1) {
			/* table is full, emit clear code and reset */
			cl_hash(sp);
			PutNextCode(tif, CODE_CLEAR);
			SetMaxCode(sp, MAXCODE(sp->lzw_nbits = BITS_MIN)+1);
		} else {
			/*
			 * If the next entry is going to be too big for
			 * the code size, then increase it, if possible.
			 */
			if (sp->lzw_free_ent > sp->lzw_maxcode) {
				sp->lzw_nbits++;
				assert(sp->lzw_nbits <= BITS_MAX);
				SetMaxCode(sp, MAXCODE(sp->lzw_nbits)+1);
			} else if (sp->enc_incount >= sp->enc_checkpoint)
				cl_block(tif);
		}
	hit:
		;
	}
	sp->lzw_oldcode = ent;
	return (1);
}

static
LZWEncodePredRow(tif, bp, cc, s)
	TIFF *tif;
	u_char *bp;
	int cc;
	u_int s;
{
	LZWState *sp = (LZWState *)tif->tif_data;

/* XXX horizontal differencing alters user's data XXX */
	(*sp->lzw_hordiff)((char *)bp, cc, sp->lzw_stride);
	return (LZWEncode(tif, bp, cc, s));
}

static
LZWEncodePredTile(tif, bp0, cc0, s)
	TIFF *tif;
	u_char *bp0;
	int cc0;
	u_int s;
{
	LZWState *sp = (LZWState *)tif->tif_data;
	int cc = cc0;
	u_char *bp = bp0;

	while (cc > 0) {
		(*sp->lzw_hordiff)((char *)bp, sp->lzw_rowsize, sp->lzw_stride);
		cc -= sp->lzw_rowsize;
		bp += sp->lzw_rowsize;
	}
	return (LZWEncode(tif, bp0, cc0, s));
}

/*
 * Finish off an encoded strip by flushing the last
 * string and tacking on an End Of Information code.
 */
static
LZWPostEncode(tif)
	TIFF *tif;
{
	LZWState *sp = (LZWState *)tif->tif_data;

	if (sp->lzw_oldcode != -1) {
		PutNextCode(tif, sp->lzw_oldcode);
		sp->lzw_oldcode = -1;
	}
	PutNextCode(tif, CODE_EOI);
	return (1);
}

static void
PutNextCode(tif, c)
	TIFF *tif;
	int c;
{
	register LZWState *sp = (LZWState *)tif->tif_data;
	register long r_off;
	register int bits, code = c;
	register char *bp;

	r_off = sp->lzw_bitoff;
	bits = sp->lzw_nbits;
	/*
	 * Flush buffer if code doesn't fit.
	 */
	if (r_off + bits > sp->lzw_bitsize) {
		/*
		 * Calculate the number of full bytes that can be
		 * written and save anything else for the next write.
		 */
		if (r_off & 7) {
			tif->tif_rawcc = r_off >> 3;
			bp = tif->tif_rawdata + tif->tif_rawcc;
			(void) TIFFFlushData1(tif);
			tif->tif_rawdata[0] = *bp;
		} else {
			/*
			 * Otherwise, on a byte boundary (in
			 * which tiff_rawcc is already correct).
			 */
			(void) TIFFFlushData1(tif);
		}
		bp = tif->tif_rawdata;
		sp->lzw_bitoff = (r_off &= 7);
	} else {
		/*
		 * Get to the first byte.
		 */
		bp = tif->tif_rawdata + (r_off >> 3);
		r_off &= 7;
	}
	/*
	 * Note that lzw_bitoff is maintained as the bit offset
	 * into the buffer w/ a right-to-left orientation (i.e.
	 * lsb-to-msb).  The bits, however, go in the file in
	 * an msb-to-lsb order.
	 */
	bits -= (8 - r_off);
	*bp = (*bp & lmask[r_off]) | (code >> bits);
	bp++;
	if (bits >= 8) {
		bits -= 8;
		*bp++ = code >> bits;
	}
	if (bits)
		*bp = (code & rmask[bits]) << (8 - bits);
	/*
	 * enc_outcount is used by the compression analysis machinery
	 * which resets the compression tables when the compression
	 * ratio goes up.  lzw_bitoff is used here (in PutNextCode) for
	 * inserting codes into the output buffer.  tif_rawcc must
	 * be updated for the mainline write code in TIFFWriteScanline()
	 * so that data is flushed when the end of a strip is reached.
	 * Note that the latter is rounded up to ensure that a non-zero
	 * byte count is present. 
	 */
	sp->enc_outcount += sp->lzw_nbits;
	sp->lzw_bitoff += sp->lzw_nbits;
	tif->tif_rawcc = (sp->lzw_bitoff + 7) >> 3;
}

/*
 * Check compression ratio and, if things seem to
 * be slipping, clear the hash table and reset state.
 */
static void
cl_block(tif)
	TIFF *tif;
{
	register LZWState *sp = (LZWState *)tif->tif_data;
	register long rat;

	sp->enc_checkpoint = sp->enc_incount + CHECK_GAP;
	if (sp->enc_incount > 0x007fffff) {	/* shift will overflow */
		rat = sp->enc_outcount >> 8;
		rat = (rat == 0 ? 0x7fffffff : sp->enc_incount / rat);
	} else
		rat = (sp->enc_incount<<8)/sp->enc_outcount; /* 8 fract bits */
	if (rat <= sp->enc_ratio) {
		cl_hash(sp);
		PutNextCode(tif, CODE_CLEAR);
		SetMaxCode(sp, MAXCODE(sp->lzw_nbits = BITS_MIN)+1);
	} else
		sp->enc_ratio = rat;
}

/*
 * Reset code table and related statistics.
 */
static void
cl_hash(sp)
	LZWState *sp;
{
	register long *htab_p = sp->enc_htab+HSIZE;
	register long i, m1 = -1;

	i = HSIZE - 16;
 	do {
		*(htab_p-16) = m1;
		*(htab_p-15) = m1;
		*(htab_p-14) = m1;
		*(htab_p-13) = m1;
		*(htab_p-12) = m1;
		*(htab_p-11) = m1;
		*(htab_p-10) = m1;
		*(htab_p-9) = m1;
		*(htab_p-8) = m1;
		*(htab_p-7) = m1;
		*(htab_p-6) = m1;
		*(htab_p-5) = m1;
		*(htab_p-4) = m1;
		*(htab_p-3) = m1;
		*(htab_p-2) = m1;
		*(htab_p-1) = m1;
		htab_p -= 16;
	} while ((i -= 16) >= 0);
    	for (i += 16; i > 0; i--)
		*--htab_p = m1;

	sp->enc_ratio = 0;
	sp->enc_incount = 0;
	sp->enc_outcount = 0;
	sp->lzw_free_ent = CODE_FIRST;
}

static
LZWCleanup(tif)
	TIFF *tif;
{
	if (tif->tif_data) {
		free(tif->tif_data);
		tif->tif_data = NULL;
	}
}

/*
 * Copyright (c) 1985, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James A. Woods, derived from original work by Spencer Thomas
 * and Joseph Orost.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
