/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


#define BMP_RGB      	0
#define BMP_RLE8     	1
#define BMP_RLE4     	2
#define BMP_BITFIELDS	3

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
} Rgb;

#define Filehdrsz	14

typedef struct {
        short	type;
        long	size;		/* file size, not structure size */
        short	reserved1;
        short	reserved2;
        long	offbits;
} Filehdr;

typedef struct {
	long	size;		/* Size of the Bitmap-file */
	long	lReserved;	/* Reserved */
	long	dataoff;	/* Picture data location */
	long	hsize;		/* Header-Size */
	long	width;		/* Picture width (pixels) */
	long	height;		/* Picture height (pixels) */
	short	planes;		/* Planes (must be 1) */
	short	bpp;		/* Bits per pixel (1, 4, 8 or 24) */
	long	compression;	/* Compression mode */
	long	imagesize;	/* Image size (bytes) */
	long	hres;		/* Horizontal Resolution (pels/meter) */
	long	vres;		/* Vertical Resolution (pels/meter) */
	long	colours;	/* Used Colours (Col-Table index) */
	long	impcolours;	/* Important colours (Col-Table index) */
} Infohdr;
