#ifndef lint
static char rcsid[] = "$Header: /usr/people/sam/tiff/libtiff/RCS/tif_tile.c,v 1.9 92/02/10 19:06:47 sam Exp $";
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
 * Tiled Image Support Routines.
 */
#include "tiffioP.h"

/*
 * Compute which tile an (x,y,z,s) value is in.
 */
u_int
TIFFComputeTile(tif, x, y, s, z)
	TIFF *tif;
	u_long x, y, z;
	u_int s;
{
	TIFFDirectory *td = &tif->tif_dir;
	u_long dx = td->td_tilewidth;
	u_long dy = td->td_tilelength;
	u_long dz = td->td_tiledepth;
	u_int tile = 1;

	if (td->td_imagedepth == 1)
		z = 0;
	if (dx == (u_long) -1)
		dx = td->td_imagewidth;
	if (dy == (u_long) -1)
		dy = td->td_imagelength;
	if (dz == (u_long) -1)
		dz = td->td_imagedepth;
	if (dx != 0 && dy != 0 && dz != 0) {
		u_int xpt = howmany(td->td_imagewidth, dx); 
		u_int ypt = howmany(td->td_imagelength, dy); 
		u_int zpt = howmany(td->td_imagedepth, dz); 

		if (td->td_planarconfig == PLANARCONFIG_SEPARATE) 
			tile = (xpt*ypt*zpt)*s +
			     (xpt*ypt)*(z/dz) +
			     xpt*(y/dy) +
			     x/dx;
		else
			tile = (xpt*ypt)*(z/dz) + xpt*(y/dy) + x/dx + s;
	}
	return (tile);
}

/*
 * Check an (x,y,z,s) coordinate
 * against the image bounds.
 */
TIFFCheckTile(tif, x, y, z, s)
	TIFF *tif;
	u_long x, y, z;
	u_int s;
{
	TIFFDirectory *td = &tif->tif_dir;

	if (x >= td->td_imagewidth) {
		TIFFError(tif->tif_name, "Col %d out of range, max %d",
		    x, td->td_imagewidth);
		return (0);
	}
	if (y >= td->td_imagelength) {
		TIFFError(tif->tif_name, "Row %d out of range, max %d",
		    y, td->td_imagelength);
		return (0);
	}
	if (z >= td->td_imagedepth) {
		TIFFError(tif->tif_name, "Depth %d out of range, max %d",
		    z, td->td_imagedepth);
		return (0);
	}
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE &&
	    s >= td->td_samplesperpixel) {
		TIFFError(tif->tif_name, "Sample %d out of range, max %d",
		    s, td->td_samplesperpixel);
		return (0);
	}
	return (1);
}

/*
 * Compute how many tiles are in an image.
 */
u_int
TIFFNumberOfTiles(tif)
	TIFF *tif;
{
	TIFFDirectory *td = &tif->tif_dir;
	u_long dx = td->td_tilewidth;
	u_long dy = td->td_tilelength;
	u_long dz = td->td_tiledepth;
	u_int ntiles;

	if (dx == (u_long) -1)
		dx = td->td_imagewidth;
	if (dy == (u_long) -1)
		dy = td->td_imagelength;
	if (dz == (u_long) -1)
		dz = td->td_imagedepth;
	ntiles = (dx != 0 && dy != 0 && dz != 0) ?
	    (howmany(td->td_imagewidth, dx) * howmany(td->td_imagelength, dy) *
		howmany(td->td_imagedepth, dz)) :
	    0;
	return (ntiles);
}

/*
 * Compute the # bytes in each row of a tile.
 */
u_long
TIFFTileRowSize(tif)
	TIFF *tif;
{
	TIFFDirectory *td = &tif->tif_dir;
	u_long rowsize;
	
	if (td->td_tilelength == 0 || td->td_tilewidth == 0)
		return (0);
	rowsize = td->td_bitspersample * td->td_tilewidth;
	if (td->td_planarconfig == PLANARCONFIG_CONTIG)
		rowsize *= td->td_samplesperpixel;
	return (howmany(rowsize, 8));
}

/*
 * Compute the # bytes in a variable length, row-aligned tile.
 */
u_long
TIFFVTileSize(tif, nrows)
	TIFF *tif;
	u_long nrows;
{
	TIFFDirectory *td = &tif->tif_dir;
	u_long tilesize;

	if (td->td_tilelength == 0 || td->td_tilewidth == 0 ||
	    td->td_tiledepth == 0)
		return (0);
#ifdef YCBCR_SUPPORT
	if (td->td_planarconfig == PLANARCONFIG_CONTIG &&
	    td->td_photometric == PHOTOMETRIC_YCBCR) {
		/*
		 * Packed YCbCr data contain one Cb+Cr for every
		 * HorizontalSampling*VerticalSampling Y values.
		 * Must also roundup width and height when calculating
		 * since images that are not a multiple of the
		 * horizontal/vertical subsampling area include
		 * YCbCr data for the extended image.
		 */
		u_long w =
		    roundup(td->td_tilewidth, td->td_ycbcrsubsampling[0]);
		u_long rowsize = howmany(w*td->td_bitspersample, 8);
		u_long samplingarea =
		    td->td_ycbcrsubsampling[0]*td->td_ycbcrsubsampling[1];
		nrows = roundup(nrows, td->td_ycbcrsubsampling[1]);
		/* NB: don't need howmany here 'cuz everything is rounded */
		tilesize = nrows*rowsize + 2*(nrows*rowsize / samplingarea);
	} else
#endif
		tilesize = nrows * TIFFTileRowSize(tif);
	return (tilesize * td->td_tiledepth);
}

/*
 * Compute the # bytes in a row-aligned tile.
 */
u_long
TIFFTileSize(tif)
	TIFF *tif;
{
	return (TIFFVTileSize(tif, tif->tif_dir.td_tilelength));
}
