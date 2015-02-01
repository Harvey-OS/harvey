/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *
 * The font data for a printer is saved in an array of the following type.
 *
 */

typedef struct map {
	char	*font;		/* a request for this PostScript font */
	char	*file;		/* means copy this unix file */
	int	downloaded;	/* TRUE after *file is downloaded */
} Map;

Map	*allocate();

