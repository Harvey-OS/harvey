/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * gzip header fields
 */
enum
{
	GZMAGIC1	= 0x1f,
	GZMAGIC2	= 0x8b,

	GZDEFLATE	= 8,

	GZFTEXT		= 1 << 0,		/* file is text */
	GZFHCRC		= 1 << 1,		/* crc of header included */
	GZFEXTRA	= 1 << 2,		/* extra header included */
	GZFNAME		= 1 << 3,		/* name of file included */
	GZFCOMMENT	= 1 << 4,		/* header comment included */
	GZFMASK		= (1 << 5) -1,		/* mask of specified bits */

	GZXFAST		= 2,			/* used fast algorithm, little compression */
	GZXBEST		= 4,			/* used maximum compression algorithm */

	GZOSFAT		= 0,			/* FAT file system */
	GZOSAMIGA	= 1,			/* Amiga */
	GZOSVMS		= 2,			/* VMS or OpenVMS */
	GZOSUNIX	= 3,			/* Unix */
	GZOSVMCMS	= 4,			/* VM/CMS */
	GZOSATARI	= 5,			/* Atari TOS */
	GZOSHPFS	= 6,			/* HPFS file system */
	GZOSMAC		= 7,			/* Macintosh */
	GZOSZSYS	= 8,			/* Z-System */
	GZOSCPM		= 9,			/* CP/M */
	GZOSTOPS20	= 10,			/* TOPS-20 */
	GZOSNTFS	= 11,			/* NTFS file system */
	GZOSQDOS	= 12,			/* QDOS */
	GZOSACORN	= 13,			/* Acorn RISCOS */
	GZOSUNK		= 255,

	GZCRCPOLY	= 0xedb88320UL,

	GZOSINFERNO	= GZOSUNIX,
};
