#ifndef lint
static char rcsid[] = "$Header: /usr/people/sam/tiff/libtiff/RCS/tif_aux.c,v 1.8 92/03/27 14:53:02 sam Exp $";
#endif

/*
 * Copyright (c) 1991, 1992 Sam Leffler
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
 * Auxiliary Support Routines.
 */
#include "tiffioP.h"
#include "prototypes.h"

/*
 * Like TIFFGetField, but return any default
 * value if the tag is not present in the directory.
 *
 * NB:	We use the value in the directory, rather than
 *	explcit values so that defaults exist only one
 *	place in the library -- in TIFFDefaultDirectory.
 */
TIFFVGetFieldDefaulted(tif, tag, ap)
	TIFF *tif;
	int tag;
	va_list ap;
{
	TIFFDirectory *td = &tif->tif_dir;
	int i;

	if (TIFFVGetField(tif, tag, ap))
		return (1);
	switch (tag) {
	case TIFFTAG_SUBFILETYPE:
		*va_arg(ap, u_short *) = td->td_subfiletype;
		return (1);
	case TIFFTAG_BITSPERSAMPLE:
		*va_arg(ap, u_short *) = td->td_bitspersample;
		return (1);
	case TIFFTAG_THRESHHOLDING:
		*va_arg(ap, u_short *) = td->td_threshholding;
		return (1);
	case TIFFTAG_FILLORDER:
		*va_arg(ap, u_short *) = td->td_fillorder;
		return (1);
	case TIFFTAG_ORIENTATION:
		*va_arg(ap, u_short *) = td->td_orientation;
		return (1);
	case TIFFTAG_SAMPLESPERPIXEL:
		*va_arg(ap, u_short *) = td->td_samplesperpixel;
		return (1);
	case TIFFTAG_ROWSPERSTRIP:
		*va_arg(ap, u_long *) = td->td_rowsperstrip;
		return (1);
	case TIFFTAG_MINSAMPLEVALUE:
		*va_arg(ap, u_short *) = td->td_minsamplevalue;
		return (1);
	case TIFFTAG_MAXSAMPLEVALUE:
		*va_arg(ap, u_short *) = td->td_maxsamplevalue;
		return (1);
	case TIFFTAG_PLANARCONFIG:
		*va_arg(ap, u_short *) = td->td_planarconfig;
		return (1);
	case TIFFTAG_GROUP4OPTIONS:
		*va_arg(ap, u_long *) = td->td_group4options;
		return (1);
	case TIFFTAG_RESOLUTIONUNIT:
		*va_arg(ap, u_short *) = td->td_resolutionunit;
		return (1);
	case TIFFTAG_PREDICTOR:
		*va_arg(ap, u_short *) = td->td_predictor;
		return (1);
#ifdef CMYK_SUPPORT
	case TIFFTAG_DOTRANGE:
		*va_arg(ap, u_short *) = 0;
		*va_arg(ap, u_short *) = (1<<td->td_bitspersample)-1;
		return (1);
	case TIFFTAG_INKSET:
		*va_arg(ap, u_short *) = td->td_inkset;
		return (1);
#endif
	case TIFFTAG_TILEDEPTH:
		*va_arg(ap, u_long *) = td->td_tiledepth;
		return (1);
	case TIFFTAG_DATATYPE:
		*va_arg(ap, u_short *) = td->td_sampleformat-1;
		return (1);
	case TIFFTAG_IMAGEDEPTH:
		*va_arg(ap, u_short *) = td->td_imagedepth;
		return (1);
#ifdef YCBCR_SUPPORT
	case TIFFTAG_YCBCRCOEFFICIENTS:
		if (!td->td_ycbcrcoeffs) {
			td->td_ycbcrcoeffs = (float *)malloc(3*sizeof (float));
			/* defaults are from CCIR Recommendation 601-1 */
			td->td_ycbcrcoeffs[0] = .299;
			td->td_ycbcrcoeffs[1] = .587;
			td->td_ycbcrcoeffs[2] = .114;
		}
		*va_arg(ap, float **) = td->td_ycbcrcoeffs;
		return (1);
	case TIFFTAG_YCBCRSUBSAMPLING:
		*va_arg(ap, u_short *) = td->td_ycbcrsubsampling[0];
		*va_arg(ap, u_short *) = td->td_ycbcrsubsampling[1];
		return (1);
	case TIFFTAG_YCBCRPOSITIONING:
		*va_arg(ap, u_short *) = td->td_ycbcrpositioning;
		return (1);
#endif
#ifdef COLORIMETRY_SUPPORT
	case TIFFTAG_TRANSFERFUNCTION:
		if (!td->td_transferfunction[0]) {
			u_short **tf = td->td_transferfunction;
			int n = 1<<td->td_bitspersample;

			tf[0] = (u_short *)malloc(n * sizeof (u_short));
			tf[0][0] = 0;
			for (i = 1; i < n; i++)
				tf[0][i] = (u_short)
				    floor(65535.*pow(i/(n-1.), 2.2) + .5);
			for (i = 1; i < td->td_samplesperpixel; i++) {
				tf[i] = (u_short *)malloc(n * sizeof (u_short));
				bcopy(tf[0], tf[i], n * sizeof (u_short));
			}
		}
		for (i = 0; i < td->td_samplesperpixel; i++)
			*va_arg(ap, u_short **) = td->td_transferfunction[i];
		return (1);
	case TIFFTAG_REFERENCEBLACKWHITE:
		if (!td->td_refblackwhite) {
			td->td_refblackwhite = (float *)
			    malloc(2*td->td_samplesperpixel * sizeof (float));
			for (i = 0; i < td->td_samplesperpixel; i++) {
				td->td_refblackwhite[2*i+0] = 0;
				td->td_refblackwhite[2*i+1] = 1L<<td->td_bitspersample;
			}
		}
		*va_arg(ap, float **) = td->td_refblackwhite;
		return (1);
#endif
	}
	return (0);
}

/*
 * Like TIFFGetField, but return any default
 * value if the tag is not present in the directory.
 */
/*VARARGS2*/
DECLARE2V(TIFFGetFieldDefaulted, TIFF*, tif, int, tag)
{
	int ok;
	va_list ap;

	VA_START(ap, tag);
	ok =  TIFFVGetFieldDefaulted(tif, tag, ap);
	va_end(ap);
	return (ok);
}
