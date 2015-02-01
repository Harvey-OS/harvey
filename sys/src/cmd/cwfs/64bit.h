/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * fundamental constants and types of the implementation
 * changing any of these changes the layout on disk
 */

/* the glorious new, incompatible (on disk) 64-bit world */

/* keeping NAMELEN ≤ 50 bytes permits 3 Dentrys per mag disk sector */
enum {
	NAMELEN		= 56,		/* max size of file name components */
	NDBLOCK		= 6,		/* number of direct blocks in Dentry */
	NIBLOCK		= 4,		/* max depth of indirect blocks */
};

/*
 * file offsets & sizes, in bytes & blocks.  typically long or vlong.
 * vlong is used in the code where would be needed if Off were just long.
 */
typedef vlong Off;

#undef COMPAT32
#define swaboff swab8
