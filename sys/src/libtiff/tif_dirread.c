#ifndef lint
static char rcsid[] = "$Header: /usr/people/sam/tiff/libtiff/RCS/tif_dirread.c,v 1.20 92/03/25 09:58:09 sam Exp $";
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
 * Directory Read Support Routines.
 *
 * NB: Beware of the varargs declarations for routines in
 *     this file.  The names and types of variables has been
 *     carefully chosen to make things work with compilers that
 *     are busted in one way or another (e.g. SGI/MIPS).
 */
#include "tiffioP.h"

#define	IGNORE	0		/* tag placeholder used below */

#if HAVE_IEEEFP
#define	TIFFCvtIEEEFloatToNative(tif, n, fp)
#endif

#include "prototypes.h"
#if USE_PROTOTYPES
static	EstimateStripByteCounts(TIFF *, TIFFDirEntry *, u_int);
static	MissingRequired(TIFF *, char *);
static	CheckDirCount(TIFF *, TIFFDirEntry *, u_long);
static	TIFFFetchData(TIFF *, TIFFDirEntry *, char *);
static	TIFFFetchString(TIFF *, TIFFDirEntry *, char *);
static	float TIFFFetchRational(TIFF *, TIFFDirEntry *);
static	TIFFFetchNormalTag(TIFF *, TIFFDirEntry *);
static	TIFFFetchPerSampleShorts(TIFF *, TIFFDirEntry *, long *);
static	TIFFFetchShortArray(TIFF *, TIFFDirEntry *, u_short []);
static	TIFFFetchStripThing(TIFF *, TIFFDirEntry *, long, u_long **);
static	TIFFFetchRefBlackWhite(TIFF *, TIFFDirEntry *);
static	TIFFFetchJPEGQTables(TIFF *, TIFFDirEntry *);
static	TIFFFetchJPEGCTables(TIFF *, TIFFDirEntry *, u_char ***);
static	TIFFFetchExtraSamples(TIFF *, TIFFDirEntry *);
static	float TIFFFetchFloat(TIFF *, TIFFDirEntry *);
static	int TIFFFetchFloatArray(TIFF *, TIFFDirEntry *, float *);
extern	int TIFFSetCompressionScheme(TIFF *, int);
extern	int TIFFDefaultDirectory(TIFF*);
extern	int TIFFFreeDirectory(TIFF*);
#else
static	EstimateStripByteCounts();
static	MissingRequired();
static	CheckDirCount();
static	TIFFFetchData();
static	TIFFFetchString();
static	float TIFFFetchRational();
static	TIFFFetchNormalTag();
static	TIFFFetchPerSampleShorts();
static	TIFFFetchShortArray();
static	TIFFFetchStripThing();
static	TIFFFetchRefBlackWhite();
static	TIFFFetchJPEGQTables();
static	TIFFFetchJPEGCTables();
static	TIFFFetchExtraSamples();
static	float TIFFFetchFloat();
static	int TIFFFetchFloatArray();
extern	int TIFFSetCompressionScheme();
extern	int TIFFDefaultDirectory();
extern	int TIFFFreeDirectory();
#endif

static char *
CheckMalloc(tif, n, what)
	TIFF *tif;
	int n;
	char *what;
{
	char *cp = malloc(n);
	if (cp == NULL)
		TIFFError(tif->tif_name, "No space %s", what);
	return (cp);
}

/*
 * Read the next TIFF directory from a file
 * and convert it to the internal format.
 * We read directories sequentially.
 */
TIFFReadDirectory(tif)
	TIFF *tif;
{
	register TIFFDirEntry *dp;
	register int n;
	register TIFFDirectory *td;
	TIFFDirEntry *dir;
	long v;
	TIFFFieldInfo *fip;
	u_short dircount;
	char *cp;
	int diroutoforderwarning = 0;

	tif->tif_diroff = tif->tif_nextdiroff;
	if (tif->tif_diroff == 0)		/* no more directories */
		return (0);
	tif->tif_curdir++;
	if (!isMapped(tif)) {
		if (!SeekOK(tif->tif_fd, tif->tif_diroff)) {
			TIFFError(tif->tif_name,
			    "Seek error accessing TIFF directory");
			return (0);
		}
		if (!ReadOK(tif->tif_fd, &dircount, sizeof (short))) {
			TIFFError(tif->tif_name,
			    "Can not read TIFF directory count");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		dir = (TIFFDirEntry *)CheckMalloc(tif,
		    dircount * sizeof (TIFFDirEntry), "to read TIFF directory");
		if (dir == NULL)
			return (0);
		if (!ReadOK(tif->tif_fd, dir, dircount*sizeof (TIFFDirEntry))) {
			TIFFError(tif->tif_name, "Can not read TIFF directory");
			goto bad;
		}
		/*
		 * Read offset to next directory for sequential scans.
		 */
		if (!ReadOK(tif->tif_fd, &tif->tif_nextdiroff, sizeof (long)))
			tif->tif_nextdiroff = 0;
#ifdef MMAP_SUPPORT
	} else {
		off_t off = tif->tif_diroff;

		if (off + sizeof (short) > tif->tif_size) {
			TIFFError(tif->tif_name,
			    "Can not read TIFF directory count");
			return (0);
		} else
			bcopy(tif->tif_base + off, &dircount, sizeof (short));
		off += sizeof (short);
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		dir = (TIFFDirEntry *)CheckMalloc(tif,
		    dircount * sizeof (TIFFDirEntry), "to read TIFF directory");
		if (dir == NULL)
			return (0);
		if (off + dircount*sizeof (TIFFDirEntry) > tif->tif_size) {
			TIFFError(tif->tif_name, "Can not read TIFF directory");
			goto bad;
		} else
			bcopy(tif->tif_base + off, dir,
			    dircount*sizeof (TIFFDirEntry));
		off += dircount* sizeof (TIFFDirEntry);
		if (off + sizeof (long) < tif->tif_size)
			bcopy(tif->tif_base + off, &tif->tif_nextdiroff,
			    sizeof (long));
		else
			tif->tif_nextdiroff = 0;
#endif
	}
	if (tif->tif_flags & TIFF_SWAB)
		TIFFSwabLong((u_long *)&tif->tif_nextdiroff);

	tif->tif_flags &= ~TIFF_BEENWRITING;	/* reset before new dir */
	/*
	 * Setup default value and then make a pass over
	 * the fields to check type and tag information,
	 * and to extract info required to size data
	 * structures.  A second pass is made afterwards
	 * to read in everthing not taken in the first pass.
	 */
	td = &tif->tif_dir;
	/* free any old stuff and reinit */
	TIFFFreeDirectory(tif);
	TIFFDefaultDirectory(tif);
	/*
	 * Electronic Arts writes gray-scale TIFF files
	 * without a PlanarConfiguration directory entry.
	 * Thus we setup a default value here, even though
	 * the TIFF spec says there is no default value.
	 */
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	for (fip = tiffFieldInfo, dp = dir, n = dircount; n > 0; n--, dp++) {
		if (tif->tif_flags & TIFF_SWAB) {
			TIFFSwabArrayOfShort(&dp->tdir_tag, 2);
			TIFFSwabArrayOfLong(&dp->tdir_count, 2);
		}
		/*
		 * Find the field information entry for this tag.
		 */
		/*
		 * Silicon Beach (at least) writes unordered
		 * directory tags (violating the spec).  Handle
		 * it here, but be obnoxious (maybe they'll fix it?).
		 */
		if (dp->tdir_tag < fip->field_tag) {
			if (!diroutoforderwarning) {
				TIFFWarning(tif->tif_name,
	"invalid TIFF directory; tags are not sorted in ascending order");
				diroutoforderwarning = 1;
			}
			fip = tiffFieldInfo;	/* O(n^2) */
		}
		while (fip->field_tag && fip->field_tag < dp->tdir_tag)
			fip++;
		if (!fip->field_tag || fip->field_tag != dp->tdir_tag) {
			TIFFWarning(tif->tif_name,
			    "unknown field with tag %d (0x%x) ignored",
			    dp->tdir_tag,  dp->tdir_tag);
			dp->tdir_tag = IGNORE;
			fip = tiffFieldInfo;	/* restart search */
			continue;
		}
		/*
		 * Null out old tags that we ignore.
		 */
		if (fip->field_bit == FIELD_IGNORE) {
	ignore:
			dp->tdir_tag = IGNORE;
			continue;
		}
		/*
		 * Check data type.
		 */
		while (dp->tdir_type != (u_short)fip->field_type) {
			if (fip->field_type == TIFF_ANY)	/* wildcard */
				break;
			fip++;
			if (!fip->field_tag || fip->field_tag != dp->tdir_tag) {
				TIFFWarning(tif->tif_name,
				   "wrong data type %d for \"%s\"; tag ignored",
				    dp->tdir_type, fip[-1].field_name);
				goto ignore;
			}
		}
		/*
		 * Check count if known in advance.
		 */
		if (fip->field_readcount != TIFF_VARIABLE) {
			u_long expected = (fip->field_readcount == TIFF_SPP) ?
			    (u_long) td->td_samplesperpixel :
			    (u_long) fip->field_readcount;
			if (!CheckDirCount(tif, dp, expected))
				goto ignore;
		}

		switch (dp->tdir_tag) {
		case TIFFTAG_STRIPOFFSETS:
		case TIFFTAG_STRIPBYTECOUNTS:
		case TIFFTAG_TILEOFFSETS:
		case TIFFTAG_TILEBYTECOUNTS:
			TIFFSetFieldBit(tif, fip->field_bit);
			break;
		case TIFFTAG_IMAGEWIDTH:
		case TIFFTAG_IMAGELENGTH:
		case TIFFTAG_IMAGEDEPTH:
		case TIFFTAG_TILELENGTH:
		case TIFFTAG_TILEWIDTH:
		case TIFFTAG_TILEDEPTH:
		case TIFFTAG_PLANARCONFIG:
		case TIFFTAG_SAMPLESPERPIXEL:
		case TIFFTAG_ROWSPERSTRIP:
			if (!TIFFFetchNormalTag(tif, dp))
				goto bad;
			break;
		}
	}

	/*
	 * Allocate directory structure and setup defaults.
	 */
	if (!TIFFFieldSet(tif, FIELD_IMAGEDIMENSIONS)) {
		MissingRequired(tif, "ImageLength");
		goto bad;
	}
	if (!TIFFFieldSet(tif, FIELD_PLANARCONFIG)) {
		MissingRequired(tif, "PlanarConfiguration");
		goto bad;
	}
	/* 
 	 * Setup appropriate structures (by strip or by tile)
	 */
	if (!TIFFFieldSet(tif, FIELD_TILEDIMENSIONS)) {
		td->td_stripsperimage = (td->td_rowsperstrip == 0xffffffff ?
		     (td->td_imagelength != 0 ? 1 : 0) :
		     howmany(td->td_imagelength, td->td_rowsperstrip));
		td->td_tilewidth = td->td_imagewidth;
		td->td_tilelength = td->td_rowsperstrip;
		td->td_tiledepth = td->td_imagedepth;
		tif->tif_flags &= ~TIFF_ISTILED;
	} else {
		td->td_stripsperimage = TIFFNumberOfTiles(tif);
		tif->tif_flags |= TIFF_ISTILED;
	}
	td->td_nstrips = td->td_stripsperimage;
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE)
		td->td_nstrips *= td->td_samplesperpixel;
	if (td->td_nstrips > 0 && !TIFFFieldSet(tif, FIELD_STRIPOFFSETS)) {
		MissingRequired(tif,
		    isTiled(tif) ? "TileOffsets" : "StripOffsets");
		goto bad;
	}

	/*
	 * Second pass: extract other information.
	 */
	for (dp = dir, n = dircount; n > 0; n--, dp++) {
		if (dp->tdir_tag == IGNORE)
			continue;
		switch (dp->tdir_tag) {
		case TIFFTAG_COMPRESSION:
		case TIFFTAG_MINSAMPLEVALUE:
		case TIFFTAG_MAXSAMPLEVALUE:
		case TIFFTAG_BITSPERSAMPLE:
			/*
			 * The 5.0 spec says the Compression tag has
			 * one value, while earlier specs say it has
			 * one value per sample.  Because of this, we
			 * accept the tag if one value is supplied.
			 *
			 * The MinSampleValue, MaxSampleValue and
			 * BitsPerSample tags are supposed to be written
			 * as one value/sample, but some vendors incorrectly
			 * write one value only -- so we accept that
			 * as well (yech).
			 */
			if (dp->tdir_count == 1) {
				v = TIFFExtractData(tif,
				    dp->tdir_type, dp->tdir_offset);
				if (!TIFFSetField(tif, dp->tdir_tag, (int)v))
					goto bad;
				break;
			}
			/* fall thru... */
		case TIFFTAG_DATATYPE:
		case TIFFTAG_SAMPLEFORMAT:
			if (!TIFFFetchPerSampleShorts(tif, dp, &v) ||
			    !TIFFSetField(tif, dp->tdir_tag, (int)v))
				goto bad;
			break;
		case TIFFTAG_STRIPOFFSETS:
		case TIFFTAG_TILEOFFSETS:
			if (!TIFFFetchStripThing(tif, dp,
			    td->td_nstrips, &td->td_stripoffset))
				goto bad;
			break;
		case TIFFTAG_STRIPBYTECOUNTS:
		case TIFFTAG_TILEBYTECOUNTS:
			if (!TIFFFetchStripThing(tif, dp,
			    td->td_nstrips, &td->td_stripbytecount))
				goto bad;
			break;
		case TIFFTAG_IMAGELENGTH:
		case TIFFTAG_ROWSPERSTRIP:
		case TIFFTAG_TILELENGTH:
		case TIFFTAG_TILEWIDTH:
		case TIFFTAG_TILEDEPTH:
		case TIFFTAG_SAMPLESPERPIXEL:
		case TIFFTAG_PLANARCONFIG:
			/* handled in first pass above */
			break;
		case TIFFTAG_COLORMAP:
			if (!CheckDirCount(tif,dp,3*(1L<<td->td_bitspersample)))
				break;
			/* fall thru... */
		case TIFFTAG_TRANSFERFUNCTION:
			v = (1L<<td->td_bitspersample) * sizeof (u_short);
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (u_short),
			    "to read \"TransferFunction\" tag");
			if (cp != NULL) {
				if (TIFFFetchData(tif, dp, cp)) {
					/*
					 * This deals with there being only
					 * one array to apply to all samples.
					 */
					if (dp->tdir_count == 1L<<td->td_bitspersample)
						v = 0;
					/* NB: we assume samples/pixel <= 4 */
					TIFFSetField(tif, dp->tdir_tag,
					    cp, cp+v, cp+2*v, cp+3*v);
				}
				free(cp);
			}
			break;
		case TIFFTAG_PAGENUMBER:
			if (TIFFFetchShortArray(tif, dp, td->td_pagenumber))
				TIFFSetFieldBit(tif, FIELD_PAGENUMBER);
			break;
		case TIFFTAG_HALFTONEHINTS:
			if (TIFFFetchShortArray(tif, dp, td->td_halftonehints))
				TIFFSetFieldBit(tif, FIELD_HALFTONEHINTS);
			break;
#ifdef COLORIMETRY_SUPPORT
		case TIFFTAG_REFERENCEBLACKWHITE:
			(void) TIFFFetchRefBlackWhite(tif, dp);
			break;
#endif
#ifdef YCBCR_SUPPORT
		case TIFFTAG_YCBCRSUBSAMPLING:
			if (TIFFFetchShortArray(tif, dp, td->td_ycbcrsubsampling))
				TIFFSetFieldBit(tif, FIELD_YCBCRSUBSAMPLING);
			break;
#endif
#ifdef CMYK_SUPPORT
		case TIFFTAG_DOTRANGE:
			if (TIFFFetchShortArray(tif, dp, td->td_dotrange))
				TIFFSetFieldBit(tif, FIELD_DOTRANGE);
			break;
#endif
#ifdef JPEG_SUPPORT
		case TIFFTAG_JPEGQTABLES:
			if (TIFFFetchJPEGQTables(tif, dp))
				TIFFSetFieldBit(tif, FIELD_JPEGQTABLES);
			break;
		case TIFFTAG_JPEGDCTABLES:
			if (TIFFFetchJPEGCTables(tif, dp, &td->td_dctab))
				TIFFSetFieldBit(tif, FIELD_JPEGDCTABLES);
			break;
		case TIFFTAG_JPEGACTABLES:
			if (TIFFFetchJPEGCTables(tif, dp, &td->td_actab))
				TIFFSetFieldBit(tif, FIELD_JPEGACTABLES);
			break;
#endif
		case TIFFTAG_EXTRASAMPLES:
			(void) TIFFFetchExtraSamples(tif, dp);
			break;
/* BEGIN REV 4.0 COMPATIBILITY */
		case TIFFTAG_OSUBFILETYPE:
			v = 0;
			switch (TIFFExtractData(tif, dp->tdir_type,
			    dp->tdir_offset)) {
			case OFILETYPE_REDUCEDIMAGE:
				v = FILETYPE_REDUCEDIMAGE;
				break;
			case OFILETYPE_PAGE:
				v = FILETYPE_PAGE;
				break;
			}
			if (v)
				(void) TIFFSetField(tif,
				    TIFFTAG_SUBFILETYPE, (int)v);
			break;
/* END REV 4.0 COMPATIBILITY */
		default:
			(void) TIFFFetchNormalTag(tif, dp);
			break;
		}
	}
	/*
	 * Verify Palette image has a Colormap.
	 */
	if (td->td_photometric == PHOTOMETRIC_PALETTE &&
	    !TIFFFieldSet(tif, FIELD_COLORMAP)) {
		MissingRequired(tif, "Colormap");
		goto bad;
	}
	/*
	 * Attempt to deal with a missing StripByteCounts tag.
	 */
	if (!TIFFFieldSet(tif, FIELD_STRIPBYTECOUNTS)) {
		/*
		 * Some manufacturers violate the spec by not giving
		 * the size of the strips.  In this case, assume there
		 * is one uncompressed strip of data.
		 */
		if (td->td_nstrips > 1) {
		    MissingRequired(tif, "StripByteCounts");
		    goto bad;
		}
		TIFFWarning(tif->tif_name,
"TIFF directory is missing required \"%s\" field, calculating from imagelength",
		    TIFFFieldWithTag(TIFFTAG_STRIPBYTECOUNTS)->field_name);
		EstimateStripByteCounts(tif, dir, dircount);
	} else if (td->td_nstrips == 1 && td->td_stripbytecount[0] == 0) {
		/*
		 * Plexus (and others) sometimes give a value
		 * of zero for a tag when they don't know what
		 * the correct value is!  Try and handle the
		 * simple case of estimating the size of a one
		 * strip image.
		 */
		TIFFWarning(tif->tif_name,
"Bogus \"%s\" field, ignoring and calculating from imagelength",
		    TIFFFieldWithTag(TIFFTAG_STRIPBYTECOUNTS)->field_name);
		EstimateStripByteCounts(tif, dir, dircount);
	}
	if (dir)
		free((char *)dir);
	if (!TIFFFieldSet(tif, FIELD_MAXSAMPLEVALUE))
		td->td_maxsamplevalue = (1L<<td->td_bitspersample)-1;
	/*
	 * Setup default compression scheme.
	 */
	if (!TIFFFieldSet(tif, FIELD_COMPRESSION))
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	/*
	 * Reinitialize i/o since we are starting on a new directory.
	 */
	tif->tif_row = -1;
	tif->tif_curstrip = -1;
	tif->tif_col = -1;
	tif->tif_curtile = -1;
	tif->tif_tilesize = TIFFTileSize(tif);
	tif->tif_scanlinesize = TIFFScanlineSize(tif);
	return (1);
bad:
	if (dir)
		free((char *)dir);
	return (0);
}

static
EstimateStripByteCounts(tif, dir, dircount)
	TIFF *tif;
	TIFFDirEntry *dir;
	u_int dircount;
{
	register TIFFDirEntry *dp;
	register TIFFDirectory *td = &tif->tif_dir;
	register int n;

	td->td_stripbytecount = (u_long *)
	    CheckMalloc(tif, sizeof (u_long), "for \"StripByteCounts\" array");
	if (td->td_compression != COMPRESSION_NONE) {
		u_long space = sizeof (TIFFHeader)
		    + sizeof (short)
		    + (dircount * sizeof (TIFFDirEntry))
		    + sizeof (long);
		long filesize = TIFFGetFileSize(tif->tif_fd);
		/* calculate amount of space used by indirect values */
		for (dp = dir, n = dircount; n > 0; n--, dp++) {
			int cc = dp->tdir_count * tiffDataWidth[dp->tdir_type];
			if (cc > sizeof (long))
				space += cc;
		}
		td->td_stripbytecount[0] = filesize - space;
		/*
		 * This gross hack handles the case were the offset to
		 * the strip is past the place where we think the strip
		 * should begin.  Since a strip of data must be contiguous,
		 * it's safe to assume that we've overestimated the amount
		 * of data in the strip and trim this number back accordingly.
		 */ 
		if (td->td_stripoffset[0] + td->td_stripbytecount[0] > filesize)
			td->td_stripbytecount[0] =
			    filesize - td->td_stripoffset[0];
	} else {
		u_long rowbytes = howmany(td->td_bitspersample *
		    td->td_samplesperpixel * td->td_imagewidth, 8);
		td->td_stripbytecount[0] = td->td_imagelength * rowbytes;
	}
	TIFFSetFieldBit(tif, FIELD_STRIPBYTECOUNTS);
	if (!TIFFFieldSet(tif, FIELD_ROWSPERSTRIP))
		td->td_rowsperstrip = td->td_imagelength;
}

static
MissingRequired(tif, tagname)
	TIFF *tif;
	char *tagname;
{
	TIFFError(tif->tif_name,
	    "TIFF directory is missing required \"%s\" field", tagname);
}

/*
 * Check the count field of a directory
 * entry against a known value.  The caller
 * is expected to skip/ignore the tag if
 * there is a mismatch.
 */
static
CheckDirCount(tif, dir, count)
	TIFF *tif;
	TIFFDirEntry *dir;
	u_long count;
{
	if (count != dir->tdir_count) {
		TIFFWarning(tif->tif_name,
	"incorrect count for field \"%s\" (%lu, expecting %lu); tag ignored",
		    TIFFFieldWithTag(dir->tdir_tag)->field_name,
		    dir->tdir_count, count);
		return (0);
	}
	return (1);
}

/*
 * Fetch a contiguous directory item.
 */
static
TIFFFetchData(tif, dir, cp)
	TIFF *tif;
	TIFFDirEntry *dir;
	char *cp;
{
	int cc, w;

	w = tiffDataWidth[dir->tdir_type];
	cc = dir->tdir_count * w;
	if (!isMapped(tif)) {
		if (!SeekOK(tif->tif_fd, dir->tdir_offset))
			goto bad;
		if (!ReadOK(tif->tif_fd, cp, cc))
			goto bad;
#ifdef MMAP_SUPPORT
	} else {
		if (dir->tdir_offset + cc > tif->tif_size)
			goto bad;
		bcopy(tif->tif_base + dir->tdir_offset, cp, cc);
#endif
	}
	if (tif->tif_flags & TIFF_SWAB) {
		switch (dir->tdir_type) {
		case TIFF_SHORT:
		case TIFF_SSHORT:
			TIFFSwabArrayOfShort((u_short *)cp, dir->tdir_count);
			break;
		case TIFF_LONG:
		case TIFF_SLONG:
		case TIFF_FLOAT:
			TIFFSwabArrayOfLong((u_long *)cp, dir->tdir_count);
			break;
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
			TIFFSwabArrayOfLong((u_long *)cp, 2*dir->tdir_count);
			break;
		}
	}
	return (cc);
bad:
	TIFFError(tif->tif_name, "Error fetching data for field \"%s\"",
	    TIFFFieldWithTag(dir->tdir_tag)->field_name);
	return (0);
}

/*
 * Fetch an ASCII item from the file.
 */
static
TIFFFetchString(tif, dir, cp)
	TIFF *tif;
	TIFFDirEntry *dir;
	char *cp;
{
	if (dir->tdir_count <= 4) {
		u_long l = dir->tdir_offset;
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong(&l);
		bcopy(&l, cp, dir->tdir_count);
		return (1);
	}
	return (TIFFFetchData(tif, dir, cp));
}

/*
 * Convert numerator+denominator to float.
 */
static int
cvtRational(tif, dir, num, denom, rv)
	TIFF *tif;
	TIFFDirEntry *dir;
	u_long num, denom;
	float *rv;
{
	if (denom == 0) {
		TIFFError(tif->tif_name,
		    "%s: Rational with zero denominator (num = %lu)",
		    TIFFFieldWithTag(dir->tdir_tag)->field_name, num);
		return (0);
	} else {
		if (dir->tdir_type == TIFF_RATIONAL)
			*rv = ((float)num / (float)denom);
		else
			*rv = ((float)(long)num / (float)(long)denom);
		return (1);
	}
}

/*
 * Fetch a rational item from the file
 * at offset off and return the value
 * as a floating point number.
 */
static float
TIFFFetchRational(tif, dir)
	TIFF *tif;
	TIFFDirEntry *dir;
{
	u_long l[2];
	float v;

	return (!TIFFFetchData(tif, dir, (char *)l) ||
	    !cvtRational(tif, dir, l[0], l[1], &v) ? 1. : v);
}

/*
 * Fetch a single floating point value
 * from the offset field and return it
 * as a native float.
 */
static float
TIFFFetchFloat(tif, dir)
	TIFF *tif;
	TIFFDirEntry *dir;
{
	float v = (float)
	    TIFFExtractData(tif, dir->tdir_type, dir->tdir_offset);
	TIFFCvtIEEEFloatToNative(tif, 1, &v);
	return (v);
}

/*
 * Fetch an array of BYTE or SBYTE values.
 */
static
TIFFFetchByteArray(tif, dir, v)
	TIFF *tif;
	TIFFDirEntry *dir;
	u_short v[];
{

	if (dir->tdir_count <= 4) {
		/*
		 * Extract data from offset field.
		 */
		if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN) {
			switch (dir->tdir_count) {
			case 4: v[3] = dir->tdir_offset & 0xff;
			case 3: v[2] = (dir->tdir_offset >> 8) & 0xff;
			case 2: v[1] = (dir->tdir_offset >> 16) & 0xff;
			case 1: v[0] = dir->tdir_offset >> 24;
			}
		} else {
			switch (dir->tdir_count) {
			case 4: v[3] = dir->tdir_offset >> 24;
			case 3: v[2] = (dir->tdir_offset >> 16) & 0xff;
			case 2: v[1] = (dir->tdir_offset >> 8) & 0xff;
			case 1: v[0] = dir->tdir_offset & 0xff;
			}
		}
		return (1);
	} else
		return (TIFFFetchData(tif, dir, (char *)v));	/* XXX */
}

/*
 * Fetch an array of SHORT or SSHORT values.
 */
static
TIFFFetchShortArray(tif, dir, v)
	TIFF *tif;
	TIFFDirEntry *dir;
	u_short v[];
{
	if (dir->tdir_count <= 2) {
		if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN) {
			switch (dir->tdir_count) {
			case 2: v[1] = dir->tdir_offset & 0xffff;
			case 1: v[0] = dir->tdir_offset >> 16;
			}
		} else {
			switch (dir->tdir_count) {
			case 2: v[1] = dir->tdir_offset >> 16;
			case 1: v[0] = dir->tdir_offset & 0xffff;
			}
		}
		return (1);
	} else
		return (TIFFFetchData(tif, dir, (char *)v));
}

/*
 * Fetch an array of LONG or SLONG values.
 */
static
TIFFFetchLongArray(tif, dir, v)
	TIFF *tif;
	TIFFDirEntry *dir;
	u_long v[];
{
	if (dir->tdir_count == 1) {
		v[0] = dir->tdir_offset;
		return (1);
	} else
		return (TIFFFetchData(tif, dir, (char *)v));
}

/*
 * Fetch an array of RATIONAL or SRATIONAL values.
 */
static
TIFFFetchRationalArray(tif, dir, v)
	TIFF *tif;
	TIFFDirEntry *dir;
	float v[];
{
	int ok = 0;
	u_long *l;

	l = (u_long *)CheckMalloc(tif,
	    dir->tdir_count*tiffDataWidth[dir->tdir_type],
	    "to fetch array of rationals");
	if (l) {
		if (TIFFFetchData(tif, dir, (char *)l)) {
			u_long i;
			for (i = 0; i < dir->tdir_count; i++) {
				ok = cvtRational(tif, dir,
				    l[2*i+0], l[2*i+1], &v[i]);
				if (!ok)
					break;
			}
		}
		free((char *)l);
	}
	return (ok);
}

/*
 * Fetch an array of FLOAT values.
 */
static
TIFFFetchFloatArray(tif, dir, v)
	TIFF *tif;
	TIFFDirEntry *dir;
	float v[];
{
	if (TIFFFetchData(tif, dir, (char *)v)) {
		TIFFCvtIEEEFloatToNative(tif, dir->tdir_count, v);
		return (1);
	} else
		return (0);
}

/*
 * Fetch a tag that is not handled by special case code.
 *
 * NB: DOUBLE and UNDEFINED types are not handled.
 */
static
TIFFFetchNormalTag(tif, dp)
	TIFF *tif;
	TIFFDirEntry *dp;
{
	static char mesg[] = "to fetch tag value";
	int ok = 0;

	if (dp->tdir_count > 1) {		/* array of values */
		char *cp = NULL;

		switch (dp->tdir_type) {
		case TIFF_BYTE:
		case TIFF_SBYTE:
			/* NB: always expand BYTE values to shorts */
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (u_short), mesg);
			ok = cp && TIFFFetchByteArray(tif, dp, (u_short *)cp);
			break;
		case TIFF_SHORT:
		case TIFF_SSHORT:
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (u_short), mesg);
			ok = cp && TIFFFetchShortArray(tif, dp, (u_short *)cp);
			break;
		case TIFF_LONG:
		case TIFF_SLONG:
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (u_long), mesg);
			ok = cp && TIFFFetchLongArray(tif, dp, (u_long *)cp);
			break;
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (float), mesg);
			ok = cp && TIFFFetchRationalArray(tif, dp, (float *)cp);
			break;
		case TIFF_FLOAT:
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (float), mesg);
			ok = cp && TIFFFetchFloatArray(tif, dp, (float *)cp);
			break;
		case TIFF_ASCII:
			/*
			 * Some vendors write strings w/o the trailing
			 * NULL byte, so always append one just in case.
			 */
			cp = CheckMalloc(tif, dp->tdir_count+1, mesg);
			if (ok = (cp && TIFFFetchString(tif, dp, cp)))
				cp[dp->tdir_count] = '\0';	/* XXX */
			break;
		}
		if (ok)
			ok = TIFFSetField(tif, dp->tdir_tag, cp);
		if (cp != NULL)
			free(cp);
	} else if (CheckDirCount(tif, dp, 1)) {	/* singleton value */
		char c[2];
		switch (dp->tdir_type) {
		case TIFF_BYTE:
		case TIFF_SBYTE:
		case TIFF_SHORT:
		case TIFF_SSHORT:
			ok = TIFFSetField(tif, dp->tdir_tag, (int)
		  TIFFExtractData(tif, dp->tdir_type, dp->tdir_offset));
			break;
		case TIFF_LONG:
		case TIFF_SLONG:
			ok = TIFFSetField(tif, dp->tdir_tag, (u_long)
		  TIFFExtractData(tif, dp->tdir_type, dp->tdir_offset));
			break;
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
			ok = TIFFSetField(tif, dp->tdir_tag,
			    TIFFFetchRational(tif, dp));
			break;
		case TIFF_FLOAT:
			ok = TIFFSetField(tif, dp->tdir_tag,
			    TIFFFetchFloat(tif, dp));
			break;
		case TIFF_ASCII:
			if (ok = (TIFFFetchString(tif, dp, c))) {
				c[1] = '\0';		/* XXX paranoid */
				ok = TIFFSetField(tif, dp->tdir_tag, c);
			}
			break;
		}
	}
	return (ok);
}

/*
 * Fetch samples/pixel short values for 
 * the specified tag and verify that
 * all values are the same.
 */
static
TIFFFetchPerSampleShorts(tif, dir, pl)
	TIFF *tif;
	TIFFDirEntry *dir;
	long *pl;
{
	u_short v[4];
	int samples = tif->tif_dir.td_samplesperpixel;

	if (CheckDirCount(tif, dir, (u_long)samples) &&
	    TIFFFetchShortArray(tif, dir, v)) {
		int i;
		for (i = 1; i < samples; i++)
			if (v[i] != v[0]) {
				TIFFError(tif->tif_name,
		"Cannot handle different per-sample values for field \"%s\"",
				   TIFFFieldWithTag(dir->tdir_tag)->field_name);
				return (0);
			}
		*pl = v[0];
		return (1);
	}
	return (0);
}

/*
 * Fetch a set of offsets or lengths.
 * While this routine says "strips",
 * in fact it's also used for tiles.
 */
static
TIFFFetchStripThing(tif, dir, nstrips, lpp)
	TIFF *tif;
	TIFFDirEntry *dir;
	long nstrips;
	u_long **lpp;
{
	register u_long *lp;
	int status;

	if (!CheckDirCount(tif, dir, nstrips))
		return (0);
	/*
	 * Allocate space for strip information.
	 */
	if (*lpp == NULL &&
	    (*lpp = (u_long *)CheckMalloc(tif,
	      nstrips * sizeof (u_long), "for strip array")) == NULL)
		return (0);
	lp = *lpp;
	if (dir->tdir_type == (int)TIFF_SHORT) {
		/*
		 * Handle short->long expansion.
		 */
		u_short *dp = (u_short *)CheckMalloc(tif,
		    dir->tdir_count* sizeof (u_short), "to fetch strip tag");
		if (dp == NULL)
			return (0);
		if (status = TIFFFetchShortArray(tif, dir, dp)) {
			register u_short *wp = dp;
			while (nstrips-- > 0)
				*lp++ = *wp++;
		}
		free((char *)dp);
	} else
		status = TIFFFetchLongArray(tif, dir, lp);
	return (status);
}

#ifdef COLORIMETRY_SUPPORT
static
TIFFFetchRefBlackWhite(tif, dir)
	TIFF *tif;
	TIFFDirEntry *dir;
{
	static char mesg[] = "for \"ReferenceBlackWhite\" array";
	char *cp;
	int ok;

	if (!CheckDirCount(tif, dir, 2*tif->tif_dir.td_samplesperpixel))
		return (0);
	if (dir->tdir_type == TIFF_RATIONAL)
		return (TIFFFetchNormalTag(tif, dir));
	/*
	 * Handle LONG's for backward compatibility.
	 */
	cp = CheckMalloc(tif, dir->tdir_count * sizeof (u_long), mesg);
	if (ok = (cp && TIFFFetchLongArray(tif, dir, (u_long *)cp))) {
		float *fp = (float *)
		    CheckMalloc(tif, dir->tdir_count * sizeof (float), mesg);
		if (ok = (fp != NULL)) {
			int i;
			for (i = 0; i < dir->tdir_count; i++)
				fp[i] = (float)((u_long *)cp)[i];
			ok = TIFFSetField(tif, dir->tdir_tag, fp);
			free((char *)fp);
		}
	}
	if (cp)
		free(cp);
	return (ok);
}
#endif

#ifdef JPEG_SUPPORT
/*
 * Fetch the JPEG Quantization tables
 * for the specified directory entry.
 * Storage for the td_qtab array is
 * allocated as a side effect.
 */
static
TIFFFetchJPEGQTables(tif, dir)
	TIFF *tif;
	TIFFDirEntry *dir;
{
	TIFFDirectory *td = &tif->tif_dir;
	long off[4];
	int i, j;
	TIFFDirEntry tdir;
	char *qmat;

	if (dir->tdir_count > 1) {
		/* XXX verify count <= 4 */
		if (!TIFFFetchData(tif, dir, (char *)off))
			return (0);
	} else
		off[0] = dir->tdir_offset;
	/*
	 * We don't share per-component q matrices because
	 * (besides complicating this logic even more), it
	 * would make it very painful if the user does a ``set''.
	 */
	td->td_qtab = (u_char **)CheckMalloc(tif,
	    dir->tdir_count*(sizeof (u_char *) + 64*sizeof (u_char)),
	    "for JPEG Q table");
	if (td->td_qtab == NULL)
		return (0);
	tdir.tdir_type = TIFF_BYTE;
	tdir.tdir_count = 64;
	qmat = (((char *)td->td_qtab) + dir->tdir_count*sizeof (u_char *));
	for (i = 0; i < dir->tdir_count; i++) {
		td->td_qtab[i] = (u_char *)qmat;
		tdir.tdir_offset = off[i];
		if (!TIFFFetchData(tif, &tdir, qmat))
			return (0);
		qmat += 64*sizeof (u_char);
	}
	return (1);
}

/*
 * Fetch JPEG Huffman code tables for the
 * specified directory entry.  Storage for
 * the tables are allocated as a side effect.
 */
static
TIFFFetchJPEGCTables(tif, dir, ptab)
	TIFF *tif;
	TIFFDirEntry *dir;
	u_char ***ptab;
{
	long off[4];
	int i, j, ncodes;
	TIFFDirEntry tdir;
	char *tab;

	if (dir->tdir_count > 1) {
		/* XXX verify count <= 4 */
		if (!TIFFFetchData(tif, dir, (char *)off))
			return (0);
	} else
		off[0] = dir->tdir_offset;
	/*
	 * We don't share per-component tables because
	 * (besides complicating this logic even more), it
	 * would make it very painful if the user does a
	 * ``set''.  Note also that we don't try to optimize
	 * storage of the tables -- we just allocate enough
	 * space to hold the largest possible.  All this
	 * stuff is so complicated 'cuz the tag is defined
	 * to be compatible with the JPEG table format,
	 * rather than something that fits well into the
	 * structure of TIFF -- argh!
	 */
	*ptab = (u_char **)CheckMalloc(tif, dir->tdir_count*
	    (sizeof (u_char *) + (16+256)*sizeof (u_char)),
	    "for JPEG Huffman table");
	if (*ptab == NULL)
		return (0);
	tdir.tdir_type = TIFF_BYTE;
	tab = (((char *)*ptab) + dir->tdir_count*sizeof (u_char *));
	for (i = 0; i < dir->tdir_count; i++) {
		(*ptab)[i] = (u_char *)tab;
		tdir.tdir_offset = off[i];
		tdir.tdir_count = 16;
		/*
		 * We must fetch the array that holds the
		 * count of codes for each bit length first
		 * and the count up the number of codes that
		 * are in the variable length table.  This
		 * information is implicit in the JPEG format
		 * 'cuz it's preceded by a length field.
		 */
		if (!TIFFFetchData(tif, &tdir, tab))	/* count array */
			return (0);
		for (ncodes = 0, j = 0; j < 16; j++)
			ncodes += tab[j];
		/*
		 * Adjust offsets and fetch codes separately.
		 */
		tdir.tdir_offset += 16;
		tdir.tdir_count = ncodes;
		tab += 16;
		if (!TIFFFetchData(tif, &tdir, tab))
			return (0);
		tab += ncodes;
	}
	return (1);
}
#endif

/*
 * Accept matteing-only ExtraSamples tag.
 */
static
TIFFFetchExtraSamples(tif, dp)
	TIFF *tif;
	TIFFDirEntry *dp;
{
	int type;
	
	if (dp->tdir_count != 1) {
		TIFFError(tif->tif_name,
		    "Can not handle more than 1 extra sample/pixel");
		return (0);
	}
	type = TIFFExtractData(tif, dp->tdir_type, dp->tdir_offset);
	if (type != EXTRASAMPLE_ASSOCALPHA) {
		TIFFError(tif->tif_name,
		    "Can only handle associated-alpha extra samples");
		return (0);
	}
	return (TIFFSetField(tif, TIFFTAG_MATTEING, 1));
}
