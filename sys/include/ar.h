/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define	ARMAG	"!<arch>\n"
#define	SARMAG	8

#define	ARFMAG	"`\n"
#define SARNAME	16

struct	ar_hdr
{
	int8_t	name[SARNAME];
	int8_t	date[12];
	int8_t	uid[6];
	int8_t	gid[6];
	int8_t	mode[8];
	int8_t	size[10];
	int8_t	fmag[2];
};
#define	SAR_HDR	(SARNAME+44)
