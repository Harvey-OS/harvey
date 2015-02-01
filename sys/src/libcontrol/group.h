/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


typedef struct Group Group;

struct Group {
	Control;
	int		lastbut;
	int		border;
	int		mansize;		/* size was set manually */
	int		separation;
	int		selected;
	int		lastkid;
	CImage	*bordercolor;
	CImage	*image;
	int		nkids;
	Control	**kids;		/* mallocated */
	Rectangle	*separators;	/* mallocated */
	int		nseparators;
};
