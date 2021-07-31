#ifndef lint
static char rcsid[] = "$Header: /usr/people/sam/tiff/libtiff/RCS/tif_open.c,v 1.33 92/02/14 13:40:51 sam Exp $";
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
 */
#include "tiffioP.h"
#include "prototypes.h"

#if USE_PROTOTYPES
extern	int TIFFDefaultDirectory(TIFF*);
#else
extern	int TIFFDefaultDirectory();
#endif

static const long typemask[13] = {
	0,		/* TIFF_NOTYPE */
	0x000000ff,	/* TIFF_BYTE */
	0xffffffff,	/* TIFF_ASCII */
	0x0000ffff,	/* TIFF_SHORT */
	0xffffffff,	/* TIFF_LONG */
	0xffffffff,	/* TIFF_RATIONAL */
	0x000000ff,	/* TIFF_SBYTE */
	0x000000ff,	/* TIFF_UNDEFINED */
	0x0000ffff,	/* TIFF_SSHORT */
	0xffffffff,	/* TIFF_SLONG */
	0xffffffff,	/* TIFF_SRATIONAL */
	0xffffffff,	/* TIFF_FLOAT */
	0xffffffff,	/* TIFF_DOUBLE */
};
static const int bigTypeshift[13] = {
	0,		/* TIFF_NOTYPE */
	24,		/* TIFF_BYTE */
	0,		/* TIFF_ASCII */
	16,		/* TIFF_SHORT */
	0,		/* TIFF_LONG */
	0,		/* TIFF_RATIONAL */
	16,		/* TIFF_SBYTE */
	16,		/* TIFF_UNDEFINED */
	24,		/* TIFF_SSHORT */
	0,		/* TIFF_SLONG */
	0,		/* TIFF_SRATIONAL */
	0,		/* TIFF_FLOAT */
	0,		/* TIFF_DOUBLE */
};
static const int litTypeshift[13] = {
	0,		/* TIFF_NOTYPE */
	0,		/* TIFF_BYTE */
	0,		/* TIFF_ASCII */
	0,		/* TIFF_SHORT */
	0,		/* TIFF_LONG */
	0,		/* TIFF_RATIONAL */
	0,		/* TIFF_SBYTE */
	0,		/* TIFF_UNDEFINED */
	0,		/* TIFF_SSHORT */
	0,		/* TIFF_SLONG */
	0,		/* TIFF_SRATIONAL */
	0,		/* TIFF_FLOAT */
	0,		/* TIFF_DOUBLE */
};

/*
 * Initialize the bit fill order, the
 * shift & mask tables, and the byte
 * swapping state according to the file
 * contents and the machine architecture.
 */
static
DECLARE3(TIFFInitOrder, register TIFF*, tif, int, magic, int, bigendian)
{
	/* XXX how can we deduce this dynamically? */
	tif->tif_fillorder = FILLORDER_MSB2LSB;

	tif->tif_typemask = typemask;
	if (magic == TIFF_BIGENDIAN) {
		tif->tif_typeshift = bigTypeshift;
		if (!bigendian)
			tif->tif_flags |= TIFF_SWAB;
	} else {
		tif->tif_typeshift = litTypeshift;
		if (bigendian)
			tif->tif_flags |= TIFF_SWAB;
	}
}

static int
DECLARE2(getMode, char*, mode, char*, module)
{
	int m = -1;

	switch (mode[0]) {
	case 'r':
		m = O_RDONLY;
		if (mode[1] == '+')
			m = O_RDWR;
		break;
	case 'w':
	case 'a':
		m = O_RDWR|O_CREAT;
		if (mode[0] == 'w')
			m |= O_TRUNC;
		break;
	default:
		TIFFError(module, "\"%s\": Bad mode", mode);
		break;
	}
	return (m);
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF *
TIFFOpen(name, mode)
	char *name, *mode;
{
	static char module[] = "TIFFOpen";
	int m, fd;

	m = getMode(mode, module);
	if (m == -1)
		return ((TIFF *)0);
	fd = TIFFOpenFile(name, m, 0666);
	if (fd < 0) {
		TIFFError(module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}
	return (TIFFFdOpen(fd, name, mode));
}

/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF *
TIFFFdOpen(fd, name, mode)
	int fd;
	char *name, *mode;
{
	static char module[] = "TIFFFdOpen";
	TIFF *tif;
	int m, bigendian;

	m = getMode(mode, module);
	if (m == -1)
		goto bad2;
	tif = (TIFF *)malloc(sizeof (TIFF) + strlen(name) + 1);
	if (tif == NULL) {
		TIFFError(module, "%s: Out of memory (TIFF structure)", name);
		goto bad2;
	}
	bzero((char *)tif, sizeof (*tif));
	tif->tif_name = (char *)tif + sizeof (TIFF);
	strcpy(tif->tif_name, name);
	tif->tif_fd = fd;
	tif->tif_mode = m &~ (O_CREAT|O_TRUNC);
	tif->tif_curdir = -1;		/* non-existent directory */
	tif->tif_curoff = 0;
	tif->tif_curstrip = -1;		/* invalid strip */
	tif->tif_row = -1;		/* read/write pre-increment */
	{ int one = 1; bigendian = (*(char *)&one == 0); }
	/*
	 * Read in TIFF header.
	 */
	if (!ReadOK(fd, &tif->tif_header, sizeof (TIFFHeader))) {
		if (tif->tif_mode == O_RDONLY) {
			TIFFError(name, "Cannot read TIFF header");
			goto bad;
		}
		/*
		 * Setup header and write.
		 */
		tif->tif_header.tiff_magic =  bigendian ?
		    TIFF_BIGENDIAN : TIFF_LITTLEENDIAN;
		tif->tif_header.tiff_version = TIFF_VERSION;
		tif->tif_header.tiff_diroff = 0;	/* filled in later */
		if (!WriteOK(fd, &tif->tif_header, sizeof (TIFFHeader))) {
			TIFFError(name, "Error writing TIFF header");
			goto bad;
		}
		/*
		 * Setup the byte order handling.
		 */
		TIFFInitOrder(tif, tif->tif_header.tiff_magic, bigendian);
		/*
		 * Setup default directory.
		 */
		if (!TIFFDefaultDirectory(tif))
			goto bad;
		tif->tif_diroff = 0;
		return (tif);
	}
	/*
	 * Setup the byte order handling.
	 */
	if (tif->tif_header.tiff_magic != TIFF_BIGENDIAN &&
	    tif->tif_header.tiff_magic != TIFF_LITTLEENDIAN) {
		TIFFError(name,  "Not a TIFF file, bad magic number %d (0x%x)",
		    tif->tif_header.tiff_magic,
		    tif->tif_header.tiff_magic);
		goto bad;
	}
	TIFFInitOrder(tif, tif->tif_header.tiff_magic, bigendian);
	/*
	 * Swap header if required.
	 */
	if (tif->tif_flags & TIFF_SWAB) {
		TIFFSwabShort(&tif->tif_header.tiff_version);
		TIFFSwabLong(&tif->tif_header.tiff_diroff);
	}
	/*
	 * Now check version (if needed, it's been byte-swapped).
	 * Note that this isn't actually a version number, it's a
	 * magic number that doesn't change (stupid).
	 */
	if (tif->tif_header.tiff_version != TIFF_VERSION) {
		TIFFError(name,
		    "Not a TIFF file, bad version number %d (0x%x)",
		    tif->tif_header.tiff_version,
		    tif->tif_header.tiff_version); 
		goto bad;
	}
	tif->tif_flags |= TIFF_MYBUFFER;
	tif->tif_rawcp = tif->tif_rawdata = 0;
	tif->tif_rawdatasize = 0;
	/*
	 * Setup initial directory.
	 */
	switch (mode[0]) {
	case 'r':
		tif->tif_nextdiroff = tif->tif_header.tiff_diroff;
#ifdef MMAP_SUPPORT
		if (TIFFMapFileContents(fd, &tif->tif_base, &tif->tif_size))
			tif->tif_flags |= TIFF_MAPPED;
#endif
		if (TIFFReadDirectory(tif)) {
			tif->tif_rawcc = -1;
			tif->tif_flags |= TIFF_BUFFERSETUP;
			return (tif);
		}
		break;
	case 'a':
		/*
		 * Don't append to file that has information
		 * byte swapped -- we will write data that is
		 * in the opposite order.
		 */
		if (tif->tif_flags & TIFF_SWAB) {
			TIFFError(name,
		"Cannot append to file that has opposite byte ordering");
			goto bad;
		}
		/*
		 * New directories are automatically append
		 * to the end of the directory chain when they
		 * are written out (see TIFFWriteDirectory).
		 */
		if (!TIFFDefaultDirectory(tif))
			goto bad;
		return (tif);
	}
bad:
	tif->tif_mode = O_RDONLY;	/* XXX avoid flush */
	TIFFClose(tif);
	return ((TIFF *)0);
bad2:
	(void) close(fd);
	return ((TIFF *)0);
}

TIFFScanlineSize(tif)
	TIFF *tif;
{
	TIFFDirectory *td = &tif->tif_dir;
	long scanline;
	
	scanline = td->td_bitspersample * td->td_imagewidth;
	if (td->td_planarconfig == PLANARCONFIG_CONTIG)
		scanline *= td->td_samplesperpixel;
	return (howmany(scanline, 8));
}

/*
 * Query functions to access private data.
 */

/*
 * Return open file's name.
 */
char *
TIFFFileName(tif)
	TIFF *tif;
{
	return (tif->tif_name);
}

/*
 * Return open file's I/O descriptor.
 */
int
TIFFFileno(tif)
	TIFF *tif;
{
	return (tif->tif_fd);
}

/*
 * Return read/write mode.
 */
int
TIFFGetMode(tif)
	TIFF *tif;
{
	return (tif->tif_mode);
}

/*
 * Return nonzero if file is organized in
 * tiles; zero if organized as strips.
 */
int
TIFFIsTiled(tif)
	TIFF *tif;
{
	return (isTiled(tif));
}

/*
 * Return current row being read/written.
 */
long
TIFFCurrentRow(tif)
	TIFF *tif;
{
	return (tif->tif_row);
}

/*
 * Return index of the current directory.
 */
int
TIFFCurrentDirectory(tif)
	TIFF *tif;
{
	return (tif->tif_curdir);
}

/*
 * Return current strip.
 */
int
TIFFCurrentStrip(tif)
	TIFF *tif;
{
	return (tif->tif_curstrip);
}

/*
 * Return current tile.
 */
int
TIFFCurrentTile(tif)
	TIFF *tif;
{
	return (tif->tif_curtile);
}
